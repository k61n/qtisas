/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: AI assistant implementation
 ******************************************************************************/

#include "AiAssistant.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

static QString loadSystemPrompt(QString *statusOut = nullptr, bool unlimited = false)
{
    QStringList docDirs = {
#ifdef Q_OS_MACOS
        QCoreApplication::applicationDirPath() + "/../Resources/python/doc"
#elif defined(Q_OS_WIN)
        QCoreApplication::applicationDirPath() + "/python/doc"
#else
        QCoreApplication::applicationDirPath() + "/../share/qtisas/python/doc",
        QCoreApplication::applicationDirPath() + "/../qtisas/python/doc",
        QCoreApplication::applicationDirPath() + "/python/doc",
#endif
    };

    QString docDir;
    for (const QString &d : docDirs)
        if (QDir(d).exists())
        {
            docDir = d;
            break;
        }

    if (docDir.isEmpty())
    {
        if (statusOut)
            *statusOut = "AI: API doc directory not found — sending without context.";
        return {};
    }

    // Compact mode: use .pyi stubs (~2500 tokens total, fits 8k context)
    // Unlimited mode: use full .md docs (~16500 tokens, for large-context models)
    const QString pattern = "*-python-api." + (unlimited ? QString("md") : QString("pyi"));
    QStringList files = QDir(docDir).entryList({pattern}, QDir::Files, QDir::Name);
    QString prompt;
    QStringList loaded, missing;

    for (const QString &fname : files)
    {
        QFile f(docDir + "/" + fname);
        if (f.open(QIODevice::ReadOnly))
        {
            prompt += "\n\n=== " + fname + " ===\n";
            prompt += QString::fromUtf8(f.readAll());
            loaded << fname;
        }
        else
            missing << fname;
    }

    if (statusOut)
    {
        if (loaded.isEmpty())
            *statusOut = "▸▸▸ AI: no API doc files found in " + docDir;
        else
            *statusOut = "▸▸▸ AI loaded API docs: " + loaded.join(", ") +
                         (missing.isEmpty() ? "" : " | missing: " + missing.join(", "));
    }
    return prompt;
}

static const QString kBasePrompt =
    "You are an assistant for QtiSAS, a scientific data analysis software. "
    "When asked for code, return ONLY the raw executable Python code — no markdown fences, no explanation. "
    "All QtiSAS API functions (newTable, plot, note, Dan, Fittable, etc.) are global — "
    "call them directly without any prefix, namespace, or module qualifier. "
    "Format code with one statement per line and standard 4-space indentation for blocks. "
    "When asked a question, answer concisely in plain text. "
    "You have NO filesystem access — do not pretend to read, write, or list files. "
    "NEVER import QApplication from PyQt5/PyQt6 — it will crash the application.\n\n";

static const QString kDocsIntro = "QtiSAS Python API documentation is provided below. "
                                  "Use it to answer questions about available classes and functions.\n\n";

// Remove <think>...</think> reasoning blocks emitted by DeepSeek-R1/QwQ-style models.
static QString stripThinkBlocks(const QString &s)
{
    QString r = s;
    while (true)
    {
        const auto open = r.indexOf("<think>", 0, Qt::CaseInsensitive);
        if (open < 0)
            break;
        const auto close = r.indexOf("</think>", open, Qt::CaseInsensitive);
        if (close < 0)
        {
            r = r.left(open);
            break;
        }
        r.remove(open, close - open + 8); // 8 = len("</think>")
    }
    return r;
}

// Strip all markdown code fence lines (``` or ```python etc.) from AI responses.
// The text browser already applies Python highlighting, so fences are pure noise.
static QString stripCodeFences(const QString &s)
{
    QStringList out;
    for (const QString &line : s.split('\n'))
        if (!line.trimmed().startsWith("```"))
            out << line;
    return out.join('\n').trimmed();
}

static QString cleanResponse(const QString &s)
{
    return stripCodeFences(stripThinkBlocks(s)).trimmed();
}

AiAssistant::AiAssistant(QObject *parent) : QObject(parent), m_nam(new QNetworkAccessManager(this))
{
    connect(m_nam, &QNetworkAccessManager::finished, this, &AiAssistant::onReplyFinished);
}

void AiAssistant::clearHistory()
{
    m_history.clear();
    m_pendingUserText.clear();
    m_cachedDocs.clear();
}

