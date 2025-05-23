/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Alex Kargovsky <kargovsky@yumr.phys.msu.su>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Add/remove curves dialog
 ******************************************************************************/

#include <QCheckBox>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QShortcut>

#include "ApplicationWindow.h"
#include "CurveRangeDialog.h"
#include "CurvesDialog.h"
#include "Folder.h"
#include "Graph.h"
#include "LegendWidget.h"
#include "Matrix.h"
#include "PlotCurve.h"
#include "QwtHistogram.h"
#include "Spectrogram.h"
#include "Table.h"


CurvesDialog::CurvesDialog( QWidget* parent, Qt::WindowFlags fl )
: QDialog( parent, fl )
{
    setObjectName( "CurvesDialog" );
	setWindowTitle( tr( "QtiSAS - Add/Remove curves" ) );
    setSizeGripEnabled(true);
	setFocus();

	QGridLayout *hl = new QGridLayout();

	hl->addWidget(new QLabel(tr("New curves style")), 0, 0);
	boxStyle = new QComboBox();
	boxStyle->addItem( QPixmap(":/lPlot.png"), tr( " Line" ) );
	boxStyle->addItem( QPixmap(":/pPlot.png"), tr( " Scatter" ) );
	boxStyle->addItem( QPixmap(":/lpPlot.png"), tr( " Line + Symbol" ) );
	boxStyle->addItem( QPixmap(":/dropLines.png"), tr( " Vertical drop lines" ) );
	boxStyle->addItem( QPixmap(":/spline.png"), tr( " Spline" ) );
	boxStyle->addItem( QPixmap(":/vert_steps.png"), tr( " Vertical steps" ) );
	boxStyle->addItem( QPixmap(":/hor_steps.png"), tr( " Horizontal steps" ) );
	boxStyle->addItem( QPixmap(":/area.png"), tr( " Area" ) );
	boxStyle->addItem( QPixmap(":/vertBars.png"), tr( " Vertical Bars" ) );
	boxStyle->addItem( QPixmap(":/hBars.png"), tr( " Horizontal Bars" ) );
	boxStyle->addItem( QPixmap(":/histogram.png"), tr( " Histogram" ) );
	hl->addWidget(boxStyle, 0, 1);

    boxMatrixStyle = new QComboBox();
	boxMatrixStyle->addItem( QPixmap(":/color_map.png"), tr("Contour - Color Fill"));
	boxMatrixStyle->addItem( QPixmap(":/contour_map.png"), tr("Contour Lines"));
	boxMatrixStyle->addItem( QPixmap(":/gray_map.png"), tr("Gray Scale Map"));
	boxMatrixStyle->addItem( QPixmap(":/histogram.png"), tr("Histogram"));
	hl->addWidget(boxMatrixStyle, 0, 2);
	hl->setColumnStretch(3, 1);

	hl->addWidget(new QLabel(tr("Axes")), 1, 0);
	boxXAxis = new QComboBox();
	boxXAxis->addItem(tr("Bottom"));
	boxXAxis->addItem(tr("Top"));
	hl->addWidget(boxXAxis, 1, 1);

	boxYAxis = new QComboBox();
	boxYAxis->addItem(tr("Left"));
	boxYAxis->addItem(tr("Right"));
	hl->addWidget(boxYAxis, 1, 2);

    QGridLayout *gl = new QGridLayout();
    
    QHBoxLayout *hb0 = new QHBoxLayout;
    hb0->addWidget(new QLabel(tr("Available data")), 0);

    //+++
    dataFilter = new QLineEdit(this);
    dataFilter->setText("*");
    connect( dataFilter, SIGNAL( textChanged (const QString&)), this,SLOT( filterAvailable()) );
    hb0->addWidget(dataFilter);
    //---
    
    gl->addLayout(hb0, 0, 0);

    
    QHBoxLayout *hbc = new QHBoxLayout;
    hbc->addWidget(new QLabel( tr( "Graph contents" )));

    btnUp = new QPushButton();
    btnUp->setIcon(QPixmap(":/arrow_up.png"));
    btnUp->setMaximumWidth(20);
    hbc->addWidget(btnUp);
	btnDown = new QPushButton();
	btnDown->setIcon(QPixmap(":/arrow_down.png"));
	btnDown->setMaximumWidth(20);
	hbc->addWidget(btnDown);
	hbc->addStretch();
	gl->addLayout(hbc, 0, 2);

	available = new QTreeWidget();
	available->setColumnCount(1);
	available->header()->hide();
    available->setIndentation(15);
	available->setSelectionMode (QAbstractItemView::ExtendedSelection);
    gl->addWidget(available, 1, 0);

    QVBoxLayout* vl1 = new QVBoxLayout();
	btnAdd = new QPushButton();
	btnAdd->setIcon(QIcon(":/next.png"));
	btnAdd->setFixedWidth (55);
	btnAdd->setFixedHeight (30);
    vl1->addWidget(btnAdd);
    //+++
    plusErr=new QCheckBox("+yErr");
    plusErr->setFixedWidth (55);
    vl1->addWidget(plusErr);
    //---
	btnRemove = new QPushButton();
	btnRemove->setIcon(QIcon(":/prev.png"));
	btnRemove->setFixedWidth (55);
	btnRemove->setFixedHeight(30);
    vl1->addWidget(btnRemove);
    vl1->addStretch();
    
    
    lin = new QPushButton();
    lin->setIcon(QIcon(":/lin-lin.png"));
    lin->setFixedWidth (55);
    lin->setFixedHeight(30);
    connect(lin, SIGNAL(clicked()),(ApplicationWindow *)this->parent(), SLOT(setLinLin()));
    vl1->addWidget(lin);
    
    log = new QPushButton();
    log->setIcon(QIcon(":/log-log.png"));
    log->setFixedWidth (55);
    log->setFixedHeight(30);
    connect(log, SIGNAL(clicked()),(ApplicationWindow *)this->parent(), SLOT(setLogLog()));
    vl1->addWidget(log);
    
    gl->addLayout(vl1, 1, 1);
	contents = new QListWidget();
	contents->setSelectionMode (QAbstractItemView::ExtendedSelection);
    gl->addWidget(contents, 1, 2);

    QVBoxLayout* vl2 = new QVBoxLayout();
	btnAssociations = new QPushButton(tr( "&Plot Associations..." ));
	btnAssociations->setEnabled(false);
    vl2->addWidget(btnAssociations);

	btnRange = new QPushButton(tr( "Edit &Range..." ));
	btnRange->setEnabled(false);
    vl2->addWidget(btnRange);

	btnEditFunction = new QPushButton(tr( "&Edit Function..." ));
	btnEditFunction->setEnabled(false);
    vl2->addWidget(btnEditFunction);

	btnOK = new QPushButton(tr( "OK" ));
	btnOK->setDefault( true );
    vl2->addWidget(btnOK);

	btnCancel = new QPushButton(tr( "Close" ));
    vl2->addWidget(btnCancel);

    boxShowRange = new QCheckBox(tr( "&Show Range" ));
    vl2->addWidget(boxShowRange);

    vl2->addStretch();
    gl->addLayout(vl2, 1, 3);

    QVBoxLayout* vl3 = new QVBoxLayout(this);
    vl3->addLayout(hl);
    vl3->addLayout(gl);

    
	boxShowCurrentFolder = new QCheckBox(tr("Show current &folder only" ));
	vl3->addWidget(boxShowCurrentFolder);

    init();

	connect(btnUp, SIGNAL(clicked()),this, SLOT(raiseCurve()));
	connect(btnDown, SIGNAL(clicked()),this, SLOT(shiftCurveBy()));

	connect(boxShowCurrentFolder, SIGNAL(toggled(bool)), this, SLOT(showCurrentFolder(bool)));
    connect(boxShowRange, SIGNAL(toggled(bool)), this, SLOT(showCurveRange(bool)));
	connect(btnRange, SIGNAL(clicked()),this, SLOT(showCurveRangeDialog()));
	connect(btnAssociations, SIGNAL(clicked()),this, SLOT(showPlotAssociations()));
	connect(btnEditFunction, SIGNAL(clicked()),this, SLOT(showFunctionDialog()));
	connect(btnAdd, SIGNAL(clicked()),this, SLOT(addCurves()));
	connect(btnRemove, SIGNAL(clicked()),this, SLOT(removeCurves()));
	connect(btnOK, SIGNAL(clicked()),this, SLOT(close()));
	connect(btnCancel, SIGNAL(clicked()),this, SLOT(close()));
	connect(contents, SIGNAL(currentRowChanged(int)), this, SLOT(showCurveBtn(int)));
    connect(contents, SIGNAL(itemSelectionChanged()), this, SLOT(enableContentsBtns()));
    connect(available, SIGNAL(itemSelectionChanged()), this, SLOT(enableAddBtn()));

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(removeCurves()));
    shortcut = new QShortcut(QKeySequence("-"), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(removeCurves()));
    shortcut = new QShortcut(QKeySequence("+"), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(addCurves()));
}

