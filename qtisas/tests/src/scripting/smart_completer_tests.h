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
            active = completer.findChild<QCompleter *>(popupKind == 2 ? "history" : "keywords");
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
        auto *history = completer.findChild<QCompleter *>("history");
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
        auto *keywords = completer.findChild<QCompleter *>("keywords");
        auto *history = completer.findChild<QCompleter *>("history");
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
        auto *keywords = completer.findChild<QCompleter *>("keywords");
        QVERIFY(keywords);
        QVERIFY(keywords->popup()->isVisible());
        QCOMPARE(keywords->completionCount(), 1);
        QTest::keyClick(&line, Qt::Key_Return);
        QCOMPARE(line.text(), QString("newTable()"));
        QCOMPARE(submitted.count(), 0);
    }

    // after "Name." only that object's members, not every keyword with the same start (Dan.M -> Mask, Merge)
    static void memberCompletionOffersOnlyTheObjectsMembers()
    {
        QLineEdit line;
        SmartCompleter completer(&line, {"Mask", "Merge", "matrix", "max", "Matrix"});
        completer.setMemberProvider([](const QString &owner) {
            if (owner == "Dan")
                return QStringList{"Mask", "Merge", "Options"};
            if (owner == "Dan.Mask")
                return QStringList{"masks", "newMask"};
            return QStringList();
        });
        line.show();
        line.setFocus();
        QCoreApplication::processEvents();
        auto *members = completer.findChild<QCompleter *>("members");
        auto *keywords = completer.findChild<QCompleter *>("keywords");
        QVERIFY(members && keywords);

        QTest::keyClicks(&line, "Dan.M");
        QVERIFY(members->popup()->isVisible());
        QVERIFY(!keywords->popup()->isVisible());
        QCOMPARE(qobject_cast<QStringListModel *>(members->model())->stringList(), QStringList({"Mask", "Merge"}));
        QTest::keyClick(&line, Qt::Key_Return);
        QCOMPARE(line.text(), QString("Dan.Mask."));
        QCoreApplication::processEvents();
        QVERIFY(members->popup()->isVisible());
        QCOMPARE(qobject_cast<QStringListModel *>(members->model())->stringList(), QStringList({"masks", "newMask"}));
        QTest::keyClicks(&line, "n");
        QCOMPARE(qobject_cast<QStringListModel *>(members->model())->stringList(), QStringList({"newMask"}));
        QTest::keyClick(&line, Qt::Key_Return);
        QCOMPARE(line.text(), QString("Dan.Mask.newMask()"));

        // no object of that name: the keyword list as before
        line.clear();
        QTest::keyClicks(&line, "x.Ma");
        QVERIFY(!members->popup()->isVisible());
        QVERIFY(keywords->popup()->isVisible());
    }
};

#endif // SMART_COMPLETER_TESTS_H