QString AiAssistant::historyText() const
{
    if (m_history.isEmpty())
        return "(no history)";
    QStringList lines;
    for (int i = 0; i < m_history.size(); ++i)
    {
        lines << QString("Q%1: %2").arg(i + 1).arg(m_history[i].first.split('\n').first());
        lines << QString("A%1: %2").arg(i + 1).arg(m_history[i].second.left(120).replace('\n', ' '));
    }
    return lines.join('\n');
}

void AiAssistant::configure(const QString &apiKey, const QString &baseUrl, const QString &model, int maxTokens,
                            bool unlimited)
{
    m_apiKey = apiKey;
    m_baseUrl = baseUrl;
    m_model = model;
    m_maxTokens = maxTokens;
    m_unlimited = unlimited;
    m_cachedDocs.clear(); // reload docs if provider/unlimited changes
}

void AiAssistant::ask(const QString &userText, const QString &systemPrompt)
{
    if (m_busy)
    {
        emit errorOccurred("▸▸▸ AI: previous request still in progress.");
        return;
    }
    m_busy = true;

    // Auto-detect API format from URL: Anthropic Messages API vs OpenAI-compatible
    const bool isAnthropic = m_baseUrl.contains("anthropic.com", Qt::CaseInsensitive);

    QString sysPrompt = kBasePrompt;

    if (systemPrompt.isEmpty())
    {
        if (m_cachedDocs.isEmpty())
        {
            QString status;
            m_cachedDocs = loadSystemPrompt(&status, m_unlimited);
            if (!status.isEmpty())
                emit infoMessage(status); // shown only on first load
        }
        if (!m_cachedDocs.isEmpty())
            sysPrompt += kDocsIntro + m_cachedDocs;
    }
    else
        sysPrompt += systemPrompt;

    QJsonObject body;
    body["model"] = m_model;
    body["stream"] = false; // always disable streaming — we read the complete response at once
    body["max_tokens"] = m_maxTokens;

    // Build messages array including full conversation history
    QJsonArray messages;
    const int start = 0;

    if (isAnthropic)
    {
        // Anthropic Messages API: system is a top-level field
        body["system"] = sysPrompt;
        for (int i = start; i < m_history.size(); ++i)
        {
            messages.append(QJsonObject{{"role", "user"}, {"content", m_history[i].first}});
            messages.append(QJsonObject{{"role", "assistant"}, {"content", m_history[i].second}});
        }
        messages.append(QJsonObject{{"role", "user"}, {"content", userText}});
        body["messages"] = messages;
    }
    else
    {
        // OpenAI-compatible: system is the first entry in the messages array
        messages.append(QJsonValue(QJsonObject{{"role", "system"}, {"content", sysPrompt}}));
        for (int i = start; i < m_history.size(); ++i)
        {
            messages.append(QJsonObject{{"role", "user"}, {"content", m_history[i].first}});
            messages.append(QJsonObject{{"role", "assistant"}, {"content", m_history[i].second}});
        }
        messages.append(QJsonValue(QJsonObject{{"role", "user"}, {"content", userText}}));
        body["messages"] = messages;
    }

    // Store pending user text to save in history after response
    m_pendingUserText = userText;

    // Strip any trailing /v1 the user already included in the base URL.
    QString baseUrl = m_baseUrl;
    while (baseUrl.endsWith("/v1") || baseUrl.endsWith("/v1/"))
        baseUrl.chop(baseUrl.endsWith('/') ? 4 : 3);

    const QString endpoint = isAnthropic ? baseUrl + "/v1/messages" : baseUrl + "/v1/chat/completions";

    QNetworkRequest req{QUrl(endpoint)};
    // First request carries the full API docs in the system prompt — allow extra time
    req.setTransferTimeout(m_cachedDocs.isEmpty() ? 60000 : 120000);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (isAnthropic)
    {
        req.setRawHeader("x-api-key", m_apiKey.toUtf8());
        req.setRawHeader("anthropic-version", "2023-06-01");
    }
    else if (!m_apiKey.isEmpty())
    {
        req.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    }

    m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
}

