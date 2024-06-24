/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Student's t-Test dialog
 ******************************************************************************/

#ifndef STUDENTTESTDIALOG_H
#define STUDENTTESTDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>

#include "CollapsiveGroupBox.h"
#include "DoubleSpinBox.h"
#include "Note.h"
#include "StatisticTest.h"
#include "Table.h"

//! Filter options dialog
class StudentTestDialog : public QDialog
{
    Q_OBJECT

public:
	StudentTestDialog(const StatisticTest::TestType& type, Table *t, bool twoSamples = false, QWidget* parent = 0);

private slots:
    void accept();
	void updateMeanLabels(double);
	void updateMeanLabel();
	void addConfidenceLevel();
	void enableDescriptiveStatistics();

private:
	void closeEvent(QCloseEvent*);
	void acceptStudentTest();
	void acceptChiSquareTest();
	void outputResults(StatisticTest* stats, const QString& s);

	StatisticTest::TestType d_test_type;
	bool d_two_samples;

	QComboBox *boxSample1;
	QComboBox *boxSample2;
	QRadioButton *bothTailButton;
	QRadioButton *leftTailButton;
	QRadioButton *rightTailButton;
	QLabel *meanLabel;
	QLabel *bothTailLabel;
	QLabel *leftTailLabel;
	QLabel *rightTailLabel;
	CollapsiveGroupBox *boxConfidenceInterval;
	QPushButton* buttonAddLevel;
	QPushButton* buttonOk;
	CollapsiveGroupBox *boxPowerAnalysis;
	QSpinBox *boxSampleSize;
	QCheckBox *boxOtherSampleSize;
	DoubleSpinBox* boxMean;
	DoubleSpinBox* boxSignificance;
	DoubleSpinBox* boxPowerLevel;

	QRadioButton *independentTestBtn;
	QRadioButton *pairedTestBtn;

	CollapsiveGroupBox *outputSettingsBox;
	QCheckBox *boxResultsTable;
	QLineEdit *tableNameLineEdit;
	QCheckBox *boxResultsLog;
	QCheckBox *boxNoteWindow;
	QLineEdit *noteNameLineEdit;
	QCheckBox *showStatisticsBox;

	Table *d_table;
	Note *d_note;
};

#endif
