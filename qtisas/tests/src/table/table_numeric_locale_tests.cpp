/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: locale tests for numeric text in table cells
 ******************************************************************************/

#include "table_numeric_locale_tests.h"

#include "numericLocale.h"

#include <QtTest/QtTest>

void TableNumericLocaleTests::sameLocalePreservesNumericText()
{
    QLocale c = QLocale::c();
    c.setNumberOptions(QLocale::OmitGroupSeparator);
    QCOMPARE(NumericLocale::convert("12.50", c, c, 'f', 2), QString("12.50"));
}

void TableNumericLocaleTests::emptyAndInvalidTextStayUnchanged()
{
    QLocale c = QLocale::c();
    c.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale german(QLocale::German, QLocale::Germany);
    german.setNumberOptions(QLocale::OmitGroupSeparator);

    QCOMPARE(NumericLocale::convert("", c, german, 'f', 2), QString());
    QCOMPARE(NumericLocale::convert("not a number", c, german, 'f', 2), QString("not a number"));
}

void TableNumericLocaleTests::convertsBetweenCAndGermanDecimals()
{
    QLocale c = QLocale::c();
    c.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale german(QLocale::German, QLocale::Germany);
    german.setNumberOptions(QLocale::OmitGroupSeparator);

    QCOMPARE(NumericLocale::convert("12.50", c, german, 'f', 2), QString("12,50"));
    QCOMPARE(NumericLocale::convert("12,50", german, c, 'f', 2), QString("12.50"));
    QCOMPARE(NumericLocale::convert("1234.5", c, german, 'f', 1), QString("1234,5"));
}

void TableNumericLocaleTests::respectsGermanGroupingOption()
{
    QLocale c = QLocale::c();
    c.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale germanGrouped(QLocale::German, QLocale::Germany);
    QCOMPARE(NumericLocale::convert("1234.5", c, germanGrouped, 'f', 1), QString("1.234,5"));
}

void TableNumericLocaleTests::convertsGeneratedScientificText()
{
    QLocale c = QLocale::c();
    c.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale german(QLocale::German, QLocale::Germany);
    german.setNumberOptions(QLocale::OmitGroupSeparator);

    QCOMPARE(NumericLocale::convert("1.2345E+03", c, german, 'E', 4), QString("1,2345E+03"));
    QCOMPARE(NumericLocale::convert("1,2345E+03", german, c, 'g', QLocale::FloatingPointShortest),
             QString("1.2345E+03")); // the notation is kept
    QCOMPARE(NumericLocale::convert("0.0001", c, german), QString("0,0001"));
}

void TableNumericLocaleTests::convertsGroupedCTextBackToInternalC()
{
    QLocale c = QLocale::c();
    c.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale cGrouped = c;
    cGrouped.setNumberOptions(QLocale::DefaultNumberOptions);
    QCOMPARE(NumericLocale::convert("1234.5", c, cGrouped, 'f', 1), QString("1,234.5"));
    QCOMPARE(NumericLocale::convert("1,234.5", cGrouped, c, 'f', 1), QString("1234.5"));
}

void TableNumericLocaleTests::rejectsCGroupingWhenOmitted()
{
    QLocale c = QLocale::c();
    c.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale cGrouped = c;
    cGrouped.setNumberOptions(QLocale::DefaultNumberOptions);

    double value = 0.0;
    QVERIFY(!NumericLocale::toDouble("1,333", c, value));
    QVERIFY(NumericLocale::toDouble("1234.5", c, value));
    QVERIFY(NumericLocale::toDouble("1,333", cGrouped, value));
    QCOMPARE(value, 1333.0);
}

void TableNumericLocaleTests::rejectsGermanGroupingWhenOmitted()
{
    QLocale german(QLocale::German, QLocale::Germany);
    german.setNumberOptions(QLocale::OmitGroupSeparator);

    double value = 0.0;
    QVERIFY(NumericLocale::toDouble("1,333", german, value));
    QCOMPARE(value, 1.333);
    QVERIFY(!NumericLocale::toDouble("1.333", german, value)); // left to Python, which reads the C number 1.333
    QVERIFY(!NumericLocale::toDouble("0.002", german, value));
}

void TableNumericLocaleTests::leavesPythonExpressionToScriptEngine()
{
    QLocale c = QLocale::c();
    c.setNumberOptions(QLocale::OmitGroupSeparator);
    double value = 0.0;
    QVERIFY(!NumericLocale::toDouble("max(1,2)", c, value));
}

void TableNumericLocaleTests::keepsPlusMinusPrefix()
{
    QLocale german(QLocale::German, QLocale::Germany);
    german.setNumberOptions(QLocale::OmitGroupSeparator);
    QCOMPARE(NumericLocale::convert(QString::fromUtf8("±0.125"), QLocale::c(), german), QString::fromUtf8("±0,125"));
    QCOMPARE(NumericLocale::convert("---", QLocale::c(), german), QString("---"));
}

void TableNumericLocaleTests::convertsBothEndsOfRanges()
{
    QLocale german(QLocale::German, QLocale::Germany);
    german.setNumberOptions(QLocale::OmitGroupSeparator);
    QCOMPARE(NumericLocale::convertRange("0.5..7.5", QLocale::c(), german), QString("0,5..7,5"));
    QCOMPARE(NumericLocale::convertRange("1,5+-0,25", german, QLocale::c()), QString("1.5+-0.25"));
    QCOMPARE(NumericLocale::convertRange("..", QLocale::c(), german), QString(".."));
}

void TableNumericLocaleTests::strictParsingRejectsOmittedGrouping()
{
    QLocale german(QLocale::German, QLocale::Germany);
    german.setNumberOptions(QLocale::OmitGroupSeparator);
    double value = -1.0;
    QVERIFY(!NumericLocale::toDouble("1,234", QLocale::c(), value));
    QCOMPARE(value, -1.0); // unchanged on failure
    QVERIFY(!NumericLocale::toDouble("1.500", german, value));
    QVERIFY(NumericLocale::toDouble("1,5", german, value));
    QCOMPARE(value, 1.5);
}

void TableNumericLocaleTests::textColumnsAcceptCOrLocale()
{
    QLocale german(QLocale::German, QLocale::Germany);
    german.setNumberOptions(QLocale::OmitGroupSeparator);
    double value = 0.0;
    QVERIFY(NumericLocale::toDoubleCOrLocale("0.25", german, value));
    QCOMPARE(value, 0.25);
    QVERIFY(NumericLocale::toDoubleCOrLocale("0,125", german, value)); // not C 125
    QCOMPARE(value, 0.125);
    QVERIFY(!NumericLocale::toDoubleCOrLocale("label", german, value));
}

void TableNumericLocaleTests::replacesDecimalPointInMixedText()
{
    QLocale german(QLocale::German, QLocale::Germany);
    german.setNumberOptions(QLocale::OmitGroupSeparator);
    const QString c = "SD=11 MODE=3 REL=1e-12 DER=0.001 DSSV=yes";
    QCOMPARE(NumericLocale::replaceDecimalPoint(c, QLocale::c(), german),
             QString("SD=11 MODE=3 REL=1e-12 DER=0,001 DSSV=yes"));
    QCOMPARE(NumericLocale::replaceDecimalPoint("DER=0,001", german, QLocale::c()), QString("DER=0.001"));
    QCOMPARE(NumericLocale::replaceDecimalPoint(c, QLocale::c(), QLocale(QLocale::English)), c);
}

QTEST_APPLESS_MAIN(TableNumericLocaleTests)
