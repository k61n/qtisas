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
    "When the user asks for code, return executable Python code without markdown fences. "
    "When the user asks a question, answer concisely in plain text. "
    "Use the QtiSAS Python API documented below.\n\n";

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

void AiAssistant::configure(Mode mode, const QString &apiKey, const QString &baseUrl, const QString &model,
                            int maxTokens, bool unlimited)
{
    m_mode = mode;
    m_apiKey = apiKey;
    m_baseUrl = baseUrl.isEmpty() ? (mode == Ollama ? "http://localhost:11434" : "https://api.anthropic.com") : baseUrl;
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
    if (m_mode == External && m_apiKey.isEmpty())
    {
        emit errorOccurred("AI: API key not configured. Set it in Edit → Preferences → AI.");
        return;
    }
    m_busy = true;

    QString sysPrompt = kBasePrompt;
    const bool isOllamaLocal = (m_mode == Ollama && m_baseUrl.contains("localhost"));

    if (systemPrompt.isEmpty())
    {
        if (m_cachedDocs.isEmpty() && !(isOllamaLocal && !m_unlimited))
        {
            QString status;
            m_cachedDocs = loadSystemPrompt(&status, m_unlimited);
            if (!status.isEmpty())
                emit errorOccurred(status); // shown only on first load
        }
        if (!m_cachedDocs.isEmpty())
            sysPrompt += m_cachedDocs;
    }
    else
        sysPrompt += systemPrompt;

    QJsonObject body;
    body["model"] = m_model;
    body["stream"] = false; // always disable streaming — we read the complete response at once

    // Build messages array including conversation history (last 5 turns max)
    QJsonArray messages;
    const int maxTurns = 5;
    const int start = qMax(0, static_cast<int>(m_history.size()) - maxTurns);

    if (m_mode == External)
    {
        // Anthropic: system is separate, history goes in messages
        body["max_tokens"] = m_maxTokens;
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
        body["max_tokens"] = m_maxTokens; // OpenAI-compatible providers
        if (isOllamaLocal)
        {
            // Ollama native /api/chat — num_predict=-1 lets the model stop at EOS naturally
            // num_ctx must cover all docs: unlimited≈19k tokens, no-docs≈0.2k tokens
            body.remove("max_tokens");
            const int numCtx = m_unlimited ? 32768 : 8192;
            body["options"] = QJsonObject{{"num_ctx", numCtx}, {"num_predict", -1}};
        }
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

    // Use Ollama native /api/chat when base URL is localhost — it reliably honours stream:false
    QString endpoint;
    if (m_mode == External)
        endpoint = m_baseUrl + "/v1/messages";
    else if (isOllamaLocal)
        endpoint = m_baseUrl + "/api/chat";
    else
        endpoint = m_baseUrl + "/v1/chat/completions";

    QNetworkRequest req{QUrl(endpoint)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (m_mode == External)
    {
        req.setRawHeader("x-api-key", m_apiKey.toUtf8());
        req.setRawHeader("anthropic-version", "2023-06-01");
    }
    else
    {
        if (!m_apiKey.isEmpty())
            req.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    }

    m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
}

void AiAssistant::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    m_busy = false; // reset on any reply (error or success)

    if (reply->error() != QNetworkReply::NoError)
    {
        const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (httpCode == 429)
        {
            const QByteArray ra = reply->rawHeader("Retry-After");
            const QString wait = ra.isEmpty() ? QString() : QString(" Retry after %1 s.").arg(ra.trimmed().constData());
            emit errorOccurred("AI: rate limit reached (HTTP 429)." + wait);
            return;
        }
        if (httpCode == 413)
        {
            emit errorOccurred("AI: request too large (HTTP 413). "
                               "Disable \"Send full API docs\" in Preferences → AI.");
            return;
        }
        const QString body = QString::fromUtf8(reply->readAll()).left(300).trimmed();
        const QString detail = body.isEmpty() ? reply->errorString() : body;
        emit errorOccurred(QString("AI HTTP %1: %2").arg(httpCode).arg(detail));
        return;
    }

    QByteArray raw = reply->readAll();

    // Ollama NDJSON streaming: each line is a JSON object with message.content + done flag
    // Accumulate content from all lines even when stream:false is ignored
    if (raw.contains('\n') && !raw.startsWith("data:") && !raw.startsWith('{'))
    {
        // preamble — skip
    }
    else if (raw.contains('\n') && raw.startsWith('{'))
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
                emit responseReady(response);
                reply->deleteLater();
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
            emit responseReady(response);
            reply->deleteLater();
            return;
        }
    }
    const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (httpCode == 429)
    {
        const QByteArray ra = reply->rawHeader("Retry-After");
        const QString wait = ra.isEmpty() ? QString() : QString(" Retry after %1 s.").arg(ra.trimmed().constData());
        emit errorOccurred("AI: rate limit reached (HTTP 429)." + wait);
        return;
    }
    if (httpCode != 200)
    {
        emit errorOccurred(QString("AI HTTP %1: %2").arg(httpCode).arg(QString::fromUtf8(raw.left(300))));
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
    // OpenAI / Ollama OpenAI-compat: { "choices": [{"message":{"content":"..."}}] }
    else if (obj.contains("choices"))
    {
        const QJsonArray arr = obj["choices"].toArray();
        if (!arr.isEmpty())
            text = arr[0].toObject()["message"].toObject()["content"].toString();
    }
    // Ollama native /api/chat: { "message": {"role":"assistant","content":"..."}, "done": true }
    else if (obj.contains("message") && obj.contains("done"))
    {
        text = obj["message"].toObject()["content"].toString();
    }
    else if (obj.contains("error"))
    {
        emit errorOccurred("AI error: " + obj["error"].toObject()["message"].toString());
        return;
    }

    if (text.isEmpty())
        emit errorOccurred("AI: empty response.");
    else
    {
        const QString response = text.trimmed();
        if (!m_pendingUserText.isEmpty())
            m_history.append({m_pendingUserText, response});
        m_pendingUserText.clear();
        emit responseReady(response);
    }
}