void CurvesDialog::showCurveBtn(int)
{
	QwtPlotItem *it = d_graph->plotItem(contents->currentRow());
	if (!it)
		return;

    if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram){
        btnEditFunction->setEnabled(false);
        btnAssociations->setEnabled(false);
        btnRange->setEnabled(false);
        return;
    }

    PlotCurve *c = (PlotCurve *)it;
    if (c){
		btnEditFunction->setEnabled(c->type() == Graph::Function);
		btnRange->setEnabled(c->type() != Graph::Function && c->type() != Graph::ErrorBars);
		btnAssociations->setEnabled(c->type() != Graph::Function);
    }
}

void CurvesDialog::showCurveRangeDialog()
{
	QList<QListWidgetItem *> lst = contents->selectedItems();
	QList<int> indexes;
	foreach(QListWidgetItem *it, lst)
		indexes << contents->row(it);

	CurveRangeDialog* crd = new CurveRangeDialog(this);
	crd->setCurvesToModify(d_graph, indexes);
	crd->exec();

	updateCurveRange();
}

void CurvesDialog::showPlotAssociations()
{
	int curve = contents->currentRow();
	if (curve < 0)
		curve = 0;

    ApplicationWindow *app = (ApplicationWindow *)this->parent();
    close();

    if (app)
        app->showPlotAssociations(curve);
}

