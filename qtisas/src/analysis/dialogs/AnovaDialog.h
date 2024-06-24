/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: ANOVA dialog
 ******************************************************************************/

#ifndef ANOVADIALOG_H
#define ANOVADIALOG_H

#include <QDialog>
#include <StatisticTest.h>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QPushButton;
class QListWidget;
class QLineEdit;
class QSpinBox;
class QTreeWidget;
class CollapsiveGroupBox;
class DoubleSpinBox;
class Table;
class Note;

//! Filter options dialog
class AnovaDialog : public QDialog
{
    Q_OBJECT

public:
	AnovaDialog(QWidget* parent, Table *t, const StatisticTest::TestType& type = StatisticTest::AnovaTest, bool twoWay = false);

private slots:
	void accept();
	void addData();
	void removeData();
	void showCurrentFolder(bool);
	void updateLevelBoxes();
	void enableDescriptiveStatistics();

private:
	void closeEvent(QCloseEvent*);
	void acceptNormalityTest();

	void acceptAnova();

	void outputResults(StatisticTest* stats, const QString& s);
	StatisticTest::TestType d_test_type;
	bool d_two_way;

	QListWidget* availableSamples;
	QTreeWidget *selectedSamples;

	QCheckBox *currentFolderBox;
	QCheckBox *showInteractionsBox;
	QCheckBox *showStatisticsBox;

	QPushButton* btnAdd;
	QPushButton* btnRemove;
	QPushButton* buttonOk;

	QComboBox* boxModel;

	QSpinBox *aLevelsBox;
	QSpinBox *bLevelsBox;
	DoubleSpinBox* boxSignificance;

	CollapsiveGroupBox *outputSettingsBox;
	QCheckBox *boxResultsTable;
	QLineEdit *tableNameLineEdit;
	QCheckBox *boxResultsLog;
	QCheckBox *boxNoteWindow;
	QLineEdit *noteNameLineEdit;

	Table *d_table;
	Note *d_note;
};

#endif
