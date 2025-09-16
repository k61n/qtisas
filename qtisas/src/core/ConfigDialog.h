/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Preferences dialog
 ******************************************************************************/

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFontComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpacerItem>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTabWidget>
#include <QWidget>

#include "ApplicationWindow.h"
#include "ColorButton.h"
#include "ColorMapEditor.h"
#include "DoubleSpinBox.h"
#include "FrameWidget.h"
#include "PatternBox.h"
#include "PenStyleBox.h"
#include "SymbolBox.h"

//! Preferences dialog
class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
	//! Constructor
	/**
	 * \param parent parent widget (must be the application window!=
	 * \param fl window flags
	 */
    explicit ConfigDialog(QWidget *parent, Qt::WindowFlags fl = Qt::WindowFlags());
	void setColumnSeparator(const QString& sep);

private slots:
    virtual void languageChange();
	void insertLanguagesList();

	void accept();
	void apply();
	void resetDefaultSettings();

	void setCurrentPage(int index);

	//table fonts
	void pickTextFont();
	void pickHeaderFont();

	//graph fonts
	void pickAxesFont();
	void pickNumbersFont();
	void pickLegendFont();
	void pickTitleFont();

	void showFrameWidth(bool ok);

	//application
	void pickApplicationFont();

	//2D curves
	int curveStyle();
	void pick3DTitleFont();
	void pick3DNumbersFont();
	void pick3DAxesFont();

	//Fitting
	void showPointsBox(bool);

	void switchToLanguage(int param);

#ifdef SCRIPTING_PYTHON
    void showStartupScriptsFolder(const QString &);
#endif
	void rehighlight();
    void customizeNotes();
	void chooseTexCompiler();
	bool validateTexCompiler();

	void enableMajorGrids(bool on);
	void enableMinorGrids(bool on);

	void updateCanvasSize(int unit);
	void adjustCanvasHeight(double width);
	void adjustCanvasWidth(double height);

	void moveColor(bool up = true);
	void moveColorDown();
	void removeColor();
	void newColor();
    void saveSymbolColorList();
	void showColorDialog(int, int);
	void changeColorName(int, int);
	void updateSymbolsList(int);
	void setCurrentSymbol(SymbolBox *);
	void loadDefaultSymbols();
	void moveSymbol(bool up = true);
	void moveSymbolDown(){moveSymbol(false);};
	void showGridOptions(int axis);
	void updateGrid();
	void majorGridEnabled(bool);
	void minorGridEnabled(bool);
	void enableCurveAntialiasingSizeBox(bool);
    //+++//
    void getMagicTemplate();
    void findSymbolColorMaps(QString colorPath);
    void colorsListSelected(int currentColorList);
    //---//
private:
	void setApplication(ApplicationWindow *app);
	void setSymbolsList(const QList<int>& symbList);
	void setColorsList(const QList<QColor>& colList, const QStringList& colNames);
	void initPlotsPage();
	void initAppPage();
	void initCurvesPage();
	void initAxesPage();
	void initGridPage();
	void initPlots3DPage();
	void initTablesPage();
	void initConfirmationsPage();
	void initFileLocationsPage();
	void initFittingPage();
	void initNotesPage();
	void initLayerGeometryPage();
	void initLayerSpeedPage();
    
//+++//
	void initQtiSasPage();
