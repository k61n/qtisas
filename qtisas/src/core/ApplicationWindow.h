/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Alex Kargovsky <kargovsky@yumr.phys.msu.su>
    2008 roemmic <roemmic@5a6a7de5-fb12-0410-b871-c33778c25c60>
    2009 Jonas Bähr <jonas@fs.ei.tum.de>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: QtiSAS's main window
 ******************************************************************************/

#ifndef APPLICATIONWINDOW_H
#define APPLICATIONWINDOW_H

#include <iostream>

#include <QAction>
#include <QActionGroup>
#include <QBuffer>
#include <QCompleter>
#include <QDesktopServices>
#include <QDockWidget>
#include <QFile>
#include <QFileInfo>
#include <QLineEdit>
#include <QLocale>
#include <QMainWindow>
#include <QMdiArea>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QShortcut>
#include <QSplitter>
#include <QString>
#include <QToolBar>
#include <QToolButton>
#include <QTranslator>
#include <QUndoView>

#include <gsl/gsl_matrix.h>

#include "ArrowMarker.h"
#include "AssociationsDialog.h"
#include "CurveRangeDialog.h"
#include "ExportDialog.h"
#include "Folder.h"
#include "FunctionDialog.h"
#include "Graph.h"
#include "Graph3D.h"
#include "Grid.h"
#include "ImportExportPlugin.h"
#include "LegendWidget.h"
#include "LinearColorMap.h"
#include "Matrix.h"
#include "MultiLayer.h"
#include "Note.h"
#include "Plot3DDialog.h"
#include "ScalePicker.h"
#include "ScriptEdit.h"
#include "ScriptWindow.h"
#include "Table.h"
#include "TableStatistics.h"
#include "TextEditor.h"
#include "TranslateCurveTool.h"

#ifdef QTISAS
class ascii1d18;
class jnse18;
class svd;
class dan18;
class compile18;
class fittable18;
#endif

class ApplicationWindow: public QMainWindow, public scripted
{
    Q_OBJECT

public:
    ApplicationWindow(bool factorySettings = false);
    explicit ApplicationWindow(const QStringList &l);
    ~ApplicationWindow() override;

    enum ShowWindowsPolicy
    {
        HideAll,
        ActiveFolder,
        SubFolders
    };
    enum WindowType
    {
        NoWindow,
        TableWindow,
        MatrixWindow,
        MultiLayerWindow,
        NoteWindow,
        Plot3DWindow
    };
    enum MatrixToTableConversion
    {
        Direct,
        XYZ,
        YXZ
    };
    enum EndLineChar
    {
        LF,
        CRLF,
        CR
    };
    enum Analysis
    {
        NoAnalysis,
        Integrate,
        Diff,
        FitLinear,
        FitGauss,
        FitLorentz,
        FitSigmoidal,
        FitSlope
    };
    enum LaTeXCompiler
    {
        CodeCogs,
        Local
    };

    FolderListView *lv, *folders;
    QDockWidget *logWindow;

//+++//
#ifdef QTISAS
    QDockWidget  *ascii1dWindow;
    QDockWidget  *jnseWindow;
	QDockWidget  *svdWindow;
	QDockWidget  *danWindow;
    QDockWidget  *compileWindow;
    QDockWidget  *fittableWindow;
#endif
    void copyPythonConfigurationFiles(bool forceInit = false) const;
//---//
    
	QString generateUniqueName(const QString& name, bool increment = true);
	void saveFitFunctions(const QStringList& lst);

	//! \name User custom actions
	//@{
	void loadCustomActions();
	void reloadCustomActions();
    void removeCustomAction(QAction *);
	void addCustomAction(QAction *, const QString& parentName, int index = -1);
    QList<QAction *> customActionsList(){return d_user_actions;};
	QList<QMenu *> customizableMenusList();

	void reloadCustomMenus();
	QMenu* addCustomMenu(const QString& title, const QString& parentName);
	void removeCustomMenu(const QString& title);
	QList<QMenu *> customMenusList(){return d_user_menus;};
	//@}

	QList<QMenu *> menusList();
	QList<QToolBar *> toolBarsList();

	MdiSubWindow *activeWindow(WindowType type = NoWindow);
	void setActiveWindow(MdiSubWindow *w){d_active_window = w;};
	QMdiArea* workspace(){return d_workspace;};

	int matrixUndoStackSize(){return d_matrix_undo_stack_size;};
	void setMatrixUndoStackSize(int size);

	QString endOfLine();
	static QString guessEndOfLine(const QString& sample);
	bool autoUpdateTableValues(){return d_auto_update_table_values;};
	void setAutoUpdateTableValues(bool on = true);

	QCompleter* completer(){return d_completer;};
	void enableCompletion(bool on = true);
	void addWindowsListToCompleter();

	QLocale clipboardLocale(){return d_clipboard_locale;};
	void setClipboardLocale(const QLocale& locale){d_clipboard_locale = locale;};

	QTextEdit *resultsLog(){return results;};
    
#ifdef SCRIPTING_CONSOLE
	QTextEdit *scriptingConsole(){return console;};
#endif

	static QString imageFilter();
	static QString getFileName(QWidget *parent = 0, const QString & caption = QString(),
		const QString & dir = QString(), const QString & filter = QString(),
		QString * selectedFilter = 0, bool save = true, bool confirmOverwrite = true);

	static void memoryAllocationError();
	QColor readColorFromProject(const QString& name);

	void enableMdiArea(bool on = true);
	bool isMdiAreaEnabled(){return d_mdi_windows_area;};

	void displayInfo(const QString& text){info->setText(text);};
	QLineEdit *infoLineEdit(){return info;};

	QList<QColor> indexedColors(){return d_indexed_colors;};
	void setIndexedColors(const QList<QColor>& lst){d_indexed_colors = lst;};

	QList<int> indexedSymbols(){return d_symbols_list;};
	void setIndexedSymbols(const QList<int>& lst){d_symbols_list = lst;};

	QStringList indexedColorNames(){return d_indexed_color_names;};
	void setIndexedColorNames(const QStringList& lst){d_indexed_color_names = lst;};

	void connectScriptEditor(ScriptEdit *);

	//! \name Import/Export Plugins
	//@{
	ImportExportPlugin* exportPlugin(const QString& suffix);
	ImportExportPlugin* importPlugin(const QString& fileName);
	//@}

	bool isFileReadable(const QString&);

	void showNoDataMessage();

    double sigma(double Q);
    bool findActivePlot(MultiLayer *&plot);
    bool findActiveGraph(Graph *&g);
    bool checkTableExistence(const QString &tableName, Table *&w);
    bool checkTableExistence(const QString &tableName);
    bool checkNoteExistence(const QString &noteName);
    bool existWindow(const QString &name);
    
    //+++ status info string
    QLabel *d_status_info;
    QLineEdit *terminal_line;
    QAction *actionCopyStatusBarText;

    QVariant scriptCaller(const QString &scriptCode);

public slots:

    //+++
    //! Show the context menu for the status bar
    void showStatusBarContextMenu(const QPoint &pos);
    
    
	void changeFontSasWidgets();
    void updatePathesInInterfaces();
    void changeSasReso();
    void findColorMaps();

	void open();
	ApplicationWindow* open(const QString& fn, bool factorySettings = false, bool newProject = true);
	ApplicationWindow* openProject(const QString& fn, bool factorySettings = false, bool newProject = true);
    void closeProject();

	/**
	 * \brief Create a new project from a data file.
	 *
	 * \param fn is read as a data file with the default column separator (as set by the user)
	 * and inserted as a table into a new, empty project.
	 * This table is then plotted with the Graph::LineSymbols style.
	 */
	ApplicationWindow * plotFile(const QString& fn);

	/**
	 * \brief Create a new project from a script file.
	 *
	 * \param fn is read as a Python script file and loaded in the command script window.
	 * \param execute specifies if the script should be executed after opening.
	 */
	ApplicationWindow * loadScript(const QString& fn, bool execute = false, bool noGui = false);

	QList<MdiSubWindow *> windowsList();
	QStringList windowsNameList();
	void updateWindowLists(MdiSubWindow *w);
	/*!
    Arranges all the visible project windows in a cascade pattern.
    */
	void cascade();

