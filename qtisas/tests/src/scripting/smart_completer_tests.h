/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: SmartCompleterTests
 ******************************************************************************/
#ifndef SMART_COMPLETER_TESTS_H
#define SMART_COMPLETER_TESTS_H

#include "SmartCompleter.h"

#include <QShortcut>
#include <QtTest/QtTest>

class SmartCompleterTests : public QObject
{
    Q_OBJECT

  private slots:
    static void initTestCase()
    {
        qRegisterMetaType<Qt::KeyboardModifiers>();
    }

    static void modifiedReturn_data()
    {
        QTest::addColumn<int>("popupKind");
        QTest::addColumn<bool>("popupTarget");
        QTest::addColumn<int>("modifiers");
        QTest::addColumn<int>("key");
        for (int popup = 0; popup < 3; ++popup)
            for (bool target : {false, true})
            {
                if (!popup && target)
                    continue;
                for (int modifier : {int(Qt::ControlModifier), int(Qt::ControlModifier | Qt::ShiftModifier),
                                     int(Qt::MetaModifier), int(Qt::MetaModifier | Qt::ShiftModifier)})
                    for (int key : {int(Qt::Key_Return), int(Qt::Key_Enter)})
                    {
                        const QByteArray name = QString("popup%1-target%2-mod%3-key%4")
                                                    .arg(popup)
                                                    .arg(target)
                                                    .arg(modifier)
                                                    .arg(key)
                                                    .toLatin1();
                        QTest::newRow(name.constData()) << popup << target << modifier << key;
                    }
            }
    }

    static void modifiedReturn()
    {
        QFETCH(int, popupKind);
        QFETCH(bool, popupTarget);
        QFETCH(int, modifiers);
        QFETCH(int, key);
        QLineEdit line;
        SmartCompleter completer(&line, {"newTable"});
        completer.addHistoryEntry("previous command", "ai");
        QSignalSpy submitted(&completer, &SmartCompleter::modifiedReturnPressed);
        QSignalSpy plainReturn(&line, &QLineEdit::returnPressed);
        line.show();
        line.setFocus();
        QCoreApplication::processEvents();
        QCompleter *active = nullptr;
        if (popupKind)
        {
            for (auto *candidate : completer.findChildren<QCompleter *>())
                if ((candidate->completionRole() == Qt::UserRole) == (popupKind == 2))
                    active = candidate;
            QVERIFY(active);
            active->setCompletionPrefix("");
            active->complete();
            active->popup()->setCurrentIndex(active->completionModel()->index(0, 0));
            QCoreApplication::processEvents();
            QVERIFY(active->popup()->isVisible());
        }
        line.setText("current command");
        QObject *target = popupTarget ? static_cast<QObject *>(active->popup()) : &line;
        QKeyEvent overrideEvent(QEvent::ShortcutOverride, key, Qt::KeyboardModifiers(modifiers));
        overrideEvent.ignore();
        QApplication::sendEvent(target, &overrideEvent);
        QVERIFY(overrideEvent.isAccepted());
        QKeyEvent press(QEvent::KeyPress, key, Qt::KeyboardModifiers(modifiers));
        QApplication::sendEvent(target, &press);
        QCOMPARE(submitted.count(), 1);
        QCOMPARE(qvariant_cast<Qt::KeyboardModifiers>(submitted.at(0).at(0)), Qt::KeyboardModifiers(modifiers));
        QCOMPARE(plainReturn.count(), 0);
        QCOMPARE(line.text(), QString("current command"));
        if (active)
            QVERIFY(!active->popup()->isVisible());
    }

    static void plainReturnStillSelectsHistory()
    {
        QLineEdit line;
        SmartCompleter completer(&line);
        completer.addHistoryEntry("previous command", "ai-chat");
        QSignalSpy submitted(&completer, &SmartCompleter::modifiedReturnPressed);
        line.show();
        line.setFocus();
        QCoreApplication::processEvents();
        QCompleter *history = nullptr;
        for (auto *candidate : completer.findChildren<QCompleter *>())
            if (candidate->completionRole() == Qt::UserRole)
                history = candidate;
        QVERIFY(history);
        history->complete();
        history->popup()->setCurrentIndex(history->completionModel()->index(0, 0));
        QCoreApplication::processEvents();
        line.setText("current command");
        QTest::keyClick(&line, Qt::Key_Return);
        QCOMPARE(submitted.count(), 0);
        QCOMPARE(line.text(), QString("previous command"));
    }

    static void disablingKeywordsKeepsHistoryAndCanBeReenabled()
    {
        QLineEdit line;
        SmartCompleter completer(&line, {"newTable"});
        completer.addHistoryEntry("explain this plot", "ai-chat");
        line.show();
        line.setFocus();
        QCoreApplication::processEvents();
        QCompleter *keywords = nullptr;
        QCompleter *history = nullptr;
        for (auto *candidate : completer.findChildren<QCompleter *>())
            if (candidate->completionRole() == Qt::UserRole)
                history = candidate;
            else
                keywords = candidate;
        QVERIFY(keywords);
        QVERIFY(history);
        QTest::keyClicks(&line, "newT");
        QVERIFY(keywords->popup()->isVisible());
        completer.setKeywordCompletionEnabled(false);
        QVERIFY(!keywords->popup()->isVisible());
        line.clear();
        QTest::keyClicks(&line, "newT");
        QVERIFY(!keywords->popup()->isVisible());
        QTest::keyClick(&line, Qt::Key_Tab);
        QCoreApplication::processEvents();
        QVERIFY(history->popup()->isVisible());
        history->popup()->setCurrentIndex(history->completionModel()->index(0, 0));
        QTest::keyClick(&line, Qt::Key_Return);
        QCOMPARE(line.text(), QString("explain this plot"));
        completer.setKeywordCompletionEnabled(true);
        line.clear();
        QTest::keyClicks(&line, "newT");
        QVERIFY(keywords->popup()->isVisible());
    }

    static void typingPythonShowsAndAcceptsKeyword()
    {
        QLineEdit line;
        SmartCompleter completer(&line, {"newTable", "newMatrix"});
        QSignalSpy submitted(&line, &QLineEdit::returnPressed);
        line.show();
        line.setFocus();
        QCoreApplication::processEvents();
        QTest::keyClicks(&line, "newT");
        QCompleter *keywords = nullptr;
        for (auto *candidate : completer.findChildren<QCompleter *>())
            if (candidate->completionRole() != Qt::UserRole)
                keywords = candidate;
        QVERIFY(keywords);
        QVERIFY(keywords->popup()->isVisible());
        QCOMPARE(keywords->completionCount(), 1);
        QTest::keyClick(&line, Qt::Key_Return);
        QCOMPARE(line.text(), QString("newTable()"));
        QCOMPARE(submitted.count(), 0);
    }
};

#endif // SMART_COMPLETER_TESTS_H