void CurvesDialog::showFunctionDialog()
{
    ApplicationWindow *app = (ApplicationWindow *)this->parent();
    int currentRow = contents->currentRow();
    close();

    if (app)
        app->showFunctionDialog(d_graph, currentRow);
}

QSize CurvesDialog::sizeHint() const
{
	return QSize(700, 400);
}

void CurvesDialog::contextMenuEvent(QContextMenuEvent *e)
{
	QPoint pos = available->viewport()->mapFromGlobal(QCursor::pos());
	QRect rect = available->visualItemRect(available->currentItem());
	if (rect.contains(pos))
	{
       QList<QTreeWidgetItem *> lst = available->selectedItems();
       int count = 0;
	   foreach (QTreeWidgetItem *item, lst){
			if (item->type() == FolderItem)
				continue;

			count++;
	   }

	   if (!count)
		return;

	   QMenu contextMenu(this);
       if (count > 1)
	       contextMenu.addAction(tr("&Plot Selection"), this, SLOT(addCurves()));
       else if (count == 1)
	       contextMenu.addAction(tr("&Plot"), this, SLOT(addCurves()));
	   contextMenu.exec(QCursor::pos());
    }

	pos = contents->viewport()->mapFromGlobal(QCursor::pos());
	rect = contents->visualItemRect(contents->currentItem());
	if (rect.contains(pos))
	{
	   QMenu contextMenu(this);
       QList<QListWidgetItem *> lst = contents->selectedItems();
       if (lst.size() > 1)
	       contextMenu.addAction(tr("&Delete Selection"), this, SLOT(removeCurves()));
       else if (lst.size() == 1)
	       contextMenu.addAction(tr("&Delete Curve"), this, SLOT(removeCurves()));
	   contextMenu.exec(QCursor::pos());
    }

    e->accept();
}