	void saveProjectAs(const QString& fileName = QString(), bool compress = false);
	bool saveProject(bool compress = false);
	void saveWindowAs(const QString& fileName = QString(), bool compress = false);
	bool saveWindow(MdiSubWindow *w, const QString& fileName = QString(), bool compress = false);

	//! Set the project status to modifed
	void modifiedProject();
	//! Set the project status to saved (not modified)
	void savedProject();
	//! Set the project status to modified and save 'w' as the last modified widget
	void modifiedProject(MdiSubWindow *w);
	//@}

	//! \name Settings
	//@{
	void setDefaultOptions();
	void readSettings();
	void saveSettings();
	void setSaveSettings(bool autoSaving, int min);
	void changeAppStyle(const QString& s);
	void changeAppFont(const QFont& f);
	void updateAppFonts();
	void setAppColors(const QColor& wc,const QColor& pc,const QColor& tpc, bool force = false);

	void initWindow();
	//@}

	//! \name Multilayer Plots
	//@{
	//! Creates a new empty multilayer plot
	MultiLayer* newGraph(const QString& caption = tr("Graph") + "1");
	MultiLayer* multilayerPlot(int c, int r, int style, const MultiLayer::AlignPolicy& align = MultiLayer::AlignLayers);
	MultiLayer* multilayerPlot(Table* w, const QStringList& colList, int style, int startRow = 0, int endRow = -1);
	//! used when restoring a plot from a project file
	MultiLayer* multilayerPlot(const QString& caption, int layers = 1, int rows = 1, int cols = 1);
	MultiLayer* waterfallPlot();
	MultiLayer* waterfallPlot(Table *t, const QStringList& list);
	void connectMultilayerPlot(MultiLayer *g);
	void addLayer();
	void addInsetLayer(bool curves = false);
	void addInsetCurveLayer();
	void deleteLayer();
	void extractGraphs();
    void extractLayers();

	//! Creates a new spectrogram graph
  	MultiLayer* plotSpectrogram(Matrix *m, Graph::CurveType type);
  	MultiLayer* plotGrayScale(Matrix *m = 0);
  	MultiLayer* plotContour(Matrix *m = 0);
  	MultiLayer* plotColorMap(Matrix *m = 0);
  	MultiLayer* plotImage(Matrix *m = 0);
  	MultiLayer* plotImageProfiles(Matrix *m = 0);

	//! Rearrange the layersin order to fit to the size of the plot window
  	void autoArrangeLayers();
	void initMultilayerPlot(MultiLayer* g, const QString& name = QString());
	void plot2VerticalLayers();
	void plot2HorizontalLayers();
	void plot4Layers();
	void plotStackedLayers();
	void plotStackedHistograms();

	void plotStackSharedAxisLayers();
	void plotVerticalSharedAxisLayers();
	void plotHorizontalSharedAxisLayers();
	void plotSharedAxesLayers();
	void plotCustomLayout(bool sharedAxes = false);
	void plotCustomLayoutSharedAxes(){plotCustomLayout(true);};
	//@}

	//! \name 3D Data Plots
	//@{
	Graph3D* newPlot3D(const QString& title = QString());
	Graph3D* plotXYZ(Table* table,const QString& zColName, int type);
	//@}

	//! \name Surface Plots
	//@{
    Graph3D* plotSurface(const QString& formula, double xl, double xr,
					   double yl, double yr, double zl, double zr, int columns = 40, int rows = 30);
	Graph3D* plotParametricSurface(const QString& xFormula, const QString& yFormula,
						const QString& zFormula, double ul, double ur, double vl, double vr,
						int columns, int rows, bool uPeriodic, bool vPeriodic);

	void connectSurfacePlot(Graph3D *plot);
	void newSurfacePlot();
	void editSurfacePlot();
	void remove3DMatrixPlots(Matrix *m);
	void updateMatrixPlots(Matrix *);
	void add3DData();
	void change3DData();
	void change3DData(const QString& colName);
	void change3DMatrix();
	void change3DMatrix(const QString& matrix_name);
	void insertNew3DData(const QString& colName);
	void add3DMatrixPlot();
	void insert3DMatrixPlot(const QString& matrix_name);

	void plot3DWireframe();
	void plot3DHiddenLine();
	void plot3DPolygons();
	void plot3DWireSurface();

	Graph3D* plot3DMatrix(Matrix *m = 0, int style = 5);

	void plot3DRibbon();
	void plot3DScatter();
	void plot3DTrajectory();
	void plot3DBars();
	//@}

	//! \name User-defined Functions
	//@{
	MultiLayer * newFunctionPlot(QStringList &formulas, double start, double end, int points = 100, const QString& var = "x", int type = 0);

	FunctionDialog* functionDialog();
	FunctionDialog* showFunctionDialog(Graph * g, int curve);
	void addFunctionCurve();
	void clearLogInfo();
	void updateFunctionLists(int type, QStringList &formulas);
	void updateSurfaceFuncList(const QString& s);
	//@}

	//! \name Matrices
	//@{
	//! Creates a new empty matrix
	Matrix* newMatrix(int rows = 32, int columns = 32);
	//! To be used when opening a project file only!
	Matrix* newMatrix(const QString& caption, int r, int c);
	Matrix* matrix(const QString& name);
	Matrix* convertTableToMatrix();
	Matrix* tableToMatrix(Table* t);
    Matrix *tableToMatrixRegularXYZ(Table *t = nullptr, const QString &colName = QString());
    

	void convertTableToMatrixRandomXYZ();
	void expandMatrix();
	void shrinkMatrix();
	void smoothMatrix();
	void showMatrixResamplingDialog(bool shrink = false);

    
	void showBinMatrixDialog();
	void initMatrix(Matrix* m, const QString& caption);
	void transposeMatrix();
	void invertMatrix();
	void matrixDeterminant();
	void flipMatrixVertically();
	void flipMatrixHorizontally();
	void rotateMatrix90();
	void rotateMatrixMinus90();
	void viewMatrixImage();
	void viewMatrixTable();
    void exportMatrix(const QString &exportFilter = QString());
	void setMatrixDefaultScale();
	void setMatrixGrayScale();
	void setMatrixRainbowScale();
	void viewMatrixColumnRow();
	void viewMatrixXY();
    void matrixDirectFFT();
    void matrixInverseFFT();
	//@}

	//! \name Tables
	//@{
	//! Creates an empty table
	Table* newTable();

    // getCurrent... functions for python interface
	Table* currentTable();
	MultiLayer* currentPlot();
	Note* currentNote();
	Matrix* currentMatrix();

	//! Used when loading a table from a project file
	Table* newTable(const QString& caption,int r, int c);
	Table* newTable(int r, int c, const QString& name = QString(),const QString& legend = QString());
	/**
	 * \brief Create a Table which is initially hidden; used to return the result of an analysis operation.
	 *
	 * \param name window name (compare MdiSubWindow::MdiSubWindow)
	 * \param label window label (compare MdiSubWindow::MdiSubWindow)
	 * \param r number of rows
	 * \param c number of columns
     * \param text tab/newline - seperated initial content; may be empty
	 */
	Table* newHiddenTable(const QString& name, const QString& label, int r, int c, const QString& text=QString());
	//Table* table(const QString& name);
    Table* table(const QString& name, bool justTableName = false);
	Table* convertMatrixToTableDirect();
	Table* convertMatrixToTableXYZ();
	Table* convertMatrixToTableYXZ();
	Table* matrixToTable(Matrix* m, MatrixToTableConversion conversionType = Direct);
	QList<MdiSubWindow *> tableList();
    QList<MdiSubWindow *> noteList();
    QList<MdiSubWindow *> multilayerList();

    //! Returns true if the project contains tables
	bool hasTable();
	//! Returns a list containing the names of all tables in the project
	QStringList tableNames();

	void connectTable(Table* w);
	void initTable(Table* w, const QString& caption);
	void customTable(Table* w);

