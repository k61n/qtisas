/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: See AiConfig.h.
 ******************************************************************************/

#include "AiConfig.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QToolButton>

#include "AiAssistant.h"
#include "ApplicationWindow.h"
#include "editable-link-browser.h"
#include "repository-synchronization.h"
#include "settings.h"

AiConfig::AiConfig(ApplicationWindow *app, QWidget *parent) : QWidget(parent), m_app(app)
{
    auto *lay = new QGridLayout(this);

    lay->addWidget(new QLabel(tr("Preset:")), 0, 0);
    const QString aiDir = app->sasPath + "ai/";
    if (!QDir(aiDir).exists())
        QDir().mkdir(aiDir);

    m_mode = new QComboBox();
    for (const QString &fname : QDir(aiDir).entryList({"*.ai"}, QDir::Files, QDir::Name))
    {
        const QString cname = QFileInfo(fname).completeBaseName();
        QSettings sf(aiDir + fname, QSettings::IniFormat);
        const int m = sf.value("AI/Mode", 0).toInt();
        const int pos = m_mode->count();
        m_mode->addItem(cname, m);
        m_mode->setItemData(pos, cname, Qt::UserRole + 1); // mark as saved preset
    }
    lay->addWidget(m_mode, 0, 1);

    auto *savePresetBtn = new QPushButton(tr("Save as..."));
    savePresetBtn->setToolTip(tr("Save current AI configuration as a named preset"));
    lay->addWidget(savePresetBtn, 0, 2);

    auto *deletePresetBtn = new QPushButton(tr("Delete"));
    deletePresetBtn->setToolTip(tr("Delete this saved preset"));
    deletePresetBtn->setEnabled(false);

    lay->addWidget(new QLabel(tr("API Key:")), 1, 0);
    m_apiKey = new QLineEdit();
    m_apiKey->setEchoMode(QLineEdit::Password);
    lay->addWidget(m_apiKey, 1, 1);
    lay->addWidget(deletePresetBtn, 1, 2);

    auto *gitGetBtn = new QToolButton();
    gitGetBtn->setText(tr("Get from repository"));
    gitGetBtn->setIcon(QIcon(":/git-get.png"));
    gitGetBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    gitGetBtn->setToolTip(tr("Download AI presets from https://iffgit.fz-juelich.de/qtisas/ai"));
    lay->addWidget(gitGetBtn, 2, 2);

    lay->addWidget(new QLabel(tr("Base URL:")), 2, 0);
    m_baseUrl = new QLineEdit();
    lay->addWidget(m_baseUrl, 2, 1);

    lay->addWidget(new QLabel(tr("Model:")), 3, 0);
    m_model = new QLineEdit();
    lay->addWidget(m_model, 3, 1);

    auto *labelMaxTokens = new QLabel(tr("Max tokens:"));
    lay->addWidget(labelMaxTokens, 4, 0);
    m_maxTokens = new QSpinBox();
    m_maxTokens->setRange(256, 32768);
    m_maxTokens->setSingleStep(256);
    m_maxTokens->setValue(4096);
    lay->addWidget(m_maxTokens, 4, 1);

    auto *labelDocsMode = new QLabel(tr("API docs sent:"));
    lay->addWidget(labelDocsMode, 5, 0);
    m_docsMode = new QComboBox();
    m_docsMode->addItem(tr("Compact (.pyi stubs)"), static_cast<int>(AiAssistant::DocsMode::Compact));
    m_docsMode->addItem(tr("Full (.md docs — for large-context models)"),
                        static_cast<int>(AiAssistant::DocsMode::Full));
    m_docsMode->addItem(tr("Integrated (model already knows the API — send none)"),
                        static_cast<int>(AiAssistant::DocsMode::Integrated));
    m_docsMode->setToolTip(tr("Integrated: for a model/deployment that already has QtiSAS's API knowledge "
                              "baked in (fine-tuned, an uploaded knowledge base, or a custom server-side "
                              "system prompt) — sends no doc content at all, and skips the doc-selection "
                              "pre-request entirely."));
    lay->addWidget(m_docsMode, 5, 1);

    auto *clearHistBtn = new QPushButton(tr("Clear conversation history"));
    clearHistBtn->setToolTip(tr("Reset AI chat context"));
    connect(clearHistBtn, &QPushButton::clicked, app, [app]() {
        if (app->aiAssistant())
            app->aiAssistant()->clearHistory();
    });
    lay->addWidget(clearHistBtn, 6, 0, 1, 2);

    auto *infoLabel = new EditableLinkBrowser();
    lay->addWidget(infoLabel, 7, 0, 1, 2);
    lay->setRowStretch(7, 1);

    // User Rules limit — global (independent of the preset/model selected above)
    auto *rulesLimitLabel = new QLabel(tr("User Rules limit:"));
    rulesLimitLabel->setToolTip(tr("Caps addRule() — independent of the AI preset/model selected above."));
    lay->addWidget(rulesLimitLabel, 8, 0);
    m_maxUserRules = new QSpinBox();
    m_maxUserRules->setRange(1, 200);
    m_maxUserRules->setSuffix(tr(" rules"));
    m_maxUserRules->setValue(app->aiSettings.maxUserRules);
    lay->addWidget(m_maxUserRules, 8, 1);
    connect(m_maxUserRules, QOverload<int>::of(&QSpinBox::valueChanged), app,
            [app](int v) { app->aiSettings.maxUserRules = v; });

    // Auto-continue limit — shared cap on automatic EXPLORE-chaining and error-retry rounds
    auto *autoContinueLimitLabel = new QLabel(tr("Auto-continue limit:"));
    autoContinueLimitLabel->setToolTip(
        tr("Caps automatic EXPLORE/retry rounds per request before the AI stops and reports back."));
    lay->addWidget(autoContinueLimitLabel, 9, 0);
    m_maxAutoContinue = new QSpinBox();
    m_maxAutoContinue->setRange(1, 50);
    m_maxAutoContinue->setSuffix(tr(" rounds"));
    m_maxAutoContinue->setValue(app->aiSettings.maxAutoContinue);
    lay->addWidget(m_maxAutoContinue, 9, 1);
    connect(m_maxAutoContinue, QOverload<int>::of(&QSpinBox::valueChanged), app,
            [app](int v) { app->aiSettings.maxAutoContinue = v; });

    // Split mode — on by default; see AiAssistant.h's splitModeActive doc comment
    m_splitByDefault = new QCheckBox(tr("Split large requests into parts by default"));
    m_splitByDefault->setToolTip(
        tr("When off, split mode stays opt-in per-request via a leading \"SPLIT:\" marker.\n"
           "When on, a leading \"NOSPLIT:\" marker forces that one request to skip split mode."));
    m_splitByDefault->setChecked(app->aiSettings.splitByDefault);
    lay->addWidget(m_splitByDefault, 10, 0, 1, 2);
    connect(m_splitByDefault, &QCheckBox::toggled, app, [app](bool on) { app->aiSettings.splitByDefault = on; });

    auto *maxSplitPartsLabel = new QLabel(tr("Max split parts:"));
    maxSplitPartsLabel->setToolTip(tr("Caps how many parts a single decomposition is allowed to produce."));
    lay->addWidget(maxSplitPartsLabel, 11, 0);
    m_maxSplitParts = new QSpinBox();
    m_maxSplitParts->setRange(2, 30);
    m_maxSplitParts->setSuffix(tr(" parts"));
    m_maxSplitParts->setValue(app->aiSettings.maxSplitParts);
    lay->addWidget(m_maxSplitParts, 11, 1);
    connect(m_maxSplitParts, QOverload<int>::of(&QSpinBox::valueChanged), app,
            [app](int v) { app->aiSettings.maxSplitParts = v; });

    // When off, split mode skips the final merge step — each part already ran independently, so
    // this only affects whether a standalone reference script gets produced at the end.
    m_splitRequestMerge = new QCheckBox(tr("Request a merged reference script after split parts complete"));
    m_splitRequestMerge->setToolTip(
        tr("Off: split mode ends right after the last part runs, with no final combined script."));
    m_splitRequestMerge->setChecked(app->aiSettings.splitRequestMerge);
    lay->addWidget(m_splitRequestMerge, 12, 0, 1, 2);
    connect(m_splitRequestMerge, &QCheckBox::toggled, app, [app](bool on) { app->aiSettings.splitRequestMerge = on; });

    // When off, every candidate action script skips the mandatory pre-execution review pass
    // (checklist + the 9 mechanical regression backstops) and goes straight to execution — trading
    // that safety net for one fewer HTTP round-trip per script. detectKnownMistakes() and the other
    // pre-execution mechanical checks still run either way.
    m_selfReviewEnabled = new QCheckBox(tr("Mandatory pre-execution self-review"));
    m_selfReviewEnabled->setToolTip(
        tr("On (default): every candidate script is sent back once for a checklist/regression "
           "review before it runs.\n"
           "Off: skip that review round-trip entirely — a compilable candidate runs immediately."));
    m_selfReviewEnabled->setChecked(app->aiSettings.selfReviewEnabled);
    lay->addWidget(m_selfReviewEnabled, 13, 0, 1, 2);
    connect(m_selfReviewEnabled, &QCheckBox::toggled, app, [app](bool on) { app->aiSettings.selfReviewEnabled = on; });

    // Fill fields when preset changes — loads from .ai file
    auto fillPreset = [=, this](int idx) mutable {
        if (idx < 0 || idx >= m_mode->count())
            return;
        const QString cname = m_mode->itemData(idx, Qt::UserRole + 1).toString();
        if (cname.isEmpty())
            return;
        QSettings sf(aiDir + cname + ".ai", QSettings::IniFormat);
        m_baseUrl->setText(sf.value("AI/BaseUrl").toString());
        m_model->setText(sf.value("AI/Model").toString());
        m_apiKey->setText(sf.value("AI/ApiKey").toString());
        m_apiKey->setPlaceholderText("");
        m_maxTokens->setValue(sf.value("AI/MaxTokens", 4096).toInt());
        m_docsMode->setCurrentIndex(m_docsMode->findData(static_cast<int>(AiAssistant::docsModeFromSettings(sf))));
        m_apiKey->setEnabled(true);
        m_maxTokens->setVisible(true);
        labelMaxTokens->setVisible(true);
        const QString savedInfo = sf.value("AI/Info").toString();
        if (savedInfo.isEmpty())
            infoLabel->setHtml(tr("Preset: <b>%1</b>").arg(cname));
        else
            infoLabel->setNormalizedHtml(savedInfo);
        deletePresetBtn->setEnabled(true);
    };
    connect(m_mode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, fillPreset);

    connect(savePresetBtn, &QPushButton::clicked, this, [=, this]() mutable {
        bool ok;
        const QString name = QInputDialog::getText(this, tr("Save AI Configuration"), tr("Preset name:"),
                                                   QLineEdit::Normal, m_mode->currentText(), &ok)
                                 .trimmed();
        if (!ok || name.isEmpty())
            return;

        const int curIdx = m_mode->currentIndex();
        const int mode = (curIdx >= 0 && curIdx < m_mode->count()) ? m_mode->itemData(curIdx).toInt() : 0;

        QSettings sf(aiDir + name + ".ai", QSettings::IniFormat);
        sf.setValue("AI/ApiKey", m_apiKey->text());
        sf.setValue("AI/BaseUrl", m_baseUrl->text());
        sf.setValue("AI/Model", m_model->text());
        sf.setValue("AI/MaxTokens", m_maxTokens->value());
        sf.setValue("AI/DocsMode", m_docsMode->currentData().toInt());
        sf.setValue("AI/Mode", mode);
        sf.setValue("AI/Info", infoLabel->toNormalizedHtml());
        sf.sync();

        // Update existing entry or append to combobox
        bool found = false;
        for (int i = 0; i < m_mode->count(); ++i)
        {
            if (m_mode->itemData(i, Qt::UserRole + 1).toString() == name)
            {
                found = true;
                m_mode->setCurrentIndex(i);
                break;
            }
        }
        if (!found)
        {
            const int pos = m_mode->count();
            m_mode->addItem(name, mode);
            m_mode->setItemData(pos, name, Qt::UserRole + 1);
            m_mode->setCurrentIndex(pos);
        }
    });

    connect(deletePresetBtn, &QPushButton::clicked, this, [=, this]() mutable {
        const int idx = m_mode->currentIndex();
        const QString cname = m_mode->itemData(idx, Qt::UserRole + 1).toString();
        if (cname.isEmpty())
            return;
        QFile::remove(aiDir + cname + ".ai");
        m_mode->removeItem(idx);
        m_mode->setCurrentIndex(0);
    });

    connect(gitGetBtn, &QToolButton::clicked, this, [=, this]() mutable {
        const QString previousPreset = m_mode->currentData(Qt::UserRole + 1).toString();

        RepositorySynchronization::updateGitInteractive(this, aiDir, repoUrl, {"ai"});

        // Reload preset list
        while (m_mode->count() > 0)
            m_mode->removeItem(m_mode->count() - 1);
        for (const QString &fname : QDir(aiDir).entryList({"*.ai"}, QDir::Files, QDir::Name))
        {
            const QString cname = QFileInfo(fname).completeBaseName();
            QSettings sf(aiDir + fname, QSettings::IniFormat);
            const int m = sf.value("AI/Mode", 0).toInt();
            const int pos = m_mode->count();
            m_mode->addItem(cname, m);
            m_mode->setItemData(pos, cname, Qt::UserRole + 1);
        }
        int restoreIdx = 0;
        for (int i = 0; i < m_mode->count(); ++i)
            if (m_mode->itemData(i, Qt::UserRole + 1).toString() == previousPreset)
            {
                restoreIdx = i;
                break;
            }
        if (m_mode->count() > 0)
        {
            m_mode->setCurrentIndex(restoreIdx);
            fillPreset(restoreIdx);
        }
    });

    // Restore last selected preset by name
    {
        QSettings *s = Settings::DefaultSettings();
        const QString lastPreset = s->value("/AI/SelectedPreset").toString();
        delete s;
        int selectIdx = 0;
        for (int i = 0; i < m_mode->count(); ++i)
        {
            if (m_mode->itemText(i) == lastPreset)
            {
                selectIdx = i;
                break;
            }
        }
        if (m_mode->count() > 0)
        {
            m_mode->setCurrentIndex(selectIdx);
            fillPreset(selectIdx);
            if (app->aiAssistant() && !app->aiAssistant()->apiKey().isEmpty())
                m_apiKey->setText(app->aiAssistant()->apiKey());
        }
    }
}

void AiConfig::apply()
{
    m_app->askAiConfigure(m_apiKey->text().trimmed(), m_baseUrl->text().trimmed(), m_model->text().trimmed(),
                          m_maxTokens->value(), static_cast<AiAssistant::DocsMode>(m_docsMode->currentData().toInt()));
    QSettings *s = Settings::DefaultSettings();
    s->setValue("/AI/SelectedPreset", m_mode->currentText());
    delete s;
    const QString cname = m_mode->currentData(Qt::UserRole + 1).toString();
    if (!cname.isEmpty())
    {
        QSettings sf(m_app->sasPath + "ai/" + cname + ".ai", QSettings::IniFormat);
        sf.setValue("AI/ApiKey", m_apiKey->text().trimmed());
        sf.sync();
    }
}
