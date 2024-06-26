/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2007 Knut Franke <knut.franke@gmx.de>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Fit wizard
 ******************************************************************************/

#ifndef FITDIALOG_H
#define FITDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QWidget>

#include "ColorButton.h"
#include "DoubleSpinBox.h"
#include "Fit.h"
#include "FunctionCurve.h"
#include "Graph.h"
#include "MdiSubWindow.h"
#include "ScriptEdit.h"
#include "Table.h"

//! Fit Wizard
class FitDialog : public QDialog
{
    Q_OBJECT

public:
	FitDialog(Graph *g, QWidget* parent = 0);

    void setSrcTables(QList<MdiSubWindow*> tables);

protected:
	void closeEvent (QCloseEvent * e );
    void initFitPage();
	void initEditPage();
	void initAdvancedPage();
	void chooseFitModelsFolder();
	void choosePluginsFolder();

private slots:
	void accept();
    //! Clears the function editor, the parameter names and the function name
    void resetFunction();
	void showFitPage();
	void showEditPage();
	void showAdvancedPage();
	void showFunctionsList(int category);
	void showParseFunctions();
	void showExpression(int function);
	void addFunction();
	void addFunctionName();
	void setFunction(bool ok);
	void saveUserFunction();
	void removeUserFunction();
	void setGraph(Graph *g);
	void activateCurve(const QString&);
	void chooseFolder();
	void changeDataRange();
	void selectSrcTable(int tabnr);
	void enableWeightingParameters(int index);
	void showPointsBox(bool);
	void showParametersTable();
	void showCovarianceMatrix();
	void showResiduals();
	void showConfidenceLimits();
	void showPredictionLimits();

	//! Applies the user changes to the numerical format of the output results
	void applyChanges();

	//! Deletes the result fit curves from the plot
	void deleteFitCurves();

    //! Enable the "Apply" button
	void enableApplyChanges(int = 0);
	void setNumPeaks(int peaks);
	void saveInitialGuesses();
	void loadInitialGuesses();
	void guessInitialValues();
	void returnToFitPage();
	void updatePreview();
	void updatePreviewColor(const QColor &);
	void showPreview(bool on);
	void showParameterRange(bool);
	void guessParameters();

private:
	void loadPlugins();
    void loadUserFunctions();
	void initBuiltInFunctions();
	void modifyGuesses(double* initVal);
	QStringList builtInFunctionNames();
	QStringList userFunctionNames();
	QStringList plugInNames();
	QString parseFormula(const QString& s);
	void setEditorTextColor(const QColor& c);
	void setCurrentFit(int);

    Fit *d_current_fit;
	Graph *d_graph;
	QPointer <Table> d_param_table;
	QList <Fit*> d_user_functions, d_built_in_functions, d_plugins;
	QList <QwtPlotCurve*> d_result_curves;
	QList <MdiSubWindow*> srcTables;
	FunctionCurve *d_preview_curve;

    QCheckBox* boxUseBuiltIn;
	QStackedWidget* tw;
    QPushButton* buttonOk;
	QPushButton* buttonCancel1;
	QPushButton* buttonCancel2;
	QPushButton* buttonCancel3;
	QPushButton* buttonAdvanced;
	QPushButton* buttonClear;
	QPushButton* buttonPlugins;
	QPushButton* btnBack;
	QPushButton* btnSaveGuesses, *btnLoadGuesses, *btnGuess;
	QComboBox* boxCurve;
	QComboBox* boxAlgorithm;
	QTableWidget* boxParams;
	DoubleSpinBox* boxFrom;
	DoubleSpinBox* boxTo;
	DoubleSpinBox* boxTolerance;
	QSpinBox* boxPoints, *generatePointsBox, *boxPrecision, *polynomOrderBox;
	QWidget *fitPage, *editPage, *advancedPage;
	ScriptEdit *editBox;
	QTextEdit *explainBox, *boxFunction;
	QListWidget *categoryBox, *funcBox;
	QLineEdit *boxName;
	QLabel *boxErrorMsg, *boxParam;
	QLabel *lblFunction, *lblPoints, *polynomOrderLabel;
	QPushButton *btnAddFunc, *btnDelFunc, *btnContinue, *btnApply;
	QPushButton *buttonEdit, *btnAddTxt, *btnAddName, *btnDeleteFitCurves;
	ColorButton* boxColor;
	QComboBox *boxWeighting, *tableNamesBox, *colNamesBox;
	QRadioButton *generatePointsBtn, *samePointsBtn;
	QPushButton *btnParamTable, *btnCovMatrix, *btnParamRange;
	QPushButton *btnResiduals, *btnConfidenceLimits, *btnPredictionLimits;
	DoubleSpinBox *boxConfidenceLevel;
	QLineEdit *covMatrixName, *paramTableName;
	QCheckBox *plotLabelBox, *logBox, *scaleErrorsBox, *globalParamTableBox;
	QCheckBox *previewBox;
};

#endif