    Table *importDatabase(const QString & = QString(), int sheet = -1);
	Table* importWaveFile();
    void importASCII(const QString &fileName = QString());
	void importASCII(const QStringList& files, int import_mode, const QString& local_column_separator, int local_ignored_lines, bool local_rename_columns,
        bool local_strip_spaces, bool local_simplify_spaces, bool local_import_comments,
		QLocale local_separators, const QString& local_comment_string, bool import_read_only, int endLineChar,
		const QList<int>& colTypes = QList<int>(), const QStringList& colFormats = QStringList());
	void exportAllTables(const QString& dir, const QString& filter, const QString& sep, bool colNames, bool colComments, bool expSelection, QString wildCard="*");

	//! recalculate selected cells of current table
	void recalculateTable();

    TableStatistics *newTableStatistics(Table *base, int type, QList<int>, int start = 0, int end = -1,
                                        const QString &caption = QString());
	//@}

	//! \name Graphs
	//@{
	void setPreferences(Graph* g);
	void setGraphDefaultSettings(bool autoscale,bool scaleFonts,bool resizeLayers,bool antialiasing);
	void setArrowDefaultSettings(double lineWidth,  const QColor& c, Qt::PenStyle style,
								int headLength, int headAngle, bool fillHead);

	void plotL();
	void plotP();
	void plotLP();
	void plotPie();
	void plotVerticalBars();
	void plotHorizontalBars();
	void plotStackBar();
	void plotStackColumn();
	void plotArea();
	void plotVertSteps();
	void plotHorSteps();
	void plotSpline();
	void plotVerticalDropLines();
	MultiLayer* plotHistogram();
	MultiLayer* plotHistogram(Matrix *m);
	void plotVectXYXY();
	void plotVectXYAM();
	void plotBox();
	void plotDoubleYAxis();
	void zoomRectanglePlot();
	QString stemPlot(Table *t = 0, const QString& colName = QString(), int power = 0, int startRow = 0, int endRow = -1);
	Note *newStemPlot();

    //! Check whether a table is valid for a 3D plot and display an appropriate error if not
    bool validFor3DPlot(Table *table);
    //! Check whether a table is valid for a 2D plot and display an appropriate error if not
    bool validFor2DPlot(Table *table, Graph::CurveType type);
    //! Generate a new 2D graph
    MultiLayer* generate2DGraph(Graph::CurveType type);
	//@}

	//! \name Image Analysis
	//@{
	void intensityTable();
	void pixelLineProfile();
	void loadImage();
	void loadImage(const QString& fn);
  	Matrix* importImage(const QString& = QString(), bool newWindow = false);
	//@}

	//! \name Export and Print
	//@{
	void exportLayer();
    void exportGraph(const QString &exportFilter = QString());
	void exportAllGraphs();
	void exportPDF();
	void print();
	void printPreview();
	void setPrintPreviewOptions(QPrinter *);
	void printAllPlots();
	//@}

	QStringList columnsList(Table::PlotDesignation plotType = Table::All);

	void undo();
	void redo();

	//! \name MDI Windows
	//@{
	MdiSubWindow* clone(MdiSubWindow* w = 0);
	void rename();
	void renameWindow();

	//!  Called when the user presses F2 and an item is selected in lv.
	void renameWindow(QTreeWidgetItem *item, int, const QString &s);

	//!  Checks weather the new window name is valid and modifies the name.
	bool setWindowName(MdiSubWindow *w, const QString &text);

	void maximizeWindow(QTreeWidgetItem *lbi = 0);
	void maximizeWindow(MdiSubWindow *w);
    void maximizeWindow(const QString &name);
	void minimizeWindow(MdiSubWindow *w = 0);

	void updateWindowStatus(MdiSubWindow* );

	bool hidden(QWidget* window);
	void closeActiveWindow();
	void closeWindow(MdiSubWindow* window);

	//!  Does all the cleaning work before actually deleting a window!
	void removeWindowFromLists(MdiSubWindow* w);

	void hideWindow(MdiSubWindow* window);
	void hideWindow();
	void hideActiveWindow();
	void activateWindow();
	void activateWindow(MdiSubWindow *);
    bool activateWindow(QString name);
	//@}

	//! Show about dialog
	static QMessageBox* about(bool dialog = true);
    void aboutQt();

	void removeCurves(const QString& name);
	QStringList dependingPlots(const QString& caption);
	QStringList depending3DPlots(Matrix *m);
	QStringList multilayerDependencies(QWidget *w);

    //+++
    bool showFullRangeAllPlots(const QString& tableName);
    
	void saveAsTemplate(MdiSubWindow* w = 0, const QString& = QString());
	void openTemplate();
	MdiSubWindow* openTemplate(const QString& fn);

	QString windowGeometryInfo(MdiSubWindow *w);
	static void restoreWindowGeometry(ApplicationWindow *app, MdiSubWindow *w, const QString s);
	void restoreApplicationGeometry();
	void resizeActiveWindow();
	void resizeWindow();

	//! \name List View in Project Explorer
	//@{
	void setListView(const QString& caption,const QString& view);
	void renameListViewItem(const QString& oldName,const QString& newName);
	void setListViewDate(const QString& caption,const QString& date);
	QString listViewDate(const QString& caption);
	void setListViewLabel(const QString& caption,const QString& label);
	//@}

	void updateColNames(const QString& oldName, const QString& newName);
	void updateTableNames(const QString& oldName, const QString& newName);
	void changeMatrixName(const QString& oldName, const QString& newName);
	void updateCurves(Table *t, const QString& name);

	void showTable(Table *, const QString& curve);
	void showTable(QAction *action);

	void addColToTable();
	void cutSelection();
	void copySelection();
	void copyMarker();
	void pasteSelection();
	void clearSelection();
	void copyActiveLayer();

	void newProject();

	//! \name Reading from a Project File
	//@{
	Matrix* openMatrix(ApplicationWindow* app, const QStringList &flist);
	Table* openTable(ApplicationWindow* app, const QStringList &flist);
	TableStatistics* openTableStatistics(const QStringList &flist);
	Graph* openGraph(ApplicationWindow* app, MultiLayer *plot, const QStringList &list);
    void openGraph(ApplicationWindow* app, MultiLayer *plot, const QStringList &list, Graph* &ag, bool newYN);
	void openRecentProject(QAction *action);
	//@}

	//! \name Table Tools
	//@{
	void sortSelection();
	void sortActiveTable();
	void normalizeSelection();
	void normalizeActiveTable();
	void correlate();
	void autoCorrelate();
	void convolute();
	void deconvolute();
	void clearTable();
	void goToRow();
	void goToColumn();
	void moveTableRowUp();
	void moveTableRowDown();
	void adjustColumnWidth();
	void showChiSquareTestDialog();
	void showStudentTestDialog(bool twoSamples = false);
	void showTwoSampleStudentTestDialog(){return showStudentTestDialog(true);};
	void testNormality();

	void showANOVADialog(bool twoWay = false);
	void showTwoWayANOVADialog(){return showANOVADialog(true);};

	//@}

	//! \name Plot Tools
	//@{
	void graphSelectionChanged(SelectionMoveResizer *);
	void lowerActiveEnrichment();
	void raiseActiveEnrichment(bool = true);
	void alignTop();
	void alignBottom();
	void alignLeft();
	void alignRight();
	void newLegend();
	void addTimeStamp();
	void drawLine();
	void drawArrow();
	void drawPoints();
	void addText();
	void addTexFormula();
	void addRectangle();
	void addEllipse();
	void addImage();
	void zoomIn();
	void zoomOut();
	void magnify(int mode = 0);
	void setAutoScale();
//+++//
    void setLogLog();
    void setLinLin();
    void slotLogLog();
    void slotLogLogSingle();
    void slotLinLin();
    void slotLinLinSingle();
    void spLogLinSwitcher(Graph* g, bool logYN);
    void minmaxPositiveXY(Graph* g, double &minX, double &maxX, double &minY, double &maxY, bool onlyPositiveX, bool onlyPositiveY);

    void setMagicTemplate(QString tmpl);
    void setMagicTemplate();
    
    void saveGraphAsProject();//new 2020.04