void AiAssistant::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    m_busy = false; // reset on any reply (error or success)

    // Single point for all HTTP/network error reporting — used by both the
    // Qt-error path (network failure) and the post-streaming HTTP-code path.
    const auto emitHttpError = [&](int code, const QByteArray &body) {
        m_pendingUserText.clear();
        if (code == 429)
        {
            const QByteArray ra = reply->rawHeader("Retry-After");
            const QString wait = ra.isEmpty() ? QString() : QString(" Retry after %1 s.").arg(ra.trimmed().constData());
            emit errorOccurred("AI: rate limit reached (HTTP 429)." + wait);
        }
        else if (code == 413)
        {
            emit errorOccurred("AI: request too large (HTTP 413). "
                               "Disable \"Send full API docs\" in Preferences → AI.");
        }
        else
        {
            const QString detail =
                body.left(300).trimmed().isEmpty() ? reply->errorString() : QString::fromUtf8(body.left(300).trimmed());
            emit errorOccurred(code > 0 ? QString("AI HTTP %1: %2").arg(code).arg(detail) : detail);
        }
    };

    if (reply->error() != QNetworkReply::NoError)
    {
        const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        emitHttpError(httpCode, reply->readAll());
        return;
    }

    QByteArray raw = reply->readAll();

    // Ollama NDJSON streaming: each line is a JSON object with message.content + done flag
    // Accumulate content from all lines even when stream:false is ignored
    if (raw.contains('\n') && raw.startsWith('{'))
    {
        const QList<QByteArray> lines = raw.split('\n');
        if (lines.size() > 1)
        {
            QString accumulated;
            for (const QByteArray &line : lines)
            {
                if (line.trimmed().isEmpty())
                    continue;
                const QJsonObject obj = QJsonDocument::fromJson(line.trimmed()).object();
                if (obj.contains("message"))
                    accumulated += obj["message"].toObject()["content"].toString();
                else if (obj.contains("choices"))
                    accumulated += obj["choices"].toArray().at(0).toObject()["delta"].toObject()["content"].toString();
            }
            if (!accumulated.isEmpty())
            {
                const QString response = accumulated.trimmed();
                if (!m_pendingUserText.isEmpty())
                    m_history.append({m_pendingUserText, response});
                m_pendingUserText.clear();
                emit responseReady(cleanResponse(response));
                return;
            }
        }
    }

    // If server ignored stream:false and sent SSE, accumulate all delta content
    if (raw.startsWith("data:"))
    {
        QString accumulated;
        for (const QByteArray &line : raw.split('\n'))
        {
            const QByteArray trimmed = line.trimmed();
            if (!trimmed.startsWith("data:") || trimmed == "data: [DONE]")
                continue;
            const QJsonObject chunk = QJsonDocument::fromJson(trimmed.mid(5).trimmed()).object();
            if (chunk.contains("choices"))
                accumulated += chunk["choices"].toArray().at(0).toObject()["delta"].toObject()["content"].toString();
            else if (chunk.contains("message"))
                accumulated += chunk["message"].toObject()["content"].toString();
        }
        if (!accumulated.isEmpty())
        {
            const QString response = accumulated.trimmed();
            if (!m_pendingUserText.isEmpty())
                m_history.append({m_pendingUserText, response});
            m_pendingUserText.clear();
            emit responseReady(cleanResponse(response));
            return;
        }
    }
    const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (httpCode != 200)
    {
        emitHttpError(httpCode, raw);
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    const QJsonObject obj = doc.object();

    QString text;

    // Anthropic format: { "content": [{"type":"text","text":"..."}] }
    if (obj.contains("content"))
    {
        const QJsonArray arr = obj["content"].toArray();
        for (const auto &v : arr)
            if (v.toObject()["type"].toString() == "text")
                text += v.toObject()["text"].toString();
    }
    // OpenAI-compatible: { "choices": [{"message":{"content":"..."}}] }
    else if (obj.contains("choices"))
    {
        const QJsonArray arr = obj["choices"].toArray();
        if (!arr.isEmpty())
            text = arr[0].toObject()["message"].toObject()["content"].toString();
    }
    else if (obj.contains("error"))
    {
        m_pendingUserText.clear();
        emit errorOccurred("AI error: " + obj["error"].toObject()["message"].toString());
        return;
    }

    if (text.isEmpty())
    {
        m_pendingUserText.clear();
        emit errorOccurred("AI: empty response.");
    }
    else
    {
        const QString response = text.trimmed();
        if (!m_pendingUserText.isEmpty())
            m_history.append({m_pendingUserText, response});
        m_pendingUserText.clear();
        emit responseReady(cleanResponse(response));
    }
}
