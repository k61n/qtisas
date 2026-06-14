/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: AI assistant — sends script/note text to an external or local LLM
             and returns generated Python code to the script console.
             API format is auto-detected from the base URL:
             URLs containing "anthropic.com" use the Anthropic Messages API;
             all others use the OpenAI-compatible Chat Completions API.
 ******************************************************************************/

#ifndef AIASSISTANT_H
#define AIASSISTANT_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>

class AiAssistant : public QObject
{
    Q_OBJECT

  public:
    explicit AiAssistant(QObject *parent = nullptr);

    void configure(const QString &apiKey, const QString &baseUrl, const QString &model, int maxTokens = 4096,
                   bool unlimited = false);

    void ask(const QString &userText, const QString &systemPrompt = QString());
    void clearHistory();
    QString historyText() const;

    QString baseUrl() const
    {
        return m_baseUrl;
    }
    QString model() const
    {
        return m_model;
    }
    QString apiKey() const
    {
        return m_apiKey;
    }
    int maxTokens() const
    {
        return m_maxTokens;
    }
    bool unlimited() const
    {
        return m_unlimited;
    }
    int historySize() const
    {
        return static_cast<int>(m_history.size());
    }

  signals:
    void responseReady(const QString &text);
    void errorOccurred(const QString &error);
    void infoMessage(const QString &text);

  private slots:
    void onReplyFinished(QNetworkReply *reply);

  private:
    QNetworkAccessManager *m_nam;
    QString m_apiKey;
    QString m_baseUrl;
    QString m_model;
    int m_maxTokens = 4096;
    bool m_unlimited = false;
    QList<QPair<QString, QString>> m_history; // (user, assistant) pairs
    QString m_pendingUserText;
    QString m_cachedDocs; // loaded once; cleared by clearHistory() or configure()
    bool m_busy = false;
};

#endif