void CurvesDialog::init()
{
    ApplicationWindow *app = (ApplicationWindow *)this->parent();
    if (app){
		bool currentFolderOnly = app->d_show_current_folder;
        boxShowCurrentFolder->setChecked(currentFolderOnly);
		showCurrentFolder(currentFolderOnly);

        int style = app->defaultCurveStyle;
        if (style == Graph::Line)
            boxStyle->setCurrentIndex(0);
        else if (style == Graph::Scatter)
            boxStyle->setCurrentIndex(1);
        else if (style == Graph::LineSymbols)
            boxStyle->setCurrentIndex(2);
        else if (style == Graph::VerticalDropLines)
            boxStyle->setCurrentIndex(3);
        else if (style == Graph::Spline)
            boxStyle->setCurrentIndex(4);
        else if (style == Graph::VerticalSteps)
            boxStyle->setCurrentIndex(5);
        else if (style == Graph::HorizontalSteps)
            boxStyle->setCurrentIndex(6);
        else if (style == Graph::Area)
            boxStyle->setCurrentIndex(7);
        else if (style == Graph::VerticalBars)
            boxStyle->setCurrentIndex(8);
        else if (style == Graph::HorizontalBars)
            boxStyle->setCurrentIndex(9);
    }

	if (!available->topLevelItemCount())
		btnAdd->setDisabled(true);
}

void CurvesDialog::setGraph(Graph *graph)
{
	d_graph = graph;
	contents->addItems(d_graph->plotItemsList());
	enableContentsBtns();
    enableAddBtn();
}

void CurvesDialog::addCurves()
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	if (!app) return;

	QApplication::setOverrideCursor(Qt::WaitCursor);

	int curves = d_graph->curveCount();
	bool updateLegend = false;
	LegendWidget *legend = d_graph->legend();
	if (legend)
    {
		updateLegend = legend->isAutoUpdateEnabled();
		legend->setAutoUpdate(false);
	}

	QStringList emptyColumns;
	QList<QTreeWidgetItem *> lst = available->selectedItems();

	foreach (QTreeWidgetItem *item, lst)
    {
		QString text = item->text(0);
        
		switch(item->type())
        {
			case ColumnItem:
			{
				Table *t = app->table(text);
				if (t && !addCurveFromTable(app, t, text)) emptyColumns << text;
                else if (plusErr->isChecked())
                {
                    QString name = text;
                    DataCurve *curve = d_graph->dataCurve(d_graph->curveIndex(name));
                    if (!curve) continue;
                    if (curve->xColumnName().isEmpty()) continue;
                    int direction = 1;
                    ErrorBarsCurve *er = nullptr;

                    QStringList yErrListAll= app->columnsList(Table::yErr);
                    QString errColumnName="";
                    for(int i=0; i<yErrListAll.count();i++) if (yErrListAll[i].left(yErrListAll[i].lastIndexOf("_"))==name.left(name.lastIndexOf("_")))
                    {
                        errColumnName=yErrListAll[i];
                        break;
                    }
                    if (errColumnName=="") continue;
                    
                    Table *errTable = app->table(errColumnName);
                    if (!errTable) continue;
                    if (errTable->isEmptyColumn(errTable->colIndex(errColumnName)))continue;
                    er = d_graph->addErrorBars(curve, errTable, errColumnName, direction, app->defaultCurveLineWidth, 0, curve->pen().color(),true,true, true);
                    
                }
			}
			break;

			case TableItem:
			{
				Table *t = app->table(text, true);
				if (!t) continue;

				QStringList lst = t->YColumns();
				for(int i = 0; i < lst.size(); i++){
					QString s = lst[i];
					if (!addCurveFromTable(app, t, s)) emptyColumns << s;
                    else if (plusErr->isChecked())
                    {
                        QString name = s;
                        DataCurve *curve = d_graph->dataCurve(d_graph->curveIndex(name));
                        if (!curve) continue;
                        if (curve->xColumnName().isEmpty()) continue;
                        int direction = 1;
                        ErrorBarsCurve *er = nullptr;
                        
                        QStringList yErrListAll= app->columnsList(Table::yErr);
                        QString errColumnName="";
                        for(int i=0; i<yErrListAll.count();i++) if (yErrListAll[i].left(yErrListAll[i].lastIndexOf("_"))==name.left(name.lastIndexOf("_")))
                        {
                            errColumnName=yErrListAll[i];
                            break;
                        }
                        if (errColumnName=="") continue;
                        
                        Table *errTable = app->table(errColumnName);
                        if (!errTable) continue;
                        if (errTable->isEmptyColumn(errTable->colIndex(errColumnName)))continue;
                        er = d_graph->addErrorBars(curve, errTable, errColumnName, direction, app->defaultCurveLineWidth, 0, curve->pen().color(),true,true, true);
                        
                    }
				}
			}
			break;

			case MatrixItem:
				if (contents->findItems(text, Qt::MatchExactly).isEmpty()){
					if (!addCurveFromMatrix(app->matrix(text)))
						emptyColumns << text;
				}
			break;

			case FolderItem:
			break;
		}
	}

	d_graph->updateAxesTitles();

	if (legend){
		QString s = legend->text().trimmed();
		if (!s.isEmpty())
			s.append("\n");
		legend->setText(s + d_graph->legendText(false, curves));
		legend->setAutoUpdate(updateLegend);
	}

	d_graph->updatePlot();
	Graph::showPlotErrorMessage(this, emptyColumns);

	showCurveRange(boxShowRange->isChecked());

	QApplication::restoreOverrideCursor();
}

