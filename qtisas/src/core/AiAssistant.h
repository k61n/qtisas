/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: AI assistant — sends script/note text to an external or local LLM
             and returns generated Python code to the script console.
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
    enum Mode
    {
        External,
        Ollama
    }; // External = Anthropic / OpenAI-compatible; Ollama = local

    explicit AiAssistant(QObject *parent = nullptr);

    void configure(Mode mode, const QString &apiKey, const QString &baseUrl, const QString &model, int maxTokens = 4096,
                   bool unlimited = false);

    void ask(const QString &userText, const QString &systemPrompt = QString());
    void clearHistory();

    Mode mode() const
    {
        return m_mode;
    }
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

  signals:
    void responseReady(const QString &text);
    void errorOccurred(const QString &error);

  private slots:
    void onReplyFinished(QNetworkReply *reply);

  private:
    QNetworkAccessManager *m_nam;
    Mode m_mode = External;
    QString m_apiKey;
    QString m_baseUrl = "https://api.anthropic.com";
    QString m_model = "claude-opus-4-8";
    int m_maxTokens = 4096;
    bool m_unlimited = false;
    QList<QPair<QString, QString>> m_history; // (user, assistant) pairs
    QString m_pendingUserText;
    QString m_cachedDocs; // loaded once; cleared by clearHistory() or configure()
    bool m_busy = false;
};

#endif