    QString activeScriptingLanguage()
    {
        return scriptEnv->objectName();
    };
/*
    void setPlusColorMap();
    void setMinusColorMap();
 */
//---//
	void showRangeSelectors();
	void showCursor();
	void showScreenReader();
	void pickPointerCursor();
	void disableTools();
	void pickDataTool( QAction* action );

	void updateLog(const QString& result);
	//@}

	//! \name Fitting
	//@{
	void deleteFitTables();
	void fitLinear();
	void fitSigmoidal();
	void fitGauss();
	void fitLorentz();
	void fitMultiPeak(int profile);
	void fitMultiPeakGauss();
	void fitMultiPeakLorentz();
	void fitSlope();
	//@}

	//! \name Calculus
	//@{
	void integrate();
	void differentiate();
	void analysis(Analysis operation);
	void analyzeCurve(Graph *g, QwtPlotCurve *c, Analysis operation);
	void showDataSetDialog(Analysis operation);
	//@}

	void addErrorBars();
	void movePoints(bool wholeCurve = false);
	void removePoints();

	//! \name Event Handlers
	//@{
    void closeEvent(QCloseEvent *) override;
    void timerEvent(QTimerEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void customEvent(QEvent *e) override;
    
#ifdef Q_OS_MACOS
    void hideEvent(QHideEvent *) override;
#endif
    
	//@}

	//! \name Dialogs
	//@{
	void showFindDialogue();
	//! Show plot style dialog for the active MultiLayer / activeLayer / specified curve or the activeLayer options dialog if no curve is specified (curveKey = -1).
	void showPlotDialog(int curveKey = -1);
	QDialog* showScaleDialog();
	QDialog* showPlot3dDialog();
	AxesDialog* showScalePageFromAxisDialog(int axisPos);
	AxesDialog* showAxisPageFromAxisDialog(int axisPos);
	void showAxisDialog();
	void showGridDialog();
	void showGeneralPlotDialog();
	void showResults(bool ok);
	void showResults(const QString& s, bool ok=true);
	void showEnrichementDialog();
	void showLineDialog();
	void showTitleDialog();
	ExportDialog* showExportASCIIDialog();
	void showCurvesDialog();
	void showCurveRangeDialog();
	CurveRangeDialog* showCurveRangeDialog(Graph *g, int curve);
	AssociationsDialog* showPlotAssociations(int curve);

	void showAxisTitleDialog();
	void showColumnOptionsDialog();
	void showRowsDialog();
	void showDeleteRowsDialog();
	void showColsDialog();
	void showColMenu(int c);
	void showColumnValuesDialog();
	void showExtractDataDialog();

	void showGraphContextMenu();
	void showTableContextMenu(bool selection);
	void showWindowContextMenu();
	void customWindowTitleBarMenu(MdiSubWindow *w, QMenu *menu);
	void showCurveContextMenu(QwtPlotItem *);
	void showCurvePlotDialog();
	void showCurveWorksheet();
    void showCurveWorksheet(Graph *g, int curveIndex);
	void showWindowPopupMenu(const QPoint &pos);

	//! Connected to the context menu signal from lv; it's called when there are several items selected in the list
	void showListViewSelectionMenu(const QPoint &p);

	//! Connected to the context menu signal from lv; it's called when there are no items selected in the list
	void showListViewPopupMenu(const QPoint &p);

	void showScriptWindow(bool parent = true);
	void showMoreWindows();
	void showMarkerPopupMenu();
	void showPlotWizard();
	void showFitPolynomDialog();
    void showFrequencyCountDialog();
	void showFunctionIntegrationDialog();
	void showInterpolationDialog();
	void showExpGrowthDialog();
	void showExpDecayDialog();
	void showExpDecayDialog(int type);
	void showTwoExpDecayDialog();
	void showExpDecay3Dialog();
	void showRowStatistics();
	void showColStatistics();
	void showFitDialog();
	void showLayerDialog();
	void showPreferencesDialog();
	void showMatrixDialog();
	void showMatrixSizeDialog();
	void showMatrixValuesDialog();
	void showSmoothSavGolDialog();
	void showSmoothFFTDialog();
	void showSmoothAverageDialog();
	void showSmoothLowessDialog();
    void showSmoothDialog(int m);
	void showFilterDialog(int filter);
	void lowPassFilterDialog();
	void highPassFilterDialog();
	void bandPassFilterDialog();
	void bandBlockFilterDialog();
	void showFFTDialog();
	void showColorMapDialog();
	//@}

	void baselineDialog();
	void subtractReferenceData();
	void subtractStraightLine();
	void translateCurveHor();
	void translateCurve(TranslateCurveTool::Direction direction = TranslateCurveTool::Vertical);

	//! Removes the curve identified by a key stored in the data() of actionRemoveCurve.
	void removeCurve();
	void hideCurve();
	void hideOtherCurves();
	void showAllCurves();
	void setCurveFullRange();

	void setAscValues();
	void setRandomValues();
	void setNormalRandomValues();
	void setXCol();
	void setYCol();
	void setZCol();
	void setXErrCol();
	void setYErrCol();
	void setLabelCol();
	void disregardCol();
	void setReadOnlyCol();
	void setReadOnlyColumns();
	void setReadWriteColumns();
	void swapColumns();
	void moveColumnRight();
	void moveColumnLeft();
	void moveColumnFirst();
	void moveColumnLast();

	void updateConfirmOptions(bool askTables, bool askMatrixes, bool askPlots2D, bool askPlots3D, bool askNotes);

	//! \name Plot3D Tools
	//@{
	void toggle3DAnimation(bool on = true);
	 //! Turns perspective mode on or off
  	void togglePerspective(bool on = true);
  	//! Resets rotation of 3D plots to default values
  	void resetRotation();
  	//! Finds best layout for the 3D plot
  	void fitFrameToLayer();
	void setFramed3DPlot();
	void setBoxed3DPlot();
	void removeAxes3DPlot();
	void removeGrid3DPlot();
	void setHiddenLineGrid3DPlot();
	void setLineGrid3DPlot();
	void setPoints3DPlot();
	void setCrosses3DPlot();
	void setCones3DPlot();
	void setBars3DPlot();
	void setFilledMesh3DPlot();
	void setEmptyFloor3DPlot();
	void setFloorData3DPlot();
	void setFloorIso3DPlot();
	void setFloorGrid3DPlot(bool on);
	void setCeilGrid3DPlot(bool on);
	void setRightGrid3DPlot(bool on);
	void setLeftGrid3DPlot(bool on);
	void setFrontGrid3DPlot(bool on);
	void setBackGrid3DPlot(bool on);
	void pickPlotStyle( QAction* action );
	void pickCoordSystem( QAction* action);
	void pickFloorStyle( QAction* action);
	void custom3DActions(QMdiSubWindow *w);
	void custom3DGrids(int grids);
	//@}

    void updateRecentProjectsList(const QString &fn = QString());

	//! Open support page in external browser
	void showSupportPage();
	//! Open donation page in external browser
	void showDonationsPage();
	//! Open QtiPlot homepage in external browser
	void showHomePage();
	//! Show download page in external browser
	void downloadManual();
//+++
    void downloadQtisasZip();
//---
	//! Show translations page in external browser
	void downloadTranslation();
	//! Shown when the user tries to save the project.
	void showProVersionMessage();

	void parseCommandLineArguments(const QStringList& args);
	void switchToLanguage(int param);
	void switchToLanguage(const QString& locale);

	bool alreadyUsedName(const QString& label);
	bool projectHas2DPlots();

	//! Returns a pointer to the window named/labelled "name" depending on "label" value
	MdiSubWindow* window(const QString& name, bool label = false);

	//! Returns a list with the names of all the matrices in the project
	QStringList matrixNames();

	//! \name Notes
	//@{
 	//! Creates a new empty note window
	Note* newNote(const QString& caption = QString());
    Note* newNoteText(const QString& caption, QString text);
	Note* openNote(ApplicationWindow* app, const QStringList &flist);
	void saveNoteAs();
	void showNoteLineNumbers(bool show = true);
	void increaseNoteIndent();
	void decreaseNoteIndent();
	void noteFindDialogue();
	void noteFindNext();
	void noteFindPrev();
	void noteReplaceDialogue();
	void renameCurrentNoteTab();
	void addNoteTab();
	void closeNoteTab();
	void executeAll();
	void commentSelection();
	void uncommentSelection();
	//@}

	//! \name Folders
	//@{
	//! Returns a pointer to the current folder in the project
	Folder* currentFolder(){return current_folder;};
	//! Adds a new folder to the project
	void addFolder();
	Folder* addFolder(QString name, Folder* parent = nullptr);
	//! Deletes the current folder
	void deleteFolder();

	//! Ask confirmation from user, deletes the folder f if user confirms and returns true, otherwise returns false;
	bool deleteFolder(Folder *f);

	//! Deletes the currently selected items from the list view #lv.
	void deleteSelectedItems();
	//! Hides the currently selected windows from the list view #lv.
	void hideSelectedWindows();
    //! Show the currently selected windows from the list view #lv.
	void showSelectedWindows();

	//! Sets all items in the folders list view to be desactivated (QPixmap = folder_closed_xpm)
	void desactivateFolders();

	//! Changes the current folder. Returns true if successfull
	bool changeFolder(Folder *newFolder, bool force = false);
    bool changeFolder(const QString &name, bool updateIfExist = false);
    
	//! Changes the current folder when the user changes the current item in the QListView "folders"
	void folderItemChanged(QTreeWidgetItem *it, QTreeWidgetItem *);
	//! Changes the current folder when the user double-clicks on a folder item in the QListView "lv"
	void folderItemDoubleClicked(QTreeWidgetItem *it);

	//!  creates and opens the context menu of a folder list view item
	/**
	 * \param it list view item
	 * \param p mouse global position
	 * \param fromFolders: true means that the user clicked right mouse buttom on an item from QListView "folders"
	 *					   false means that the user clicked right mouse buttom on an item from QListView "lv"
	 */
	void showFolderPopupMenu(const QPoint &pos, bool fromFolders = true);

	//!  starts renaming the selected folder by creating a built-in text editor
	void startRenameFolder();

	//!  starts renaming the selected folder by creating a built-in text editor
	void startRenameFolder(QTreeWidgetItem *item);

    //!  checks weather the new folder name is valid and modifies the name
    void renameFolder(QTreeWidgetItem *it, int col);

	//!  forces showing all windows in the current folder and subfolders, depending on the user's viewing policy
	void showAllFolderWindows();

	//!  forces hidding all windows in the current folder and subfolders, depending on the user's viewing policy
	void hideAllFolderWindows();

	//!  hides all windows in folder f
	void hideFolderWindows(Folder *f);

	//!  pops up folder information
	void folderProperties();

	//!  pops up information about the selected window item
	void windowProperties();

	//!  pops up information about the current project
	void projectProperties();

	//! Pops up a file dialog and invokes appendProject(const QString&) on the result.
	void appendProject();
	//! Open the specified project file and add it as a subfolder to the parentFolder or to the current folder if no parent folder is specified.
	Folder* appendProject(const QString& file_name, Folder* parentFolder = 0);
	void saveAsProject();
	void saveFolderAsProject(Folder *f);
	void saveFolder(Folder *folder, const QString& fn, bool compress = false);

	//!  adds a folder list item to the list view "lv"
	void addFolderListViewItem(Folder *f);

	//!  adds a widget list item to the list view "lv"
	void addListViewItem(MdiSubWindow *w);

    //!  hides or shows windows in the current folder and changes the view windows policy
    void setShowWindowsPolicy(QAction *triggeredAction);

	//!  returns a pointer to the root project folder
	Folder* projectFolder();

	//!  used by the findDialog
	void find(const QString& s, bool windowNames, bool labels, bool folderNames,
			  bool caseSensitive, bool partialMatch, bool subfolders);

	//!  initializes the list of items dragged by the user
	void dragFolderItems(QList<QTreeWidgetItem *> items){draggedItems = items;};

	//!  Drop the objects in the list draggedItems to the folder of the destination item
	void dropFolderItems(QTreeWidgetItem *dest);

	//!  moves a folder item to another
	/**
	 * \param src source folder item
	 * \param dest destination folder item
	 */
	void moveFolder(FolderListItem *src, FolderListItem *dest);
	//!  copies a folder to another
	/**
	 * \param src source folder
	 * \param dest destination folder
     */
	bool copyFolder(Folder *src, Folder *dest);

	void foldersMenuActivated(bool id);
	//@}

	//! \name Scripting
	//@{
	//! notify the user that an error occured in the scripting system
	void scriptError(const QString &message, const QString &scriptName, int lineNumber);
	//! execute all notes marked auto-exec
	void executeNotes();
	//! show scripting language selection dialog
	void showScriptingLangDialog();
	//! create a new environment for the current scripting language
	void restartScriptingEnv();
	//! print to scripting console (if available) or to stdout
	void scriptPrint(const QString &text);
	//! switches to the given scripting language; if this is the same as the current one and force is true, restart it
	bool setScriptingLanguage(const QString &lang, bool force=false);

	void scriptsDirPathChanged(const QString& path);
	//@}

	void showToolBarsMenu();
	void setFormatBarFont(const QFont &);
	void setFormatBarColor(const QColor&);

    //+++
    void bringToFront( QDockWidget* dockIn, QAction* action );
#ifdef QTISAS
    void showAscii1dDialog();
    void showJnseDialog();
    void showSvdDialog();
    void showDanDialog();
    void showFittableDialog();
    void eFitAction(QAction*);
    void showCompileDialog();
#endif
    void showExplorerDialog();
    void showLogDialog();
    //---
signals:
	void modified();

private:
	//! \name Initialization
	//@{
	void init(bool factorySettings = false);
	void initCompleter();
	void createActions();
	void initMainMenu();
	void initToolBars();
	void initPlot3DToolBar();
	void initPlot3D(Graph3D *plot);
	void insertTranslatedStrings();
	void translateActionsStrings();
	//@}
    QMenu *createPopupMenu() override
    {
        return nullptr;
    };
    void updateCompleter(const QString &windowName, bool remove = false, const QString &newName = QString());
	QMessageBox::StandardButton showSaveProjectMessage();
	QString getSaveProjectName(const QString& fileName, bool *compress = 0, int scope = 0);
	void goToParentFolder();
	bool isProjectFile(const QString& fn);


private slots:
    //+++
    void copyStatusBarText(); //Copy the status bar text to the clipboard 2023
    void setStatusBarTextDebugInfo(QString text);
    
    void terminal();
    void terminal(QString str);
    void removeWindows(QString pattern); //2023
    void renameWindows(QString pattern); //2023
    void radialAveragingMatrix(QString pattern); //2023
    void radUniHF    (
                      int chanellNumberX,int chanellNumberY,
                      gsl_matrix *Sample, gsl_matrix *SampleErr,  gsl_matrix *mask,
                      double XYcenter, double YXcenter,
                      QString sampleMatrix,
                      QString label, double qScale, double iScale, bool normalizeYN
                      );
    QString matrixCalculator(QString script);
    //---
	void addColumnNameToCompleter(const QString& colName, bool remove = false);
	void removeColumnNameFromCompleter(const QString& colName){addColumnNameToCompleter(colName, true);};

	void disableActions();
	void customColumnActions();
	void disableToolbars();
	void customToolBars(QMdiSubWindow* w);
	void customMenu(QMdiSubWindow* w);
	void windowActivated(QMdiSubWindow *w);

	void custom2DPlotTools(MultiLayer *);
	void updateExplorerWindowLayout(Qt::DockWidgetArea);

	void analysisMenuAboutToShow();
	void scriptingMenuAboutToShow();
	void fileMenuAboutToShow();
	void editMenuAboutToShow();
	void matrixMenuAboutToShow();
	void plotMenuAboutToShow();
	void plotDataMenuAboutToShow();
	void tableMenuAboutToShow();
	void windowsMenuAboutToShow();
	void windowsMenuActivated(bool id);

    //+++
	void qtisasMenuAboutToShow();
    //---
    
	//! \name Font Format Functions
	//@{
	void enableTextEditor(Graph *g);
	void setFontSize(int);
	void setFontFamily(const QFont &);
	void setItalicFont(bool);
	void setBoldFont(bool);
    void insertSuperscript();
    void insertSubscript();
    void underline();
    void insertGreekSymbol();
    void insertGreekMajSymbol();
    void insertMathSymbol();
    //+++
    void insertUnicodeSymbol();
    //---
	void setTextColor();

	//@}

	void showCustomActionDialog();
	void performCustomAction(QAction *);

	void hideSelectedColumns();
	void showAllColumns();
	void closedLastCopiedLayer(){lastCopiedLayer = nullptr;};

	void increasePrecision();
	void decreasePrecision();

#ifdef SCRIPTING_PYTHON
    void openQtDesignerUi();
	void executeStartupScripts();
#endif

// TODO: a lot of this stuff should be private
public:
    
    //+++
	int sasFontIncrement;
    QString sasPath;
    int screenResoHight;
    double sasResoScale;
    int sasDefaultInterface;
    int currentColorMap;
    QStringList colorMapList;
    
    QString magicTemplateFile;
    QStringList magicList;
    
    QString imageFormat;
    int imageRes;
    //---
    
	bool d_fft_norm_amp;
	bool d_fft_shift_res;
	bool d_fft_power2;
	bool d_int_sort_data;
	bool d_int_show_plot;
	bool d_int_results_table;
	bool d_show_empty_cell_gap;
	bool d_show_table_paste_dialog;
	double d_stats_significance_level;
	bool d_stats_result_table;
	bool d_stats_result_log;
	bool d_stats_result_notes;
	bool d_stats_confidence;
	bool d_stats_power;
	bool d_stats_output;
	bool d_descriptive_stats;
	bool d_confirm_modif_2D_points;
	bool d_ask_web_connection;
	bool d_open_last_project;
	int d_curve_max_antialising_size;
	bool d_disable_curve_antialiasing;
	bool d_force_muParser;
	bool d_keep_aspect_ration;
    QPageSize::PageSizeId d_print_paper_size;
    QPageLayout::Orientation d_printer_orientation;
	bool d_muparser_c_locale;
	Graph::LegendDisplayMode d_graph_legend_display;
	Graph::AxisTitlePolicy d_graph_axis_labeling;
	int d_graph_attach_policy;
	bool d_synchronize_graph_scales;
	int d_latex_compiler;
	QString d_latex_compiler_path;
	//! Last selected filter in open project dialog
    QString d_open_project_filter;
	//! Default geometry unit to be displayed in the EnrichmentDialog.
	int d_frame_geometry_unit;
	//! Default geometry unit to be displayed in the PlotDialog.
	int d_layer_geometry_unit;
	int d_layer_canvas_width, d_layer_canvas_height;
    int d_notes_tab_length;
    QFont d_notes_font;
	//! End of line convention used for copy/paste operations and when exporting tables/matrices to ASCII files.
	EndLineChar d_eol;
    //! Flag telling if the in-place editing of 2D plot labels is enabled
    bool d_in_place_editing;
    //! Flag telling if completion hints are enabled
    bool d_completion;
    //! Flag telling if line numbers are displayed in note windows
    bool d_note_line_numbers;
	QString d_python_config_folder;
	//! Scripts in this folder are executed at startup if default script language is Python
	QString d_startup_scripts_folder;
	//! Flag telling if the application is opening a project file or not
	bool d_opening_file;
	//! Flag telling if the application is appending a project file or not
	bool d_is_appending_file;
    QString customActionsDirPath;
	bool d_matrix_tool_bar, d_file_tool_bar, d_table_tool_bar, d_column_tool_bar, d_edit_tool_bar;
	bool d_plot_tool_bar, d_plot3D_tool_bar, d_display_tool_bar, d_format_tool_bar, d_notes_tool_bar;
	bool d_backup_files;
	WindowType d_init_window_type;
	QRect d_script_win_rect, d_app_rect;
	bool d_script_win_on_top;
	bool d_inform_rename_table;
	QString d_export_col_separator;
	bool d_export_col_names, d_export_table_selection, d_export_col_comment;
    //! Last selected filter in export image dialog
    QString d_image_export_filter, d_export_ASCII_file_filter;
    double d_scale_fonts_factor;
    bool d_export_transparency;
	int d_export_quality, d_export_compression;
    int d_export_vector_resolution, d_export_bitmap_resolution, d_export_size_unit;
    QSizeF d_export_raster_size;
    bool d_export_color;
    bool d_export_escape_tex_strings;
    bool d_export_tex_font_sizes;
	int d_3D_export_text_mode, d_3D_export_sort;
	//! Locale used to specify the decimal separators in imported ASCII files
	QLocale d_clipboard_locale;
	//! Locale used to specify the decimal separators in imported ASCII files
	QLocale d_ASCII_import_locale;
	//! End of line convention used to import ASCII files.
	EndLineChar d_ASCII_end_line;
    //! Last selected filter in import ASCII dialog
    QString d_ASCII_file_filter, d_ASCII_comment_string;
	bool d_ASCII_import_comments, d_ASCII_import_read_only, d_ASCII_import_preview;
	int d_ASCII_import_mode, d_preview_lines;
	int d_ASCII_import_first_row_role;
	QSize d_import_ASCII_dialog_size;
	//! Specifies if only the Tables/Matrices in the current folder should be displayed in the Add/remove curve dialog.
	bool d_show_current_folder;
	bool d_scale_plots_on_print, d_print_cropmarks;
	bool d_show_table_comments;
	bool d_extended_plot_dialog;
	bool d_extended_import_ASCII_dialog;
	bool d_extended_export_dialog;
	bool d_extended_open_dialog;
	bool generateUniformFitPoints;
	bool generatePeakCurves;
	QColor peakCurvesColor;
	bool d_multi_peak_messages;
	//! User defined size for the Add/Remove curves dialog
	QSize d_add_curves_dialog_size;

	//! Scale the errors output in fit operations with reduced chi^2
	bool fit_scale_errors;

	//! Number of points in a generated fit curve
	int fitPoints;

	//! Calculate only 2 points in a generated linear fit function curve
	bool d_2_linear_fit_points;

	bool pasteFitResultsToPlot;

	//! Write fit output information to Result Log
	bool writeFitResultsToLog;

	//! precision used for the output of the fit operations
	int fit_output_precision;

	//! default precision to be used for all other operations than fitting
	int d_decimal_digits;

	//! pointer to the current folder in the project
	Folder *current_folder;
	//! Describes which windows are shown when the folder becomes the current folder
	ShowWindowsPolicy show_windows_policy;
	enum {MaxRecentProjects = 10};
	//! File version code used when opening project files (= maj * 100 + min * 10 + patch)
	int d_file_version;

	Grid *d_default_2D_grid;

	QColor d_rect_default_background;
	QBrush d_rect_default_brush;
	QColor workspaceColor, panelsColor, panelsTextColor;
	QColor d_graph_background_color, d_graph_canvas_color, d_graph_border_color, d_canvas_frame_color;
	int d_graph_background_opacity, d_graph_canvas_opacity, d_graph_border_width;
	QString appStyle, workingDir;

	//! Path to the folder where the last template file was opened/saved
	QString templatesDir;
	bool autoScaleFonts, autoResizeLayers;
	bool confirmCloseTable, confirmCloseMatrix, confirmClosePlot2D, confirmClosePlot3D;
	bool confirmCloseFolder, confirmCloseNotes, d_confirm_overwrite;
	bool titleOn, autoSave, autoscale2DPlots, antialiasing2DPlots;
	//! \name User preferences for axes in new 2D plot layers
	//@{
	//! Axes to be shown
	QVector<bool> d_show_axes;
	//! Axes with labels enabled
	QVector<bool> d_show_axes_labels;
	//! Draw axes backbones
	bool drawBackbones;
	//! Default distance between the ticks and the tick labels
	int d_graph_tick_labels_dist;
	//! Distance between the axes title and the backbone
	int d_graph_axes_labels_dist;
	//! Default axes linewidth
	int axesLineWidth;
	//@}
	int majTicksStyle, minTicksStyle, legendFrameStyle, autoSaveTime, canvasFrameWidth;
	QColor legendBackground, legendTextColor, defaultArrowColor;
	int defaultArrowHeadLength, defaultArrowHeadAngle, d_legend_default_angle, d_symbol_style;
    double defaultArrowLineWidth, defaultSymbolEdge;
    float defaultCurveLineWidth;
	bool defaultArrowHeadFill, d_fill_symbols, d_indexed_symbols;
	int defaultCurveBrush, defaultCurveAlpha, d_curve_line_style;
	Qt::PenStyle defaultArrowLineStyle;
	QPen d_frame_widget_pen;
	int majTicksLength, minTicksLength, defaultPlotMargin;
	int defaultCurveStyle, defaultSymbolSize;
	QFont appFont;
	QFont tableTextFont, tableHeaderFont, plotAxesFont, plotLegendFont, plotNumbersFont, plotTitleFont;
	QColor tableBkgdColor, tableTextColor, tableHeaderColor;
//+++ Table Header Bgd Colour
    QColor tableHeaderColorRows;
//---
	QString projectname,columnSeparator, appLanguage;
	QString configFilePath, fitPluginsPath, fitModelsPath, asciiDirPath, imagesDirPath, scriptsDirPath;
	int ignoredLines, savingTimerId, recentMenuID;
	bool renameColumns, strip_spaces, simplify_spaces;
	QStringList recentProjects;
	bool saved;
	QStringList locales;
	QStringList d_recent_functions; //user-defined functions;
	QStringList xFunctions, yFunctions, rFunctions, thetaFunctions; // user functions for parametric and polar plots
	QStringList surfaceFunc; //user-defined surface functions;
    QStringList d_param_surface_func; //user-defined parametric surface functions;
	//! List of tables and matrices renamed in order to avoid conflicts when appending a project to a folder
	QStringList renamedTables;

	//! \name variables used when user copy/paste markers
	//@{
	FrameWidget *d_enrichement_copy;
	ArrowMarker *d_arrow_copy;
	//@}


	//! The scripting language to use for new projects.
	QString defaultScriptingLang;

	//! User custom colors used for Python syntax highlighting
	QColor d_comment_highlight_color, d_class_highlight_color, d_numeric_highlight_color;
	QColor d_keyword_highlight_color, d_function_highlight_color, d_quotation_highlight_color;

	//! \name user custom values for 3D plots look
	//@{
	bool d_3D_smooth_mesh, d_3D_legend, d_3D_orthogonal, d_3D_autoscale;
	int d_3D_resolution, d_3D_projection;
	QFont d_3D_title_font, d_3D_numbers_font, d_3D_axes_font;
	LinearColorMap d_3D_color_map;
	QColor d_3D_mesh_color;
	QColor d_3D_axes_color;
	QColor d_3D_numbers_color;
	QColor d_3D_labels_color;
	QColor d_3D_background_color;
	QColor d_3D_grid_color, d_3D_minor_grid_color;
	bool d_3D_minor_grids, d_3D_major_grids;
	int d_3D_major_style, d_3D_minor_style;
	double d_3D_major_width, d_3D_minor_width;

    void setPlot3DOptions();
	//@}
	//+++
	QTextEdit *results;
	//---
    
//+++//
#ifdef QTISAS
    ascii1d18    *ascii1dWidget;
    QAction      *actionShowAscii1d;
    jnse18        *jnseWidget;
    QAction     *actionShowJnse;
    svd		*svdWidget;
    QAction 	*actionShowSvd;
    dan18		    *danWidget;
    QAction 	*actionShowDan;
    compile18	*compileWidget;
    QAction 	*actionShowCompile;
    fittable18	*fittableWidget;
    QAction 	*actionShowFittable;
#endif
//---//
        QLineEdit *info;
private:
	void loadPlugins();
	QList<ImportExportPlugin *> d_import_export_plugins;

	bool d_mdi_windows_area;
	MdiSubWindow *d_active_window;
	TextEditor *d_text_editor;
	// Flag telling if table values should be automatically recalculated when values in a column are modified.
	bool d_auto_update_table_values;
	int d_matrix_undo_stack_size;

	//! Workaround for the new colors introduced in rev 447
	int convertOldToNewColorIndex(int cindex);

	//! Stores the pointers to the dragged items from the FolderListViews objects
	QList<QTreeWidgetItem *> draggedItems;

	Graph *lastCopiedLayer;
	QSplitter *explorerSplitter;

	ScriptWindow *scriptWindow;
	QTranslator *appTranslator, *qtTranslator;
	QDockWidget *explorerWindow, *undoStackWindow;
//+++	QTextEdit *results;
#ifdef SCRIPTING_CONSOLE
	QDockWidget *consoleWindow;
	QTextEdit *console;
#endif
	QMdiArea *d_workspace;

	QToolBar *fileTools, *plotTools, *tableTools, *columnTools, *plot3DTools, *displayBar, *editTools, *plotMatrixBar;
	QToolBar *formatToolBar, *noteTools;
	QToolButton *btnResults;
	QWidgetList *hiddenWindows;
	//! Completer used in notes and in the script window
	QCompleter *d_completer;

	QMenu *windowsMenu, *foldersMenu, *view, *graphMenu, *fileMenu, *format, *edit, *recent;
//+++//
	QMenu 		*qtisasMenu;
	QToolBar 	*qtisasToolBar;
//---//
    

	QMenu *help, *plot2DMenu, *analysisMenu, *multiPeakMenu;
	QMenu *matrixMenu, *plot3DMenu, *plotDataMenu, *tablesDepend, *scriptingMenu;
	QMenu *tableMenu, *fillMenu, *normMenu, *newMenu, *exportPlotMenu, *smoothMenu, *filterMenu, *decayMenu, *importMenu;

	QAction *actionEditCurveRange, *actionCurveFullRange, *actionShowAllCurves, *actionHideCurve, *actionHideOtherCurves;
	QAction *actionRemoveCurve, *actionShowCurveWorksheet, *actionShowCurvePlotDialog;
	QAction *actionNewProject, *actionAppendProject, *actionNewNote, *actionNewTable, *actionNewFunctionPlot;
	QAction *actionNewSurfacePlot, *actionNewMatrix, *actionNewGraph, *actionNewFolder;
	QAction *actionOpen, *actionLoadImage, *actionSaveProject, *actionSaveProjectAs, *actionImportImage;
	QAction *actionLoad, *actionUndo, *actionRedo, *actionImportSound;
	QAction *actionImportDatabase;
	QAction *actionCopyWindow, *actionShowAllColumns, *actionHideSelectedColumns;
	QAction *actionCutSelection, *actionCopySelection, *actionPasteSelection, *actionClearSelection;
	QAction *actionShowExplorer, *actionShowLog, *actionAddLayer, *actionShowLayerDialog, *actionAutomaticLayout;
//+++
    QAction *actionLogLog, *actionLinLin;
//---
#ifdef SCRIPTING_CONSOLE
	QAction *actionShowConsole;
#endif
#ifdef SCRIPTING_PYTHON
	QAction *actionOpenQtDesignerUi, *actionCommentSelection, *actionUncommentSelection;
#endif
	QAction *actionSwapColumns, *actionMoveColRight, *actionMoveColLeft, *actionMoveColFirst, *actionMoveColLast, *actionShowScriptWindow;
    QAction *actionExportGraph, *actionExportLayer, *actionExportAllGraphs, *actionPrint, *actionPrintAllPlots, *actionShowExportASCIIDialog;//+++ new: actionExportLayer
	QAction *actionExportPDF, *actionReadOnlyCol, *actionStemPlot;
	QAction *actionCloseAllWindows, *actionCloseProject, *actionClearLogInfo, *actionShowPlotWizard, *actionShowConfigureDialog;
	QAction *actionShowCurvesDialog, *actionAddErrorBars, *actionAddFunctionCurve, *actionUnzoom, *actionNewLegend, *actionAddImage, *actionAddText;
	QAction *actionPlotL, *actionPlotP, *actionPlotLP, *actionPlotVerticalDropLines, *actionPlotSpline;
	QAction *actionPlotVertSteps, *actionPlotHorSteps, *actionPlotVerticalBars, *actionStackBars, *actionStackColumns;
	QAction *actionPlotHorizontalBars, *actionPlotArea, *actionPlotPie, *actionPlotVectXYAM, *actionPlotVectXYXY;
	QAction *actionPlotHistogram, *actionPlotStackedHistograms, *actionPlot2VerticalLayers, *actionPlot2HorizontalLayers, *actionPlot4Layers, *actionPlotStackedLayers;
	QAction *actionPlot3DRibbon, *actionPlot3DBars, *actionPlot3DScatter, *actionPlot3DTrajectory;
	QAction *actionPlotDoubleYAxis, *actionAddInsetLayer, *actionAddInsetCurveLayer;
	QAction *actionShowColStatistics, *actionShowRowStatistics, *actionShowIntDialog, *actionIntegrate;
	QAction *actionDifferentiate, *actionFitLinear, *actionFitSlope, *actionShowFitPolynomDialog;

	QAction *actionShowExpDecayDialog, *actionShowTwoExpDecayDialog, *actionShowExpDecay3Dialog;
	QAction *actionFitExpGrowth, *actionFitSigmoidal, *actionFitGauss, *actionFitLorentz, *actionShowFitDialog;
	QAction *actionShowAxisDialog, *actionShowTitleDialog;
	QAction *actionShowColumnOptionsDialog, *actionShowColumnValuesDialog, *actionShowColsDialog, *actionShowRowsDialog;
	QAction *actionTableRecalculate, *actionExtractGraphs, *actionExtractLayers;
    QAction *actionAbout, *actionAboutQt;
	QAction *actionRename, *actionCloseWindow;
	QAction *actionConvertTableDirect, *actionConvertTableBinning, *actionConvertTableRegularXYZ;

	QAction *actionConvertTableRandomXYZ, *actionExpandMatrix, *actionShrinkMatrix, *actionSmoothMatrix;

	QAction *actionAddColToTable, *actionDeleteLayer, *actionInterpolate;
	QAction *actionResizeActiveWindow, *actionHideActiveWindow;
	QAction *actionShowMoreWindows, *actionPixelLineProfile, *actionIntensityTable;
	QAction *actionShowLineDialog, *actionShowTextDialog;
	QAction *actionActivateWindow, *actionMinimizeWindow, *actionMaximizeWindow, *actionHideWindow, *actionResizeWindow;
	QAction *actionEditSurfacePlot, *actionAdd3DData;
	QAction *actionMatrixDeterminant, *actionSetMatrixProperties, *actionConvertMatrixXYZ, *actionConvertMatrixYXZ;
	QAction *actionSetMatrixDimensions, *actionConvertMatrixDirect, *actionSetMatrixValues, *actionTransposeMatrix, *actionInvertMatrix;
	QAction *actionPlot3DWireFrame, *actionPlot3DHiddenLine, *actionPlot3DPolygons, *actionPlot3DWireSurface;
	QAction *actionColorMap, *actionContourMap, *actionGrayMap;
	QAction *actionDeleteFitTables, *actionShowGridDialog, *actionTimeStamp;
	QAction *actionSmoothSavGol, *actionSmoothFFT, *actionSmoothAverage, *actionSmoothLowess, *actionFFT;
	QAction *actionLowPassFilter, *actionHighPassFilter, *actionBandPassFilter, *actionBandBlockFilter;
	QAction *actionSortTable, *actionSortSelection, *actionNormalizeSelection;
	QAction *actionNormalizeTable, *actionConvolute, *actionDeconvolute, *actionCorrelate, *actionAutoCorrelate;
	QAction *actionTranslateHor, *actionTranslateVert, *actionSetAscValues, *actionSetRandomValues, *actionSetRandomNormalValues;
	QAction *actionSetXCol, *actionSetYCol, *actionSetZCol, *actionSetLabelCol, *actionDisregardCol, *actionSetXErrCol, *actionSetYErrCol;
	QAction *actionBoxPlot, *actionMultiPeakGauss, *actionMultiPeakLorentz;
	QAction *actionHomePage, *actionDownloadManual;
//+++
    QAction *actionDownloadQtisasZip;
    QAction *actionSaveGraphAsProject; //+++ 2020.04
//---
	QAction *actionShowPlotDialog, *actionShowScaleDialog, *actionOpenTemplate, *actionSaveTemplate, *actionSaveWindow;
	QAction *actionNextWindow, *actionPrevWindow;
	QAction *actionScriptingLang, *actionRestartScripting, *actionClearTable, *actionGoToRow, *actionGoToColumn;
    QAction *actionNoteExecuteAll, *actionSaveNote, *actionFrequencyCount;
	QAction *actionOneSampletTest, *actionTwoSampletTest, *actionShapiroWilk, *actionChiSquareTest;
	QAction *actionAnimate, *actionPerspective, *actionFitFrame, *actionResetRotation;
	QAction *actionDeleteRows, *actionDrawPoints, *actionAddZoomPlot;
	QAction *btnCursor, *btnSelect, *btnPicker, *btnRemovePoints, *btnMovePoints, *actionDragCurve;
	QAction *btnZoomIn, *btnZoomOut, *btnPointer, *btnLine, *btnArrow;
	QAction *actionFlipMatrixVertically, *actionFlipMatrixHorizontally, *actionRotateMatrix;
	QAction *actionViewMatrixImage, *actionViewMatrix, *actionExportMatrix, *actionMatrixDefaultScale;
	QAction *actionMatrixGrayScale, *actionMatrixRainbowScale, *actionMatrixCustomScale, *actionRotateMatrixMinus;
	QAction *actionMatrixXY, *actionMatrixColumnRow, *actionImagePlot, *actionToolBars, *actionImageProfilesPlot;
	QAction *actionMatrixFFTDirect, *actionMatrixFFTInverse;
	QAction *actionFontBold, *actionFontItalic, *actionFontBox, *actionFontSize, *actionTextColor;
	QAction *actionSuperscript, *actionSubscript, *actionUnderline, *actionGreekSymbol, *actionCustomActionDialog;
//+++
    QAction *actionUnicodeSymbol;
//---
	QAction *actionGreekMajSymbol, *actionMathSymbol;
	QAction *Box, *Frame, *None;
	QAction *front, *back, *right, *left, *ceil, *floor, *floordata, *flooriso, *floornone;
	QAction *wireframe, *hiddenline, *polygon, *filledmesh, *pointstyle, *barstyle, *conestyle, *crossHairStyle;
	QAction *actionShowUndoStack, *actionShowNoteLineNumbers, *actionAddFormula, *actionAddRectangle, *actionAddEllipse;
	QActionGroup *coord, *floorstyle, *grids, *plotstyle, *dataTools;
	QAction *actionMagnify, *actionFindWindow, *actionWaterfallPlot, *actionMagnifyHor, *actionMagnifyVert;
	QAction *actionMoveRowUp, *actionMoveRowDown, *actionAdjustColumnWidth;

	QAction *actionRenameNoteTab, *actionAddNoteTab, *actionCloseNoteTab;
	QAction *actionIncreaseIndent, *actionDecreaseIndent, *actionFind, *actionFindNext, *actionFindPrev, *actionReplace;
	QAction *actionIncreasePrecision, *actionDecreasePrecision, *actionPrintPreview;
	QAction *actionVertSharedAxisLayers, *actionHorSharedAxisLayers, *actionSharedAxesLayers, *actionStackSharedAxisLayers;
	QAction *actionCustomSharedAxisLayers, *actionCustomLayout, *actionExtractTableData, *actionSubtractLine, *actionSubtractReference;
	QAction *actionRaiseEnrichment, *actionLowerEnrichment, *actionBaseline;
	QAction *actionAlignTop, *actionAlignBottom, *actionAlignLeft, *actionAlignRight;

	QList<QAction *> d_user_actions;
	QUndoView *d_undo_view;
	QList<QMenu *> d_user_menus;

	QList<QColor> d_indexed_colors;
	QStringList d_indexed_color_names;
	QList<int> d_symbols_list;

	QAction *actionOneWayANOVA, *actionTwoWayANOVA;

};

#endif