bool CurvesDialog::addCurveFromMatrix(Matrix *m)
{
	if (!m)
		return false;

	QwtPlotItem* it = nullptr;
	switch (boxMatrixStyle->currentIndex()){
		case 0:
			it = d_graph->plotSpectrogram(m, Graph::ColorMap);
		break;
		case 1:
			it = d_graph->plotSpectrogram(m, Graph::Contour);
		break;
		case 2:
			it = d_graph->plotSpectrogram(m, Graph::GrayScale);
		break;
		case 3:
			it = d_graph->addHistogram(m);
		break;
	}

	if (it)
		it->setAxis(boxXAxis->currentIndex() + 2, boxYAxis->currentIndex());

	return true;
}

bool CurvesDialog::addCurveFromTable(ApplicationWindow *app, Table *t, const QString& name)
{
	int style = curveStyle();
	DataCurve *c = nullptr;
	if (style == Graph::Histogram)
    {
		c = new QwtHistogram(t, name);
		if (c)
        {
			d_graph->insertCurve(c);
			((QwtHistogram *)c)->loadData();
			d_graph->addLegendItem();
		}
	} else
		c = d_graph->insertCurve(t, name, style);

	if (!c)
		return false;

	c->setAxis(boxXAxis->currentIndex() + 2, boxYAxis->currentIndex());

	CurveLayout cl = Graph::initCurveLayout();
	int cIndex, sIndex;
	d_graph->guessUniqueCurveLayout(cIndex, sIndex, plusErr->isChecked());

	QList<QColor> indexedColors = app->indexedColors();
	if (cIndex >= 0 && cIndex < indexedColors.size())
		cl.lCol = indexedColors[cIndex];
	cl.symCol = cl.lCol;

	cl.fillCol = app->d_fill_symbols ? cl.lCol : QColor();
	cl.penWidth = app->defaultSymbolEdge;

	cl.lWidth = app->defaultCurveLineWidth;
	cl.lStyle = app->d_curve_line_style;
	cl.sSize = app->defaultSymbolSize;
	if (style == Graph::Area || style == Graph::VerticalBars || style == Graph::HorizontalBars ||
		style == Graph::StackBar || style == Graph::StackColumn || style == Graph::Histogram){
		cl.aStyle = app->defaultCurveBrush;
		cl.filledArea = 0.01*app->defaultCurveAlpha;
	}

	if (app->d_indexed_symbols){
		QList<int> indexedSymbols = app->indexedSymbols();
		if (sIndex >= 0 && sIndex < indexedSymbols.size())
			cl.sType = indexedSymbols[sIndex] + 1;
	} else
		cl.sType = app->d_symbol_style;

	if (style == Graph::Line)
		cl.sType = 0;
	else if (style == Graph::Histogram){
		cl.aCol = cl.lCol;
		cl.sType = 0;
	} else if (style == Graph::VerticalBars || style == Graph::HorizontalBars){
		cl.aCol = cl.lCol;
		cl.lCol = Qt::black;
		int i = d_graph->curveCount() - 1;
		if (i >= 0 && i < indexedColors.size())
			cl.aCol = indexedColors[i];

		cl.sType = 0;
	} else if (style == Graph::Area){
		cl.aCol = cl.lCol;
		cl.sType = 0;
	} else if (style == Graph::VerticalDropLines)
		cl.connectType = 2;
	else if (style == Graph::VerticalSteps || style == Graph::HorizontalSteps){
		cl.connectType = 3;
		cl.sType = 0;
	} else if (style == Graph::Spline)
		cl.connectType = 5;

	d_graph->updateCurveLayout(c, &cl);
	return true;
}

