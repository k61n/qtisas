/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: The "Ask AI" tab of ConfigDialog — AI preset management (API
             key/base URL/model/max tokens/docs mode) plus the global AI
             settings (user-rules limit, auto-continue limit, split-mode
             defaults). Lives alongside the rest of the AI subsystem rather
             than inside the general-purpose preferences dialog file.
 ******************************************************************************/

#ifndef AICONFIG_H
#define AICONFIG_H

#include <QWidget>

inline const QString repoUrl = "https://iffgit.fz-juelich.de/qtisas/ai.git";

class ApplicationWindow;
class QComboBox;
class QLineEdit;
class QSpinBox;
class QCheckBox;

class AiConfig : public QWidget
{
    Q_OBJECT
  public:
    explicit AiConfig(ApplicationWindow *app, QWidget *parent = nullptr);

    // Called from ConfigDialog::apply(): pushes the current preset's fields into the live
    // AiAssistant, remembers the selected preset name, and saves the API key back into that
    // preset's .ai file.
    void apply();

  private:
    ApplicationWindow *m_app;
    QComboBox *m_mode;
    QLineEdit *m_apiKey;
    QLineEdit *m_baseUrl;
    QLineEdit *m_model;
    QSpinBox *m_maxTokens;
    QComboBox *m_docsMode;
    QSpinBox *m_maxUserRules;
    QSpinBox *m_maxAutoContinue;
    QCheckBox *m_splitByDefault;
    QSpinBox *m_maxSplitParts;
    QCheckBox *m_splitRequestMerge;
    QCheckBox *m_selfReviewEnabled;
    QCheckBox *m_exploreEnabled;
    QSpinBox *m_requestTimeoutSec;

};

#endif