//---//
    
	//! Calculates a sensible width for the items list
	void updateMenuList();
	bool validFolderPath(const QString& path);
	int convertToPixels(double w, FrameWidget::Unit unit, int dimension);
	double convertFromPixels(int w, FrameWidget::Unit unit, int dimension);

	QFont textFont, headerFont, axesFont, numbersFont, legendFont, titleFont, appFont;
	QFont d_3D_title_font, d_3D_numbers_font, d_3D_axes_font;

	QCheckBox *boxScaleLayersOnPrint, *boxPrintCropmarks, *linearFit2PointsBox;
	QTabWidget *plotsTabWidget, *appTabWidget;
	ColorButton *btnBackground3D, *btnMesh, *btnAxes, *btnLabels, *btnNumbers;
	QGroupBox *colorMapBox;
	ColorMapEditor *colorMapEditor;
	QPushButton	*btnTitleFnt, *btnLabelsFnt, *btnNumFnt;
	ColorButton *buttonBackground, *buttonText, *buttonHeader;
    QPushButton *buttonOk, *buttonCancel, *buttonApply;
	QPushButton* buttonTextFont, *buttonHeaderFont;
	QStackedWidget * generalDialog;
	QWidget *appColors, *tables, *plotOptions, *plotTicks, *plotFonts, *confirm, *plotPrint;
	QWidget *application, *curves, *axesPage, *plots3D, *fitPage, *numericFormatPage, *notesPage, *plotGeometryPage, *plotSpeedPage;
	QPushButton* buttonAxesFont, *buttonNumbersFont, *buttonLegendFont, *buttonTitleFont, *fontsBtn;
	QCheckBox *boxOrthogonal, *logBox, *plotLabelBox, *scaleErrorsBox;
	QCheckBox *boxTitle, *boxFrame, *boxPlots3D, *boxPlots2D, *boxTables, *boxNotes, *boxFolders;
	QCheckBox *boxSave, *boxBackbones, *boxShowLegend, *boxSmoothMesh;
	QCheckBox *boxAutoscaling, *boxMatrices, *boxScaleFonts, *boxResize;
    //+++//
    QLabel * lblHeaderColorRows; //2019
    ColorButton *headerColorRows;//2019
    //---//
	QComboBox *boxMajTicks, *boxMinTicks, *boxStyle, *boxCurveStyle, *boxSeparator, *boxLanguage, *boxDecimalSeparator;
	QComboBox *boxClipboardLocale, *boxProjection;
	QLabel *lblClipboardSeparator, *lblFloorStyle;
	QSpinBox *boxMinutes, *boxLineWidth, *boxFrameWidth, *boxResolution, *boxMargin, *boxPrecision, *boxAppPrecision;
	QSpinBox *boxSymbolSize, *boxMinTicksLength, *boxMajTicksLength, *generatePointsBox;
	DoubleSpinBox *boxCurveLineWidth;
	ColorButton *btnWorkspace, *btnPanels, *btnPanelsText;
	QListWidget * itemsList;
	QLabel *labelFrameWidth, *lblLanguage, *lblWorkspace, *lblPanels, *lblPageHeader;
	QLabel *lblPanelsText, *lblFonts, *lblStyle, *lblDecimalSeparator, *lblAppPrecision;
	QGroupBox *groupBoxConfirm;
	QGroupBox *groupBoxTableFonts, *groupBoxTableCol;
	QLabel *lblSeparator, *lblTableBackground, *lblTextColor, *lblHeaderColor;

	QLabel *lblSymbSize, *lblAxesLineWidth, *lblCurveStyle, *lblResolution, *lblPrecision;
	QGroupBox *groupBox3DFonts, *groupBox3DCol;
	QLabel *lblMargin, *lblMajTicks, *lblMajTicksLength, *lblLineWidth, *lblMinTicks, *lblMinTicksLength, *lblPoints, *lblPeaksColor;
	QGroupBox *groupBoxFittingCurve, *groupBoxFitParameters;
	QRadioButton *samePointsBtn, *generatePointsBtn;
    QGroupBox *groupBoxMultiPeak;
	ColorButton *boxPeaksColor;
	QLabel *lblScriptingLanguage, *lblInitWindow;
	QComboBox *boxScriptingLanguage, *boxInitWindow;
	QCheckBox *boxAutoscale3DPlots, *boxTableComments, *boxThousandsSeparator;
	QCheckBox *boxPromptRenameTables, *boxBackupProject, *boxLabelsEditing, *boxEmptyCellGap;
	QWidget *fileLocationsPage;
	QLabel *lblUndoStackSize, *lblEndOfLine;
	QSpinBox *undoStackSizeBox;
	QComboBox *boxEndLine;
#ifdef SCRIPTING_PYTHON
	QLabel *lblPythonConfigDir;
	QLineEdit *pythonConfigDirLine;
	QPushButton *browsePythonScriptsBtn;
	QLabel *lblPythonScriptsDir;
	QLineEdit *pythonScriptsDirLine;