void CurvesDialog::removeCurves()
{
	QList<QListWidgetItem *> lst = contents->selectedItems();
	QList<QwtPlotItem *> curvesList;
	for (int i = 0; i < lst.size(); ++i)
		curvesList << d_graph->curve(contents->row(lst.at(i)));

	foreach(QwtPlotItem *c, curvesList)
		d_graph->removeCurve(c);

	showCurveRange(boxShowRange->isChecked());
	d_graph->updatePlot();
}

void CurvesDialog::enableAddBtn()
{
    btnAdd->setEnabled (available->topLevelItemCount() > 0 &&
						!available->selectedItems().isEmpty());
}

void CurvesDialog::enableContentsBtns()
{
	QList<QListWidgetItem *> lst = contents->selectedItems();
    btnRemove->setEnabled (contents->count()>0 && !lst.isEmpty());

    int row = contents->currentRow();
	btnUp->setEnabled (lst.size() == 1 && row > 0);
	btnDown->setEnabled (lst.size() == 1 && row < contents->count() - 1);
}

int CurvesDialog::curveStyle()
{
	int style = 0;
	switch (boxStyle->currentIndex())
	{
		case 0:
			style = Graph::Line;
			break;
		case 1:
			style = Graph::Scatter;
			break;
		case 2:
			style = Graph::LineSymbols;
			break;
		case 3:
			style = Graph::VerticalDropLines;
			break;
		case 4:
			style = Graph::Spline;
			break;
		case 5:
			style = Graph::VerticalSteps;
			break;
		case 6:
			style = Graph::HorizontalSteps;
			break;
		case 7:
			style = Graph::Area;
			break;
		case 8:
			style = Graph::VerticalBars;
			break;
		case 9:
			style = Graph::HorizontalBars;
			break;
		case 10:
			style = Graph::Histogram;
			break;
	}
	return style;
}

void CurvesDialog::showCurveRange(bool on)
{
	QList<int> selectedRows;
	foreach(QListWidgetItem *it, contents->selectedItems())
		selectedRows << contents->row(it);

	contents->clear();
	if (on){
		QStringList lst = QStringList();
		for (int  i= 0; i < d_graph->curveCount(); i++){
			QwtPlotItem *it = d_graph->plotItem(i);
			if (!it)
				continue;

			if (it->rtti() == QwtPlotItem::Rtti_PlotCurve && ((PlotCurve *)it)->type() != Graph::Function){
				DataCurve *c = (DataCurve *)it;
				lst << c->title().text() + "[" + QString::number(c->startRow() + 1) + ":" + QString::number(c->endRow() + 1) + "]";
			} else
				lst << it->title().text();
		}
		contents->addItems(lst);
	} else
		contents->addItems(d_graph->plotItemsList());

	foreach(int row, selectedRows){//restore selection
		QListWidgetItem *it = contents->item(row);
		if (it)
			it->setSelected(true);
	}

	enableContentsBtns();
}

void CurvesDialog::updateCurveRange()
{
    showCurveRange(boxShowRange->isChecked());
}

