/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2010 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Student's t-Test dialog
 ******************************************************************************/

#ifndef StudentTestDialog_H
#define StudentTestDialog_H

#include <QDialog>
#include <StatisticTest.h>

class QGroupBox;
class QComboBox;
class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;
class DoubleSpinBox;
class Table;
class Note;
class CollapsiveGroupBox;

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