#endif
	QCheckBox *boxUpdateTableValues, *boxTablePasteDialog;
	QGroupBox *groupBackgroundOptions;
	QLabel *labelGraphFrameColor, *labelGraphFrameWidth;
	QLabel *labelGraphBkgColor, *labelGraphCanvasColor;
	QLabel *labelGraphBkgOpacity, *labelGraphCanvasOpacity;
	ColorButton *boxBackgroundColor, *boxCanvasColor, *boxBorderColor;
	QSpinBox *boxBackgroundTransparency, *boxCanvasTransparency, *boxBorderWidth, *boxTabLength;
	QSlider *bkgOpacitySlider, *canvasOpacitySlider, *curveOpacitySlider;
	QCheckBox *completionBox, *lineNumbersBox;
	QLabel *labelTabLength, *labelNotesFont;
	QFontComboBox *boxFontFamily;
	QSpinBox *boxFontSize;
	QPushButton *buttonItalicFont, *buttonBoldFont;
	QLabel *labelGraphAxesLabelsDist, *labelTickLabelsDist;
	QSpinBox *boxAxesLabelsDist, *boxTickLabelsDist;
	QLabel *xBottomLabel, *xTopLabel, *yLeftLabel, *yRightLabel, *enableAxisLabel, *showNumbersLabel;
	QCheckBox *boxEnableAxis, *boxShowAxisLabels;
	QGroupBox * enabledAxesGroupBox;
	QGridLayout *enabledAxesGrid;

    QLineEdit *texCompilerPathBox;
    QPushButton *browseTexCompilerBtn;
    QLabel *texCompilerLabel;

	QComboBox *legendDisplayBox, *attachToBox;
	QLabel *legendDisplayLabel, *attachToLabel;

	DoubleSpinBox *boxMajorGridWidth, *boxMinorGridWidth;
	QComboBox *boxMajorGridStyle, *boxMinorGridStyle;
	QCheckBox *boxMajorGrids, *boxMinorGrids;
	ColorButton *btnGrid, *btnGridMinor;
	QLabel *label3DGridsColor, *label3DGridsWidth, *label3DGridsStyle;
	QGroupBox *groupBox3DGrids;

	QGroupBox *groupSyntaxHighlighter;
	ColorButton *buttonCommentColor, *buttonNumericColor, *buttonQuotationColor;
	ColorButton *buttonKeywordColor, *buttonFunctionColor, *buttonClassColor;
	QLabel *buttonCommentLabel, *buttonNumericLabel, *buttonQuotationLabel;
	QLabel *buttonKeywordLabel, *buttonFunctionLabel, *buttonClassLabel;

	QCheckBox *boxMuParserCLocale, *boxConfirmOverwrite, *boxConfirmModifyDataPoints;
	DoubleSpinBox *boxCanvasHeight, *boxCanvasWidth;
	QComboBox *unitBox;
	QLabel *unitBoxLabel, *canvasWidthLabel, *canvasHeightLabel;
	QCheckBox *keepRatioBox, *boxMultiPeakMsgs;

	double aspect_ratio;

	QGroupBox *groupIndexedColors, *symbolGroupBox, *groupIndexedSymbols, *fillCurvesGroupBox;
	QTableWidget *colorsList, *symbolsList;
    QComboBox *colorsListComboBox;
    QStringList symbolColorList;
	QPushButton *btnColorUp, *btnColorDown, *btnRemoveColor, *btnNewColor, *btnLoadDefaultColors;
	QList<QColor> d_indexed_colors;
	QStringList d_indexed_color_names;
	QCheckBox *fillSymbolsBox;
	DoubleSpinBox *symbolEdgeBox;
	QLabel *lblSymbEdge, *lblSymbBox;
	SymbolBox *symbolBox;
	QList<int> d_indexed_symbols;
	QPushButton *btnLoadDefaultSymbols, *btnSymbolUp, *btnSymbolDown;
	PatternBox *patternBox;
	QLabel *lblPattern, *lblCurveAlpha, *lblLineStyle;
	QSpinBox *curveAlphaBox;
	PenStyleBox *lineStyleBox;
	QGroupBox * curvesGroupBox;
	QLabel *lblAxisLabeling;
	QComboBox *axisLabelingBox;
	QCheckBox *boxSynchronizeScales;

	QWidget *gridPage;
	QCheckBox *boxMinorGrid, *boxMajorGrid;
	ColorButton *boxColorMinor, *boxColorMajor;
	PenStyleBox *boxTypeMinor, *boxTypeMajor;
	DoubleSpinBox *boxWidthMinor, *boxWidthMajor;
	QComboBox *boxGridYAxis, *boxGridXAxis;
	QCheckBox *boxAntialiseGrid;
	QListWidget *axesGridList;
	QLabel *gridLineColorLbl, *gridLineWidthLbl, *gridLineTypeLbl, *gridAxesLbl;
	QGroupBox * antialiasingGroupBox;
	QCheckBox *disableAntialiasingBox, *openLastProjectBox;
	QSpinBox *curveSizeBox;
	QPushButton *btnDefaultSettings;

//+++//
	QWidget *qtiSasWidgets;
    
	QSpinBox *fontIncrement;
	QLabel *lblQtiSasFont;
    QLabel *lblQtiSasFontInfo;

    DoubleSpinBox *resoScaling;
    QLabel *lblQtiSasReso;
    QLabel *lblQtiSasResoInfo;

    QComboBox* sasDefaultInterfaceComboBox;
    QLabel *sasDefaultInterfaceLabel;
    QLabel *sasDefaultInterfaceLabelInfo;
    
    QSpacerItem* spacer;
    
    QPushButton* selectMagicTemplate;
    QLabel *magicTemplate;
    QLabel *magicTemplateLabel;
    
    QComboBox* imageFormatComboBox;
    QLabel *imageFormatLabel;
    QLabel *imageFormatLabelInfo;
    QLineEdit *imageFormatRes;
//---//
};

#endif