void CurvesDialog::showCurrentFolder(bool currentFolder)
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	if (!app)
		return;

	app->d_show_current_folder = currentFolder;
	available->clear();
    
    if (currentFolder)
    {
    	Folder *f = app->currentFolder();
		if (f) addFolderItems(f);
    }
    else
    {
    	Folder *f = app->projectFolder();
		addFolderItems(f);

		f = f->folderBelow();
		TreeWidgetFolderItem *folderItem = nullptr;
		while (f)
        {
			if (f->depth() > 1)
            {
				Folder *parentFolder = (Folder *)f->parent();
				QTreeWidgetItemIterator it(available);
				 while (*it) {
					 TreeWidgetFolderItem *fi = (TreeWidgetFolderItem *)(*it);
					 if (fi->folder() == parentFolder)
                     {
						 folderItem = new TreeWidgetFolderItem(fi, f);
						 break;
					 }
					 ++it;
				 }
			} else
				folderItem = new TreeWidgetFolderItem(available, f);
            //if (folderItem->number()==0) continue;
			available->addTopLevelItem(folderItem);
			if (!addFolderItems(f, folderItem)) folderItem->setHidden(true);

			f = f->folderBelow();
		}
    }
}

bool CurvesDialog::addFolderItems(Folder *f, QTreeWidgetItem* parent)
{
    if (!f)
        return false;

    QRegularExpression rx(QRegularExpression::wildcardToRegularExpression(dataFilter->text()));

    bool existingData = false;
    foreach (MdiSubWindow *w, f->windowsList())
    {
        if (w->inherits("Table"))
        {
            auto t = (Table *)w;
            if (!rx.match(t->name()).hasMatch())
                continue;
            else
                existingData = true;
            QTreeWidgetItem *tableItem;
            if (!parent)
                tableItem = new QTreeWidgetItem(available, QStringList(t->objectName()), TableItem);
            else
                tableItem = new QTreeWidgetItem(parent, QStringList(t->objectName()), TableItem);
            tableItem->setIcon(0, QIcon(QPixmap(":/worksheet.png")));
            available->addTopLevelItem(tableItem);

            for (int i = 0; i < t->numCols(); i++)
            {
                if (t->colPlotDesignation(i) == Table::Y)
                {
                    auto colItem =
                        new QTreeWidgetItem(tableItem, QStringList(t->objectName() + "_" + t->colLabel(i)), ColumnItem);
                    available->addTopLevelItem(colItem);
                }
            }
            continue;
        }

        if (w->inherits("Matrix"))
        {
            auto m = (Matrix *)w;
            if (!rx.match(m->name()).hasMatch())
                continue;
            else
                existingData = true;
            QTreeWidgetItem *item;
            if (!parent)
                item = new QTreeWidgetItem(available, QStringList(m->objectName()), MatrixItem);
            else
                item = new QTreeWidgetItem(parent, QStringList(m->objectName()), MatrixItem);
            item->setIcon(0, QIcon(QPixmap(":/matrix.png")));
            available->addTopLevelItem(item);
        }
    }
    return existingData;
}

void CurvesDialog::closeEvent(QCloseEvent* e)
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	if (app)
		app->d_add_curves_dialog_size = this->size();

	e->accept();
}

void CurvesDialog::raiseCurve()
{
	shiftCurveBy(-1);
}

void CurvesDialog::shiftCurveBy(int offset)
{
	if (!d_graph)
		return;

	int row = contents->currentRow();
	d_graph->changeCurveIndex(row, row + offset);

	contents->clear();
	contents->addItems(d_graph->plotItemsList());
	contents->setCurrentRow(row + offset);
}

TreeWidgetFolderItem::TreeWidgetFolderItem( QTreeWidget *parent, Folder *f )
	: QTreeWidgetItem( parent, QStringList(f->objectName()), CurvesDialog::FolderItem)
{
	myFolder = f;

	setIcon(0, QIcon(":/folder_open.png"));
	setExpanded(true);
}

TreeWidgetFolderItem::TreeWidgetFolderItem( QTreeWidgetItem *parent, Folder *f )
	: QTreeWidgetItem ( parent, QStringList(f->objectName()), CurvesDialog::FolderItem)
{
	myFolder = f;

	setIcon(0, QIcon(":/folder_open.png"));
	setExpanded(true);
}

//+++
void CurvesDialog::filterAvailable()
{
    bool currentFolder=boxShowCurrentFolder->isChecked();
    showCurrentFolder(currentFolder);
}
