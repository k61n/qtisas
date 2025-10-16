/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: Command completion and historization class for a QLineEdit Widget
 ******************************************************************************/

#ifndef SMARTCOMPLETER_H
#define SMARTCOMPLETER_H

#include <QCompleter>
#include <QLineEdit>
#include <QStringListModel>
#include <QKeyEvent>
#include <QTimer>
#include <QEvent>
#include <QAbstractItemView>
#include <QApplication>

class SmartCompleter : public QObject
{
    Q_OBJECT

  public:
    explicit SmartCompleter(QLineEdit *edit, const QStringList &keywords = {}, QObject *parent = nullptr);
    void addKeywords(const QStringList &list);

  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

  private slots:
    void updateCompletionPrefix();
    void selectCompletion(QCompleter *active, const QString &completion);

  private:
    QLineEdit *m_lineEdit = nullptr;
    QStringListModel *m_keywordModel = nullptr;
    QStringListModel *m_historyModel = nullptr;
    QCompleter *m_completer = nullptr;
    QCompleter *m_historyCompleter = nullptr;

    bool m_blockReturn = false;
};

#endif
