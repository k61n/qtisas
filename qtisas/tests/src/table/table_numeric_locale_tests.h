/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: test functions of table_numeric_locale_tests.cpp (a header, so that AUTOMOC needs no .moc include)
 ******************************************************************************/

#ifndef TABLE_NUMERIC_LOCALE_TESTS_H
#define TABLE_NUMERIC_LOCALE_TESTS_H

#include <QObject>

class TableNumericLocaleTests : public QObject
{
    Q_OBJECT

  private slots:
    static void sameLocalePreservesNumericText();
    static void emptyAndInvalidTextStayUnchanged();
    static void convertsBetweenCAndGermanDecimals();
    static void respectsGermanGroupingOption();
    static void convertsGeneratedScientificText();
    static void convertsGroupedCTextBackToInternalC();
    static void rejectsCGroupingWhenOmitted();
    static void rejectsGermanGroupingWhenOmitted();
    static void leavesPythonExpressionToScriptEngine();
    static void keepsPlusMinusPrefix();
    static void convertsBothEndsOfRanges();
    static void strictParsingRejectsOmittedGrouping();
    static void textColumnsAcceptCOrLocale();
    static void replacesDecimalPointInMixedText();
};

#endif
