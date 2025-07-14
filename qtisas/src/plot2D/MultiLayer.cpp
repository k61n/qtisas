/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Multi layer widget
 ******************************************************************************/

#include <QApplication>
#include <QBitmap>
#include <QCheckBox>
#include <QClipboard>
#include <QDir>
#include <QGroupBox>
#include <QImageWriter>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QSpinBox>
#include <QSvgGenerator>
#include <QSvgRenderer>
#include <QTextDocumentWriter>
#include <QTextStream>
#include <QWidgetList>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include <gsl/gsl_vector.h>
#include <qtexengine/QTeXEngine.h>
#include <qwt/qwt_layout_metrics.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_canvas.h>
#include <qwt/qwt_plot_layout.h>
#include <qwt/qwt_scale_widget.h>
#include <qwt/qwt_text_label.h>

#include "ApplicationWindow.h"
#include "ColorButton.h"
#include "ImportExportPlugin.h"
#include "LegendWidget.h"
#include "Matrix.h"
#include "MultiLayer.h"
#include "PlotCurve.h"
#include "ScaleEngine.h"
#include "ScreenPickerTool.h"
#include "SelectionMoveResizer.h"
#include "Spectrogram.h"

LayerButton::LayerButton(const QString& text, QWidget* parent)
: QPushButton(text, parent)
{
	int btn_size = 20;

    setCheckable(true);
	setChecked(true);
	setMaximumWidth(btn_size);
	setMaximumHeight(btn_size);
	setToolTip(tr("Activate layer"));
}

void LayerButton::mousePressEvent( QMouseEvent *event )
{
	if (!isChecked())
		emit clicked(this);

	if (event->button() == Qt::RightButton && isChecked())
		showLayerContextMenu();
}

void LayerButton::mouseDoubleClickEvent ( QMouseEvent * )
{
	emit showCurvesDialog();
}

MultiLayer::MultiLayer(ApplicationWindow* parent, int layers, int rows, int cols,
			const QString& label, const char* name, Qt::WindowFlags f)
: MdiSubWindow(label, parent, name, f),
active_graph(nullptr),
d_cols(cols),
d_rows(rows),
graph_width(500),
graph_height(400),
colsSpace(5),
rowsSpace(5),
left_margin(5),
right_margin(5),
top_margin(5),
bottom_margin(5),
l_canvas_width(parent->d_layer_canvas_width),
l_canvas_height(parent->d_layer_canvas_height),
hor_align(HCenter),
vert_align(VCenter),
d_scale_on_print(true),
d_print_cropmarks(false),
d_scale_layers(parent->autoResizeLayers),
d_is_waterfall_plot(false),
d_waterfall_fill_color(Qt::black),
d_canvas_size(QSize()),
d_align_policy(AlignLayers),
d_size_policy(UserSize),
d_link_x_axes(false),
d_common_axes_layout(false)
{
	d_layer_coordinates.resize(0);

	layerButtonsBox = new QHBoxLayout();
	waterfallBox = new QHBoxLayout();
	toolbuttonsBox = new QHBoxLayout();
//+++//
    magicTemplate = new QToolButton();
    magicTemplate->setToolTip(tr("\"Magic Template\"::\n\t Apply Template settings to selected Graph\n\t Select Your current template in::\n\t Preferences\\QtiSAS\\Magic Template"));

    magicMenu = new QMenu(this);
    updateMagicMenu();
    connect(magicMenu, SIGNAL(triggered(QAction*)), this, SLOT(magicMenuSelected(QAction*)));
    magicTemplate->setMenu(magicMenu);
    
    magicTemplate->setIcon(QIcon(":/magicTemplate.png"));
    magicTemplate->setMaximumWidth(LayerButton::btnSize());
    magicTemplate->setMaximumHeight(LayerButton::btnSize());
    connect (magicTemplate, SIGNAL(clicked()), this->applicationWindow(), SLOT(setMagicTemplate()));

    toolbuttonsBox->addWidget(magicTemplate);
    
    
    d_loglog = new QToolButton();
    d_loglog->setToolTip("- \"Log-Log\" Presentation:\t if no AXIS is selected\n- \"Log\" Presentation:\t for selected AXIS\n- \"Log\" Presentation of Spectrograms Color-Fill:\t for Map and Bar Scale");

    
    d_loglog->setIcon(QIcon(":/log-log.png"));
    d_loglog->setMaximumWidth(LayerButton::btnSize());
    d_loglog->setMaximumHeight(LayerButton::btnSize());
    connect (d_loglog, SIGNAL(clicked()), this->applicationWindow(), SLOT(setLogLog()));
    toolbuttonsBox->addWidget(d_loglog);
    
    d_linlin = new QToolButton();
    d_linlin->setToolTip("- \"Lin-Lin\" Presentation:\t if no AXIS is selected\n- \"Lin\" Presentation:\t for selected AXIS\n- \"Lin\" Presentation of Spectrograms Color-Fill:\t for Map and Bar Scale");
    
    d_linlin->setIcon(QIcon(":/lin-lin.png"));
    d_linlin->setMaximumWidth(LayerButton::btnSize());
    d_linlin->setMaximumHeight(LayerButton::btnSize());
    connect (d_linlin, SIGNAL(clicked()), this->applicationWindow(), SLOT(setLinLin()));
    toolbuttonsBox->addWidget(d_linlin);
//---//
	d_add_layer_btn = new QToolButton();
	d_add_layer_btn->setToolTip(tr("Add layer"));
	d_add_layer_btn->setIcon(QIcon(":/plus.png"));
	d_add_layer_btn->setMaximumWidth(LayerButton::btnSize());
	d_add_layer_btn->setMaximumHeight(LayerButton::btnSize());
	connect (d_add_layer_btn, SIGNAL(clicked()), this->applicationWindow(), SLOT(addLayer()));
	toolbuttonsBox->addWidget(d_add_layer_btn);

	d_remove_layer_btn = new QToolButton();
	d_remove_layer_btn->setToolTip(tr("Remove active layer"));
	d_remove_layer_btn->setIcon(QIcon(":/delete.png"));
	d_remove_layer_btn->setMaximumWidth(LayerButton::btnSize());
	d_remove_layer_btn->setMaximumHeight(LayerButton::btnSize());
	connect (d_remove_layer_btn, SIGNAL(clicked()), this, SLOT(confirmRemoveLayer()));
	toolbuttonsBox->addWidget(d_remove_layer_btn);

#ifdef Q_OS_MACOS
	layerButtonsBox->setSpacing(12);
//	toolbuttonsBox->setSpacing(12);
#endif
	QHBoxLayout *hbox = new QHBoxLayout();
	hbox->addLayout(layerButtonsBox);
	hbox->addStretch();
	hbox->addLayout(toolbuttonsBox);
	hbox->addLayout(waterfallBox);

	d_canvas = new QWidget();

	QWidget *mainWidget = new QWidget();
	mainWidget->setAutoFillBackground(true);
	mainWidget->setBackgroundRole(QPalette::Window);

	QVBoxLayout* layout = new QVBoxLayout(mainWidget);
	layout->addLayout(hbox);
	layout->addWidget(d_canvas, 1);
    layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	setWidget(mainWidget);

	int canvas_width = graph_width + left_margin + right_margin;
	int canvas_height = graph_height + top_margin + bottom_margin;
	resize(canvas_width, canvas_height + LayerButton::btnSize());

	d_canvas->resize(canvas_width, canvas_height);
	d_canvas->installEventFilter(this);

	QPalette pal = palette();
	pal.setColor(QPalette::Window, QColor(Qt::white));
	setPalette(pal);

	for (int i = 0; i < layers; i++)
		addLayer();

	setFocusPolicy(Qt::StrongFocus);
	setFocus();
	setAcceptDrops(true);
}

Graph *MultiLayer::layer(int num)
{
    int index = num - 1;
    if (index < 0 || index >= graphsList.count())
        return 0;

	return (Graph*) graphsList.at(index);
}

LayerButton* MultiLayer::addLayerButton()
{
	foreach(LayerButton *btn, buttonsList)
		btn->setChecked(false);

	LayerButton *button = new LayerButton(QString::number(graphsList.size() + 1));
	connect (button, SIGNAL(clicked(LayerButton*)), this, SLOT(activateGraph(LayerButton*)));
	connect (button, SIGNAL(showCurvesDialog()), this, SIGNAL(showCurvesDialog()));
	ApplicationWindow *app = applicationWindow();
	if (app)
		connect(button, SIGNAL(showLayerContextMenu()), app, SLOT(showWindowContextMenu()));

	buttonsList.append(button);
    layerButtonsBox->addWidget(button);
	return button;
}

Graph* MultiLayer::addLayer(int x, int y, int width, int height, bool setPreferences)
{
	addLayerButton();
	if (!width && !height){
		width =	(int)((d_canvas->width() - left_margin - right_margin - (d_cols - 1)*colsSpace)/(double)d_cols);
		height = (int)((d_canvas->height() - top_margin - left_margin - (d_rows - 1)*rowsSpace)/(double)d_rows);

		int layers = graphsList.size();
		x = left_margin + (layers % d_cols)*(width + colsSpace);
	    y = top_margin + (layers / d_cols)*(height + rowsSpace);
	} else if (!width)
		 width = height;
	  else if (!height)
		 height = width;

	Graph* g = new Graph(x, y, width, height, d_canvas);
    g->show();
    g->raiseEnrichements();
	graphsList.append(g);

	active_graph = g;
	if (setPreferences)
		applicationWindow()->setPreferences(g);

	connectLayer(g);
	return g;
}

void MultiLayer::activateGraph(LayerButton* button)
{
	for (int i = 0; i<buttonsList.count(); i++){
		LayerButton *btn=(LayerButton*)buttonsList.at(i);
		if (btn->isChecked())
			btn->setChecked(false);

		if (btn == button){
			active_graph = (Graph*) graphsList.at(i);
			active_graph->setFocus();
			active_graph->raise();//raise layer on top of the layers stack

			if (d_layers_selector){
				delete d_layers_selector;
				d_layers_selector = new SelectionMoveResizer(active_graph->canvas());
				connect(d_layers_selector, SIGNAL(targetsChanged()), this, SIGNAL(modifiedPlot()));
			} else
				active_graph->raiseEnrichements();
			button->setChecked(true);
		}
	}
}

bool MultiLayer::isLayerSelected(Graph* g)
{
	if (!g || !d_layers_selector)
		return false;

	return d_layers_selector.data()->contains(g->canvas());
}

//! Try to guess on which layer the user clicked if the layers are superposed
Graph* MultiLayer::clickedLayer(Graph* g)
{
	if (!g)
		return 0;

	QRect ar = g->frameGeometry();
	foreach (Graph *gr, graphsList){
		if (gr == g)
			continue;

		QPoint pos = gr->mapFromGlobal(QCursor::pos());
		if ((ar.contains(gr->frameGeometry()) && gr->rect().contains(pos)))
			return gr;
	}
	return g;
}

void MultiLayer::selectLayerCanvas(Graph* g)
{
	setActiveLayer(g);

	if (active_graph && active_graph != g){
        QMouseEvent e(QEvent::MouseButtonPress, mapFromGlobal(QCursor::pos()), QCursor::pos(), Qt::LeftButton,
                      Qt::NoButton, Qt::NoModifier);
		if (!active_graph->mousePressed(&e)){
			d_layers_selector = new SelectionMoveResizer(active_graph->canvas());
			connect(d_layers_selector, SIGNAL(targetsChanged()), this, SIGNAL(modifiedPlot()));
		}
	} else {
		d_layers_selector = new SelectionMoveResizer(g->canvas());
		connect(d_layers_selector, SIGNAL(targetsChanged()), this, SIGNAL(modifiedPlot()));
	}
}

void MultiLayer::setActiveLayer(Graph* g)
{
	/*Graph *ag = clickedLayer(g);
	if (!ag || active_graph == ag)
		return;*/

	if (!g || active_graph == g)
		return;

	active_graph = g;
	active_graph->setFocus();

	deselect();

	active_graph->raise();//raise layer on top of the layers stack
	active_graph->raiseEnrichements();

	for(int i = 0; i < graphsList.count(); i++){
		Graph *gr = (Graph *)graphsList.at(i);
		gr->deselect();

		LayerButton *btn = (LayerButton *)buttonsList.at(i);
		if (gr == g)
			btn->setChecked(true);
		else
			btn->setChecked(false);
	}
}

QRect MultiLayer::canvasChildrenRect()
{
	QRect r = QRect();
	foreach (Graph *g, graphsList)
		r = r.united(g->boundingRect());

	return r.adjusted(0, 0, right_margin, bottom_margin);
}

void MultiLayer::adjustLayersToCanvasSize()
{
	foreach (Graph *g, graphsList){
		QRectF rf = g->pageGeometry();
		if (rf.isNull())
			continue;

		int x = qRound(rf.x()*d_canvas->width());
		int y = qRound(rf.y()*d_canvas->height());
		int w = qRound(rf.width()*d_canvas->width());
		int h = qRound(rf.height()*d_canvas->height());
		g->setGeometry(x, y, w, h);
	}
}

void MultiLayer::resizeLayers(QResizeEvent *re)
{
	if (!d_scale_layers || applicationWindow()->d_opening_file || graphsList.isEmpty())
    {
		if (!applicationWindow()->d_opening_file)
			emit modifiedPlot();
		return;
	}

	QSize size = re->size();
	QSize oldSize = re->oldSize();

	if (size.height() <= 0) size.setHeight(oldSize.height());

	bool invalidOldSize = !oldSize.isValid();
	if (invalidOldSize)
    {
		//The old size is invalid when maximizing a window or when a minimized window is restored from a project file
		if (d_canvas_size.isValid())
			oldSize = d_canvas_size;
		else if (this->isMaximized())
        {
			adjustLayersToCanvasSize();
			return;
		} else
			return;
	}

	double w = (double)size.width();
	double h = (double)size.height();
	double w_ratio = w/(double)oldSize.width();
	double h_ratio = h/(double)(oldSize.height());

	if (d_common_axes_layout && !invalidOldSize){
		arrangeLayers(false, false);
		foreach (Graph *g, graphsList){
			if (g->autoscaleFonts())
				g->scaleFonts(h_ratio);
		}
	} else {
		Graph *g0 = graphsList[0];
		int l = graphsList.size();
		if (d_layer_coordinates.size() == 0){
			d_layer_coordinates.resize(l);

			QPoint oPos = g0->pos() + g0->canvas()->pos();
			for(int i = 1; i < l; i++){
				Graph *g = graphsList[i];
				if (!g)
					continue;

				QwtPlotCanvas *canvas = g->canvas();
				QPoint pos = g->pos() + canvas->pos() - oPos;

				double xl = g0->invTransform(QwtPlot::xBottom, pos.x());
				double yt = g0->invTransform(QwtPlot::yLeft, pos.y());
				double xr = g0->invTransform(QwtPlot::xBottom, pos.x() + canvas->width() - 1);
				double yb = g0->invTransform(QwtPlot::yLeft, pos.y() + canvas->height() - 1);

				d_layer_coordinates[i] = QRectF(QPointF(xl, yt), QPointF(xr, yb));
			}
		}

		QwtPlotCanvas *canvas0 = g0->canvas();
		g0->setGeometry(qRound(g0->x()*w_ratio), qRound(g0->y()*h_ratio),
						qRound(g0->width()*w_ratio), qRound(g0->height()*h_ratio));

		if (g0->autoscaleFonts())
			g0->scaleFonts(h_ratio);
		g0->updateLayout();

		for(int i = 1; i < l; i++){
			Graph *g = graphsList[i];
			if (!g)
				continue;

			QRectF r = d_layer_coordinates[i];

			int xl = g0->transform(QwtPlot::xBottom, r.left());
			int yt = g0->transform(QwtPlot::yLeft, r.top());
			int xr = g0->transform(QwtPlot::xBottom, r.right());
			int yb = g0->transform(QwtPlot::yLeft, r.bottom());

			QPoint tl = canvas0->mapTo(d_canvas, QPoint(xl, yt));
			QPoint br = canvas0->mapTo(d_canvas, QPoint(xr, yb));

			if (g->autoscaleFonts())
				g->scaleFonts(h_ratio);

			g->setCanvasGeometry(QRect(tl, br));
		}
	}

	foreach (Graph *g, graphsList)
		g->setPageGeometry(QRectF((double)g->x()/w, (double)g->y()/h, (double)g->width()/w, (double)g->height()/h));

	if (d_is_waterfall_plot)
		updateWaterfalls();

	emit modifiedPlot();
}

void MultiLayer::confirmRemoveLayer()
{
    if (graphsList.size() > 1)
    {
        switch (QMessageBox::question(this, tr("QtiSAS - Guess best layout?"),
                                      tr("Do you want QtiSAS to rearrange the remaining layers?"),
                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel))
        {
        case QMessageBox::Yes:
            removeLayer(active_graph);
            arrangeLayers(true, false);
            break;
        case QMessageBox::No:
            removeLayer(active_graph);
            break;
        default:
            break;
		}
	} else
		removeLayer(active_graph);
}

bool MultiLayer::removeActiveLayer()
{
    return removeLayer(active_graph);
}

bool MultiLayer::removeLayer(Graph *g)
{
    if (!g)
        return false;

    int index = graphsList.indexOf(g);
    if (index < 0 || index > graphsList.size())
        return false;

	//remove corresponding button
	LayerButton* btn = buttonsList.at(index);
	if (btn)
        btn->close();
    buttonsList.removeAt(index);

	int i = 0;
	foreach(LayerButton* btn, buttonsList){
		btn->setText(QString::number(++i));//update the texts of the buttons
		btn->setChecked(false);
    }

	if (g->zoomOn() || g->activeTool())
		emit setPointerCursor();

	graphsList.removeAt(index);
	g->close();
	if(index >= graphsList.count())
		index--;

	emit modifiedWindow(this);

	if (graphsList.count() == 0){
		active_graph = nullptr;
		return true;
	}

	active_graph = (Graph*) graphsList.at(index);

	for (i=0; i<(int)graphsList.count(); i++){
		Graph *gr = (Graph *)graphsList.at(i);
		if (gr == active_graph){
			LayerButton *button = (LayerButton *)buttonsList.at(i);
			button->setChecked(true);
			break;
		}
	}
	return true;
}

void MultiLayer::setGraphGeometry(int x, int y, int w, int h)
{
	if (active_graph->pos() == QPoint (x,y) &&
		active_graph->size() == QSize (w,h))
		return;

	active_graph->setGeometry(QRect(QPoint(x,y),QSize(w,h)));
	emit modifiedPlot();
}

void MultiLayer::setEqualSizedLayers()
{
	int layers = graphsList.size();
	const QRect rect = d_canvas->geometry();

	int width = qRound((rect.width() - (d_cols - 1)*colsSpace - right_margin - left_margin)/(double)d_cols);
	int height = qRound((rect.height() - (d_rows - 1)*rowsSpace - top_margin - bottom_margin)/(double)d_rows);

	for(int i = 0; i < layers; i++){
		Graph *g = graphsList[i];

		int row = i / d_cols;
		if (row >= d_rows )
			row = d_rows - 1;

		int col = i % d_cols;

		int x = left_margin + col*(width + colsSpace);
	    int y = top_margin + row*(height + rowsSpace);

		bool autoscaleFonts = g->autoscaleFonts();//save user settings
		g->setAutoscaleFonts(false);
		g->setGeometry(x, y, width, height);
		g->setAutoscaleFonts(autoscaleFonts);
	}
}

QSize MultiLayer::arrangeLayers(bool userSize)
{
	if (userSize)
		d_size_policy = UserSize;
	else
		d_size_policy = Expanding;

	int layers = graphsList.size();
	const QRect rect = d_canvas->geometry();

	gsl_vector *xTopR = gsl_vector_calloc (layers);//ratio between top axis + title and d_canvas height
	gsl_vector *xBottomR = gsl_vector_calloc (layers); //ratio between bottom axis and d_canvas height
	gsl_vector *yLeftR = gsl_vector_calloc (layers);
	gsl_vector *yRightR = gsl_vector_calloc (layers);
	gsl_vector *maxXTopHeight = gsl_vector_calloc (d_rows);//maximum top axis + title height in a row
	gsl_vector *maxXBottomHeight = gsl_vector_calloc (d_rows);//maximum bottom axis height in a row
	gsl_vector *maxYLeftWidth = gsl_vector_calloc (d_cols);//maximum left axis width in a column
	gsl_vector *maxYRightWidth = gsl_vector_calloc (d_cols);//maximum right axis width in a column
	gsl_vector *Y = gsl_vector_calloc (d_rows);
	gsl_vector *X = gsl_vector_calloc (d_cols);

	for (int i=0; i<layers; i++)
	{//calculate scales/d_canvas dimensions reports for each layer and stores them in the above vectors
		Graph *g = (Graph *)graphsList.at(i);
		QwtPlotLayout *plotLayout = g->plotLayout();
		QRect cRect = plotLayout->canvasRect();
		double ch = (double) cRect.height();
		double cw = (double) cRect.width();

		QRect tRect=plotLayout->titleRect ();
		QwtScaleWidget *scale=(QwtScaleWidget *) g->axisWidget (QwtPlot::xTop);

		int topHeight = 0;
		if (!tRect.isNull())
			topHeight += tRect.height() + plotLayout->spacing();
		if (scale){
			QRect sRect=plotLayout->scaleRect (QwtPlot::xTop);
			topHeight += sRect.height();
		}
		gsl_vector_set (xTopR, i, double(topHeight)/ch);

		scale=(QwtScaleWidget *) g->axisWidget (QwtPlot::xBottom);
		if (scale){
			QRect sRect = plotLayout->scaleRect (QwtPlot::xBottom);
			gsl_vector_set (xBottomR, i, double(sRect.height())/ch);
		}

		scale=(QwtScaleWidget *) g->axisWidget (QwtPlot::yLeft);
		if (scale){
			QRect sRect = plotLayout->scaleRect (QwtPlot::yLeft);
			gsl_vector_set (yLeftR, i, double(sRect.width())/cw);
		}

		scale=(QwtScaleWidget *) g->axisWidget (QwtPlot::yRight);
		if (scale){
			QRect sRect = plotLayout->scaleRect (QwtPlot::yRight);
			gsl_vector_set (yRightR, i, double(sRect.width())/cw);
		}

		//calculate max scales/d_canvas dimensions ratio for each line and column and stores them to vectors
		int row = i / d_cols;
		if (row >= d_rows )
			row = d_rows - 1;

		int col = i % d_cols;

		double aux = gsl_vector_get(xTopR, i);
		double old_max = gsl_vector_get(maxXTopHeight, row);
		if (aux >= old_max)
			gsl_vector_set(maxXTopHeight, row,  aux);

		aux = gsl_vector_get(xBottomR, i) ;
		if (aux >= gsl_vector_get(maxXBottomHeight, row))
			gsl_vector_set(maxXBottomHeight, row,  aux);

		aux = gsl_vector_get(yLeftR, i);
		if (aux >= gsl_vector_get(maxYLeftWidth, col))
			gsl_vector_set(maxYLeftWidth, col, aux);

		aux = gsl_vector_get(yRightR, i);
		if (aux >= gsl_vector_get(maxYRightWidth, col))
			gsl_vector_set(maxYRightWidth, col, aux);
	}

	double c_heights = 0.0;
	for (int i = 0; i < d_rows; i++){
		gsl_vector_set (Y, i, c_heights);
		c_heights += 1 + gsl_vector_get(maxXTopHeight, i) + gsl_vector_get(maxXBottomHeight, i);
	}

	double c_widths = 0.0;
	for (int i=0; i<d_cols; i++){
		gsl_vector_set (X, i, c_widths);
		c_widths += 1 + gsl_vector_get(maxYLeftWidth, i) + gsl_vector_get(maxYRightWidth, i);
	}

	if (!userSize){
		l_canvas_width = qRound((rect.width()-(d_cols-1)*colsSpace - right_margin - left_margin)/(double)c_widths);
		l_canvas_height = qRound((rect.height()-(d_rows-1)*rowsSpace - top_margin - bottom_margin)/(double)c_heights);
	}

	if (l_canvas_width < 50 || l_canvas_height < 50)
		return QSize();

	QSize size = QSize(l_canvas_width, l_canvas_height);
	for (int i=0; i<layers; i++){
		int row = i / d_cols;
		if (row >= d_rows )
			row = d_rows - 1;

		int col = i % d_cols;

		//calculate sizes and positions for layers
		const int w = int (l_canvas_width*(1 + gsl_vector_get(yLeftR, i) + gsl_vector_get(yRightR, i)));
		const int h = int (l_canvas_height*(1 + gsl_vector_get(xTopR, i) + gsl_vector_get(xBottomR, i)));

		int x = left_margin;
		int y = top_margin;

		Graph *lg = 0;
		if (col){
			int index = i - 1;
			if (index >= 0){
				lg = graphsList.at(index);//left neighbour
				if (lg)
					x = lg->x() + lg->width() + colsSpace;
			}
		}

		Graph *tg = 0;
		if (row){
			int index = i - d_cols;
			if (index >= 0){
				tg = graphsList.at(index);//top neighbour
				if (tg)
					y = tg->y() + tg->height() + rowsSpace;
			}
		}

		//resizes and moves layers
		Graph *g = (Graph *)graphsList.at(i);
		bool autoscaleFonts = g->autoscaleFonts();//save user font settings
		g->setAutoscaleFonts(false);

		if (userSize)
			g->setCanvasSize(size);
		else
			g->resize(w, h);

		g->move(x, y);

		if (d_align_policy == AlignCanvases){
			QPoint pos = g->mapTo(d_canvas, g->canvas()->pos());
			if (col && lg){
				pos.setX(lg->mapTo(d_canvas, lg->canvas()->pos()).x() + lg->canvas()->width() + colsSpace);
				pos.setY(lg->mapTo(d_canvas, lg->canvas()->pos()).y());
			} else if (row && tg){
				pos.setX(tg->mapTo(d_canvas, tg->canvas()->pos()).x());
				pos.setY(tg->mapTo(d_canvas, tg->canvas()->pos()).y() + tg->canvas()->height() + rowsSpace);
			}
			g->setCanvasGeometry(QRect(pos, size));
		}

		g->setAutoscaleFonts(autoscaleFonts);//restore user font settings
	}

	//free memory
	gsl_vector_free (maxXTopHeight); gsl_vector_free (maxXBottomHeight);
	gsl_vector_free (maxYLeftWidth); gsl_vector_free (maxYRightWidth);
	gsl_vector_free (xTopR); gsl_vector_free (xBottomR);
	gsl_vector_free (yLeftR); gsl_vector_free (yRightR);
	gsl_vector_free (X); gsl_vector_free (Y);

	if (!graphsList.isEmpty()){
		Graph *g = graphsList[0];
		if (g){
			size = g->canvas()->size();//return the real size of the canvas
			l_canvas_width = size.width();
			l_canvas_height = size.height();
		}
	}
	return size;
}

void MultiLayer::setCommonLayerAxes(bool verticalAxis, bool horizontalAxis)
{
	d_common_axes_layout = true;

	int layers = graphsList.size();
	for (int i = 0; i < layers; i++){
		int row = i / d_cols;
		if (row >= d_rows )
			row = d_rows - 1;

		int col = i % d_cols;

		int index = i - 1; //left neighbour
		if (verticalAxis && col && index >= 0){
			Graph *aux = graphsList.at(index);
			if (aux){
				QwtScaleWidget *scale = aux->axisWidget(QwtPlot::yRight);
				if (scale){
                    aux->setAxisTitleString(QwtPlot::yRight, QString());
					QwtScaleDraw *sd = aux->axisScaleDraw(QwtPlot::yRight);
					if (sd)
						sd->enableComponent(QwtAbstractScaleDraw::Labels, false);
				}
			}
		}

		index = i - d_cols;
		if (horizontalAxis && row && index >= 0){
			Graph *aux = graphsList.at(index);//top neighbour
			if (aux){
				QwtScaleWidget *scale = aux->axisWidget(QwtPlot::xBottom);
				if (scale){
                    aux->setAxisTitleString(QwtPlot::xBottom, QString());
					QwtScaleDraw *sd = aux->axisScaleDraw(QwtPlot::xBottom);
					if (sd)
						sd->enableComponent(QwtAbstractScaleDraw::Labels, false);
				}
			}
		}

		Graph *g = (Graph *)graphsList.at(i);
		if (!g)
			continue;

		QColor c = Qt::white;
		c.setAlpha(0);
		g->setBackgroundColor(c);
		g->setCanvasBackground(c);

		if (horizontalAxis && row && !g->title().text().isEmpty())
            g->setTitle(QString());

		if (verticalAxis && !col && (row != d_rows - 1)){
			QwtScaleWidget *scale = g->axisWidget(QwtPlot::yLeft);
			if (scale){
				ScaleDraw *sd = (ScaleDraw *)g->axisScaleDraw(QwtPlot::yLeft);
				if (sd)
					sd->setShowTicksPolicy(ScaleDraw::HideBegin);
			}
		}

		if (verticalAxis && col){
			QwtScaleWidget *scale = g->axisWidget(QwtPlot::yLeft);
			if (scale){
                g->setAxisTitleString(QwtPlot::yLeft, QString());
				QwtScaleDraw *sd = g->axisScaleDraw(QwtPlot::yLeft);
				if (sd)
					sd->enableComponent(QwtAbstractScaleDraw::Labels, false);
			}

			if (row != d_rows - 1){
				scale = g->axisWidget(QwtPlot::yRight);
				if (scale){
					ScaleDraw *sd = (ScaleDraw *)g->axisScaleDraw(QwtPlot::yRight);
					if (sd)
						sd->setShowTicksPolicy(ScaleDraw::HideBegin);
				}
			}
		}

		if (horizontalAxis){
			QwtScaleWidget *scale = g->axisWidget(QwtPlot::xTop);
			if (scale){
				if (row)
                    g->setAxisTitleString(QwtPlot::xTop, QString());

				ScaleDraw *sd = (ScaleDraw *)g->axisScaleDraw(QwtPlot::xTop);
				if (sd){
					if (row)
						sd->enableComponent(QwtAbstractScaleDraw::Labels, false);
					else if (col)
						sd->setShowTicksPolicy(ScaleDraw::HideBegin);
				}

				if (col && row == d_rows - 1){
					sd = (ScaleDraw *)g->axisScaleDraw(QwtPlot::xBottom);
					if (sd)
						sd->setShowTicksPolicy(ScaleDraw::HideBegin);
				}
			}
		}
		g->updateLayout();
	}
}

void MultiLayer::findBestLayout(int &d_rows, int &d_cols)
{
	int layers = graphsList.size();
	int sqr = (int)ceil(sqrt(layers));
	d_rows = sqr;
	d_cols = sqr;

	if (d_rows*d_cols - layers >= d_rows)
		d_rows--;

}

bool MultiLayer::arrangeLayers(bool fit, bool userSize)
{
	if (graphsList.size() == 0)
		return false;

	QApplication::setOverrideCursor(Qt::WaitCursor);

	if(d_layers_selector)
		delete d_layers_selector;

	if (fit)
		findBestLayout(d_rows, d_cols);

	bool minimized = isMinimized();
	if (minimized)
		showNormal();

	QSize size = arrangeLayers(userSize);
	if (!size.isValid()){
		QApplication::restoreOverrideCursor();
		setEqualSizedLayers();
		return false;
	}

	int fw = width() - d_canvas->width();//frame width
	int fh = height() - d_canvas->height();//frame height

	if (!userSize){
		//the d_canvas sizes of all layers become equal only after several
		//resize iterations, due to the way Qwt handles the plot layout
		int iterations = 0;
		QSize canvas_size = QSize(1, 1);
		while (canvas_size != size && iterations < 10){
			iterations++;
			canvas_size = size;
			size = arrangeLayers(userSize);
			if (!size.isValid()){
				QApplication::restoreOverrideCursor();
				setEqualSizedLayers();
				return false;
			}
		}
	} else if (!this->isMaximized()){//resize window to fit new dimensions of the layers
		bool resizeLayers = d_scale_layers;
		d_scale_layers = false;

		QSize size = d_canvas->childrenRect().size();
		resize(d_canvas->x() + size.width() + left_margin + right_margin + fw, d_canvas->y() + size.height() + fh - bottom_margin);

		d_scale_layers = resizeLayers;
	}

	if (minimized) showMinimized();

	emit modifiedPlot();
	QApplication::restoreOverrideCursor();

	return true;
}

void MultiLayer::setCols(int c)
{
	if (d_cols != c)
		d_cols = c;
}

void MultiLayer::setRows(int r)
{
	if (d_rows != r)
		d_rows = r;
}

QList<Graph*> MultiLayer::stackOrderedLayersList()
{
	QList<Graph*> gLst;
	QObjectList lst = d_canvas->children();//! this list is sorted according to the stack order
	foreach (QObject *o, lst){
		Graph *g = qobject_cast<Graph *>(o);
		if (g)
			gLst << g;
	}
	return gLst;
}

QPixmap MultiLayer::canvasPixmap(const QSize& size, double scaleFontsFactor, bool transparent)
{
	if (!size.isValid()){
		QPixmap pic(d_canvas->size());
		if (transparent)
			pic.fill(Qt::transparent);
		else
			pic.fill();
		QPainter p(&pic);

		QList<Graph*> lst = stackOrderedLayersList();
		foreach (Graph *g, lst)
			g->print(&p, g->geometry(), ScaledFontsPrintFilter(1.0));

		p.end();
		return pic;
	}

	double xScale = (double)size.width()/(double)d_canvas->width();
	double yScale = (double)(size.height())/(double)(d_canvas->height());
	if (scaleFontsFactor == 0.0)
		scaleFontsFactor = yScale;

	QPixmap pic(size);
	if (transparent)
		pic.fill(Qt::transparent);
	else
		pic.fill();
	QPainter p(&pic);

	QList<Graph*> lst = stackOrderedLayersList();
	foreach (Graph *g, lst){
		int gx = qRound(g->x()*xScale);
		int gy = qRound(g->y()*yScale);
		int gw = qRound(g->width()*xScale);
		int gh = qRound(g->height()*yScale);

		g->print(&p, QRect(gx, gy, gw, gh), ScaledFontsPrintFilter(scaleFontsFactor));
	}

	p.end();
	return pic;
}

void MultiLayer::exportToFile(const QString &fileName)
{
    if (fileName.isEmpty())
    {
        QMessageBox::critical(nullptr, "QtiSAS - Error", "Please provide a valid file name!");
        return;
    }

    for (const QString &ext : {".eps", ".pdf", ".ps"})
    {
        if (fileName.endsWith(ext, Qt::CaseInsensitive))
        {
            exportVector(fileName, true, defaultResolusion);
            return;
        }
    }

    if (fileName.endsWith(".svg", Qt::CaseInsensitive))
    {
        exportSVG(fileName);
        return;
    }

    if (fileName.endsWith(".tex", Qt::CaseInsensitive))
    {
        exportTeX(fileName);
        return;
    }

    if (fileName.endsWith(".emf", Qt::CaseInsensitive))
    {
        exportEMF(fileName);
        return;
    }

    // Check if file extension matches a supported raster image format
    const ApplicationWindow *app = applicationWindow();
    const QList<QByteArray> formats = QImageWriter::supportedImageFormats();

    for (const QByteArray &format : formats)
    {
        if (fileName.endsWith("." + format.toLower(), Qt::CaseInsensitive))
        {
            exportImage(fileName, app->d_export_quality, app->d_export_transparency, app->d_export_bitmap_resolution);
            return;
        }
    }

    QMessageBox::critical(this, "QtiSAS - Error", "File format not handled, operation aborted!");
}

void MultiLayer::exportImage(const QString &fileName, int quality, bool transparent, int dpi, const QSizeF &customSize,
                             int unit, double fontsFactor, int compression)
{
    if (!dpi)
        dpi = logicalDpiX();

    QSize size = QSize();
    if (customSize.isValid())
        size = Graph::customPrintSize(customSize, unit, dpi);

    QString fn = fileName;
    fn = fn.replace(".qti.gz", ".tmp.svg").replace(".qti", ".tmp.svg");
    exportSVG(fn, dpi, size, unit, fontsFactor);

    // Prepare a QImage with desired characteritisc
    QSvgRenderer renderer(fn);
    double factorRES = dpi / defaultResolusion;

    QSizeF imageInitSize = renderer.defaultSize();
    QImage imageSVG(int(factorRES * imageInitSize.width()), int(factorRES * imageInitSize.height()),
                    QImage::Format_ARGB32);
    if (transparent)
        imageSVG.fill(QColor("transparent"));
    else
        imageSVG.fill(QColor("white"));
    QPainter painter(&imageSVG);
    renderer.render(&painter);
    QImage image = imageSVG.copy(0, 0, int(factorRES * imageInitSize.width()), int(factorRES * imageInitSize.height()));
    
    QFile::remove(fn);

    QImageWriter writer(fileName);
    if (compression > 0 && writer.supportsOption(QImageIOHandler::CompressionRatio))
    {
        writer.setQuality(quality);
        writer.setCompression(compression);
        writer.write(image);
    }
    else
        image.save(fileName, nullptr, quality);
}

void MultiLayer::exportImage(QTextDocument *document, int, bool transparent,
				int dpi, const QSizeF& customSize, int unit, double fontsFactor)
{
	if (!document)
		return;

	if (!dpi)
		dpi = logicalDpiX();

    
	QSize size = QSize();
	if (customSize.isValid())
		size = Graph::customPrintSize(customSize, unit, dpi);

	QPixmap pic = canvasPixmap(size, fontsFactor);
	QImage image = pic.toImage();

	if (transparent){
		QBitmap mask(size);
		mask.fill(Qt::color1);
		QPainter p(&mask);
		p.setPen(Qt::color0);

		QRgb backgroundPixel = QColor(Qt::white).rgb ();
		for (int y = 0; y < image.height(); y++){
			for (int x = 0; x < image.width(); x++){
				QRgb rgb = image.pixel(x, y);
				if (rgb == backgroundPixel) // we want the frame transparent
					p.drawPoint(x, y);
			}
		}
		p.end();
		pic.setMask(mask);
		image = pic.toImage();
	}

	int dpm = (int)ceil(100.0/2.54*dpi);
	image.setDotsPerMeterX(dpm);
	image.setDotsPerMeterY(dpm);

	QTextCursor cursor = QTextCursor(document);
	cursor.movePosition(QTextCursor::End);
	cursor.insertBlock();
	cursor.insertText(objectName());
	cursor.insertBlock();
	cursor.insertImage(image);
}

void MultiLayer::exportPDF(const QString& fname)
{
	exportVector(fname, true);
}

void MultiLayer::exportVector(QPrinter *printer, bool fontEmbedding, int res, bool color,
				const QSizeF& customSize, int unit, double fontsFactor)
{
	if (!printer)
		return;
	if (!printer->resolution())
		printer->setResolution(logicalDpiX());//we set screen resolution as default

	printer->setDocName (objectName());
	printer->setFontEmbeddingEnabled(fontEmbedding);
	printer->setCreator("QtiSAS");
	printer->setFullPage(true);

	if (color)
		printer->setColorMode(QPrinter::Color);
	else
		printer->setColorMode(QPrinter::GrayScale);

    printer->setPageOrientation(QPageLayout::Portrait);

    double wfactor = (double)res / 72.0;
    double hfactor = (double)res / 72.0;

	if (customSize.isValid()){
		QSize size = Graph::customPrintSize(customSize, unit, res);
		if (res && res != printer->resolution())
			printer->setResolution(res);
        // to make compatible with QPageSize in Qt >= 5.3
        QSize size_in_points = Graph::customPrintSize(customSize, FrameWidget::Unit::Point, res);
        printer->setPageSize(QPageSize(QSizeF(size_in_points), QPageSize::Point));
		QPainter paint(printer);
		QList<Graph*> lst = stackOrderedLayersList();

        wfactor *= (double)size.width() / (double)d_canvas->width();
        hfactor *= (double)size.height() / (double)d_canvas->height();

        paint.scale(wfactor * wfactor, hfactor * hfactor);

		foreach (Graph *g, lst){
			QRect r = g->geometry();
			r.setSize(QSize(int(r.width()*wfactor), int(r.height()*hfactor)));
			r.moveTo(int(r.x()*wfactor), int(r.y()*hfactor));

			if (fontsFactor == 0.0)
				fontsFactor = Graph::customPrintSize(customSize, unit, logicalDpiX()).height()/(double)height();

			g->print(&paint, r, ScaledFontsPrintFilter(fontsFactor));
		}
		paint.end();
	} else if (res && res != printer->resolution()){

        wfactor *= (double)res / (double)logicalDpiX();
        hfactor *= (double)res / (double)logicalDpiY();

		printer->setResolution(res);
        // to make compatible with QPageSize in Qt >= 5.3
        QSize size_in_points = Graph::customPrintSize(QSizeF(d_canvas->width() * wfactor, d_canvas->height() * hfactor),
                                                      FrameWidget::Unit::Point, res);
        printer->setPageSize(QPageSize(QSizeF(size_in_points), QPageSize::Point));
        QPainter paint(printer);
        paint.scale(wfactor * wfactor, hfactor * hfactor);

		QList<Graph*> lst = stackOrderedLayersList();
		foreach (Graph *g, lst){
			QRect r = g->geometry();
			r.setSize(QSize(int(r.width()*wfactor), int(r.height()*hfactor)));
			r.moveTo(int(r.x()*wfactor), int(r.y()*hfactor));
			g->print(&paint, r, ScaledFontsPrintFilter(fontsFactor));
		}
		paint.end();
	} else {
        // to make compatible with QPageSize in Qt >= 5.3
        QSize size_in_points =
            Graph::customPrintSize(QSizeF(d_canvas->width(), d_canvas->height()), FrameWidget::Unit::Point, res);
        printer->setPageSize(QPageSize(QSizeF(size_in_points), QPageSize::Point));
		QPainter paint(printer);
        paint.scale(wfactor * wfactor, hfactor * hfactor);
		QList<Graph*> lst = stackOrderedLayersList();
		foreach (Graph *g, lst)
			g->print(&paint, g->geometry(), ScaledFontsPrintFilter(fontsFactor));
		paint.end();
	}
}

void MultiLayer::exportVector(const QString& fileName, bool fontEmbedding, int res, bool color,
				const QSizeF& customSize, int unit, double fontsFactor)
{
	if (fileName.isEmpty()){
		QMessageBox::critical(this, tr("QtiSAS - Error"), tr("Please provide a valid file name!"));
		return;
	}

	QPrinter printer;
	printer.setOutputFileName(fileName);
    
	if (fileName.contains(".eps"))
		printer.setOutputFormat(QPrinter::PdfFormat);

	exportVector(&printer, fontEmbedding, res, color, customSize, unit, fontsFactor);
}

void MultiLayer::draw(QPaintDevice *device, const QSizeF &customSize, int unit, int res, double fontsFactor, int reso)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QPainter paint(device);

	if (customSize.isValid()){
		QSize size = Graph::customPrintSize(customSize, unit, res);
		QList<Graph*> lst = stackOrderedLayersList();
		foreach (Graph *g, lst){
			QRect r = g->geometry();
			double wfactor = (double)size.width()/(double)d_canvas->width();
			double hfactor = (double)size.height()/(double)d_canvas->height();
			r.setSize(QSize(int(r.width()*wfactor), int(r.height()*hfactor)));
			r.moveTo(int(r.x()*wfactor), int(r.y()*hfactor));

			if (fontsFactor == 0.0)
				fontsFactor = Graph::customPrintSize(customSize, unit, logicalDpiX()).height()/(double)height();

            g->print(&paint, r, ScaledFontsPrintFilter(fontsFactor), reso);
		}
	} else {
		QList<Graph*> lst = stackOrderedLayersList();
		foreach (Graph *g, lst)
            g->print(&paint, g->geometry(), ScaledFontsPrintFilter(fontsFactor), reso);
	}
	paint.end();
	QApplication::restoreOverrideCursor();
}

void MultiLayer::exportSVG(const QString &fname, int reso, const QSizeF &customSize, int unit, double fontsFactor)
{
    int maxw=0;
    int maxh=0;
    foreach(Graph *g, layersList())
    {
        if (g->pos().x()+g->geometry().width() > maxw) maxw=g->pos().x()+g->geometry().width();
        if (g->pos().y()+g->geometry().height() > maxh) maxh=g->pos().y()+g->geometry().height();
        
        foreach(LegendWidget *l, g->textsList())
        {
            if (l->pos().x()+l->geometry().width() > maxw) maxw=l->pos().x()+l->geometry().width();
            if (l->pos().y()+l->geometry().height() > maxh) maxh=l->pos().y()+l->geometry().height();
        }

        foreach(FrameWidget *f, g->enrichmentsList())
        {
            if (f->pos().x()+f->geometry().width() > maxw) maxw=f->pos().x()+f->geometry().width();
            if (f->pos().y()+f->geometry().height() > maxh) maxh=f->pos().y()+f->geometry().height();
        }
    }

	QSvgGenerator svg;
	svg.setFileName(fname);
    
    if (maxw>0 && maxh>0)
    {
        svg.setSize(QSize(maxw,maxh));
        svg.setResolution(defaultResolusion);
        svg.setViewBox(QRectF(0,0,double(maxw),double(maxh)));
        
        draw(&svg, customSize, unit, defaultResolusion, fontsFactor, reso);
    }
    else
    {
        svg.setSize(d_canvas->size());
        svg.setResolution(defaultResolusion);
        svg.setViewBox(QRectF(0,0,d_canvas->size().width(),d_canvas->size().height()));
        
        if (customSize.isValid())
        {
            QSize size = Graph::customPrintSize(customSize, unit, defaultResolusion);
            svg.setSize(size);
            svg.setViewBox(QRectF(0,0,size.width(),size.height()));
            draw(&svg, customSize, unit, defaultResolusion, fontsFactor);
        }
        else
            draw(&svg, QSize(d_canvas->size().width(), d_canvas->size().height()), unit, defaultResolusion,
                 fontsFactor);
    }
}

void MultiLayer::exportEMF(const QString& fname, const QSizeF& customSize, int unit, double fontsFactor)
{
	if (!applicationWindow())
		return;
	ImportExportPlugin *ep = applicationWindow()->exportPlugin("emf");
	if (!ep)
		return;
	ep->exportMultiLayerPlot(this, fname, customSize, unit, fontsFactor);
}

void MultiLayer::exportTeX(const QString& fname, bool color, bool escapeStrings, bool fontSizes, const QSizeF& customSize, int unit, double fontsFactor)
{
	int res = logicalDpiX();
	QSize size = d_canvas->size();
	if (customSize.isValid())
		size = Graph::customPrintSize(customSize, unit, res);

	QTeXPaintDevice tex(fname, size);
	tex.setEscapeTextMode(false);
	tex.exportFontSizes(fontSizes);
	if (!color)
		tex.setColorMode(QPrinter::GrayScale);

	foreach (Graph* g, graphsList){
		g->setTeXExportingMode();
		g->setEscapeTeXStringsMode(escapeStrings);
	}

	if (!fontSizes)
		fontsFactor = 1.0;

	draw(&tex, customSize, unit, res, fontsFactor);

	foreach (Graph* g, graphsList)
		g->setTeXExportingMode(false);
}

void MultiLayer::copyAllLayers()
{
	bool selectionOn = false;
	if (d_layers_selector){
		d_layers_selector->hide();
		selectionOn = true;
	}

	foreach (Graph* g, graphsList)
		g->deselectMarker();


/*#ifdef Q_OS_WIN
	if (OpenClipboard(0)){
		EmptyClipboard();

		QString path = QDir::tempPath();
		QString name = path + "/" + "qtiplot_clipboard.emf";
		name = QDir::cleanPath(name);

		exportEMF(name);
		HENHMETAFILE handle = GetEnhMetaFile(name.toStdWString().c_str());

		SetClipboardData(CF_ENHMETAFILE, handle);
		CloseClipboard();
		QFile::remove(name);
	}
#else */
    
	QApplication::clipboard()->setImage(canvasPixmap().toImage());
//#endif

	if (selectionOn)
		d_layers_selector->show();
}

void MultiLayer::printActiveLayer()
{
	if (active_graph)
		active_graph->print();
}

void MultiLayer::print()
{
	QPrinter printer;
	printer.setColorMode (QPrinter::Color);
	printer.setFullPage(true);
#ifdef Q_OS_LINUX
	printer.setOutputFileName(objectName());
#endif

	QRect canvasRect = d_canvas->rect();
	double aspect = double(canvasRect.width())/double(canvasRect.height());
	if (aspect < 1)
        printer.setPageOrientation(QPageLayout::Portrait);
	else
        printer.setPageOrientation(QPageLayout::Landscape);

	QPrintDialog printDialog(&printer, applicationWindow());
	if (printDialog.exec() == QDialog::Accepted){
	#ifdef Q_OS_LINUX
        if (printDialog.options() & QAbstractPrintDialog::PrintToFile)
        {
			QString fn = printer.outputFileName();
			if (printer.outputFormat() == QPrinter::PdfFormat && !fn.contains(".ps"))
				printer.setOutputFileName(fn + ".ps");
			else if (printer.outputFormat() == QPrinter::PdfFormat && !fn.contains(".pdf"))
				printer.setOutputFileName(fn + ".pdf");
		}
	#endif

		print(&printer);
	}
}

void MultiLayer::print(QPrinter *printer)
{
	if (!printer)
		return;

	QPainter paint(printer);
	printAllLayers(&paint);
	paint.end();
}

void MultiLayer::printAllLayers(QPainter *painter)
{
    if (!painter)
        return;

    auto printer = dynamic_cast<QPrinter *>(painter->device());
    QPageLayout layout = printer->pageLayout();
    QPageSize pageSize = layout.pageSize();

    // Get page size in points (portrait orientation)
    QSizeF sizePt = pageSize.size(QPageSize::Point);

    // Printer DPI (dots per inch)
    double printerDpiX = printer->logicalDpiX();
    double printerDpiY = printer->logicalDpiY();

    // Convert page size from points (1pt = 1/72 inch) to printer pixels
    int paperWidthPx = int(std::lround(sizePt.width() * printerDpiX / 72.0));
    int paperHeightPx = int(std::lround(sizePt.height() * printerDpiY / 72.0));

    // Adjust for orientation
    if (layout.orientation() == QPageLayout::Landscape)
        std::swap(paperWidthPx, paperHeightPx);

    QRect paperRectPx(0, 0, paperWidthPx, paperHeightPx);

    // Canvas rectangle (in screen pixels)
    QRectF canvasRect = d_canvas->rect();

    // Get screen DPI from canvas widget (assumed to be available)
    double screenDpiX = d_canvas->logicalDpiX();
    double screenDpiY = d_canvas->logicalDpiY();

    // Calculate scale factors between printer and screen DPI
    double dpiScaleX = printerDpiX / screenDpiX;
    double dpiScaleY = printerDpiY / screenDpiY;

    QRect cr = canvasRect.toRect(); // cropmarks rectangle

    if (d_scale_on_print)
    {
        // Define symmetric margin (1 cm in printer pixels)
        int marginPx = int(std::lround((1.0 / 2.54) * printerDpiY));

        // Define printable area after margins
        QRectF printableRect(marginPx, marginPx, paperRectPx.width() - 2 * marginPx,
                             paperRectPx.height() - 2 * marginPx);

        // Compute canvas size in printer pixels
        double canvasWidthPx = canvasRect.width() * dpiScaleX;
        double canvasHeightPx = canvasRect.height() * dpiScaleY;

        // Uniform scale to fit canvas inside printable rect
        double scaleFactor = std::min(printableRect.width() / canvasWidthPx, printableRect.height() / canvasHeightPx);

        double scaledCanvasWidth = canvasWidthPx * scaleFactor;
        double scaledCanvasHeight = canvasHeightPx * scaleFactor;

        // Compute offset to center scaled canvas inside printable rect
        double offsetX = printableRect.left() + (printableRect.width() - scaledCanvasWidth) / 2.0;
        double offsetY = printableRect.top() + (printableRect.height() - scaledCanvasHeight) / 2.0;

        if (d_print_cropmarks)
        {
            cr.moveTo(QPoint(int(offsetX), int(offsetY)));
            cr.setWidth(int(scaledCanvasWidth));
            cr.setHeight(int(scaledCanvasHeight));
        }

        for (Graph *g : graphsList)
        {
            QPoint pos = g->pos();
            QPoint targetPos(int(offsetX + pos.x() * dpiScaleX * scaleFactor),
                             int(offsetY + pos.y() * dpiScaleY * scaleFactor));

            int width = int(g->frameGeometry().width() * dpiScaleX * scaleFactor);
            int height = int(g->frameGeometry().height() * dpiScaleY * scaleFactor);

            g->print(painter, QRect(targetPos, QSize(width, height)), ScaledFontsPrintFilter(scaleFactor));
        }
    }
    else
    {
        // No scaling: center canvas with fallback margins (0.5 cm)
        int x_margin = (paperRectPx.width() - int(canvasRect.width() * dpiScaleX)) / 2;
        if (x_margin <= 0)
            x_margin = int(std::lround((0.5 / 2.54) * printerDpiY));

        int y_margin = (paperRectPx.height() - int(canvasRect.height() * dpiScaleY)) / 2;
        if (y_margin <= 0)
            y_margin = x_margin;

        if (d_print_cropmarks)
            cr.moveTo(x_margin, y_margin);

        for (Graph *g : graphsList)
        {
            QPoint pos = g->pos();
            QPoint targetPos(x_margin + int(pos.x() * dpiScaleX), y_margin + int(pos.y() * dpiScaleY));

            int width = int(g->width() * dpiScaleX);
            int height = int(g->height() * dpiScaleY);

            g->print(painter, QRect(targetPos, QSize(width, height)), ScaledFontsPrintFilter(1.0));
        }
    }

    if (d_print_cropmarks)
    {
        // Draw cropmarks around canvas rectangle
        cr.adjust(-1, -1, 2, 2);
        painter->save();
        painter->setPen(QPen(QColor(Qt::black), 0.5, Qt::DashLine));
        painter->drawLine(paperRectPx.left(), cr.top(), paperRectPx.right(), cr.top());
        painter->drawLine(paperRectPx.left(), cr.bottom(), paperRectPx.right(), cr.bottom());
        painter->drawLine(cr.left(), paperRectPx.top(), cr.left(), paperRectPx.bottom());
        painter->drawLine(cr.right(), paperRectPx.top(), cr.right(), paperRectPx.bottom());
        painter->restore();
    }
}

void MultiLayer::setFonts(const QFont& titleFnt, const QFont& scaleFnt,
		const QFont& numbersFnt, const QFont& legendFnt)
{
	foreach (Graph *g, graphsList){
		QwtText text = g->title();
  	    text.setFont(titleFnt);
  	    g->setTitle(text);

		for (int j= 0; j<QwtPlot::axisCnt; j++){
			g->setAxisFont(j, numbersFnt);
			text = g->axisTitle(j);
  	        text.setFont(scaleFnt);
  	        ((QwtPlot *)g)->setAxisTitle(j, text);
		}

		QList <LegendWidget *> texts = g->textsList();
		foreach (LegendWidget *l, texts)
			l->setFont(legendFnt);

		g->replot();
	}
	emit modifiedPlot();
}

void MultiLayer::connectLayer(Graph *g)
{
	ApplicationWindow *app = applicationWindow();
	if (app){
		connect(g, SIGNAL(selectionChanged(SelectionMoveResizer *)), app, SLOT(graphSelectionChanged(SelectionMoveResizer *)));
		connect(g, SIGNAL(showPlotDialog(int)), app, SLOT(showPlotDialog(int)));
		connect(g, SIGNAL(showContextMenu()), app, SLOT(showGraphContextMenu()));
		connect(g, SIGNAL(showMarkerPopupMenu()), app, SLOT(showMarkerPopupMenu()));
		connect(g, SIGNAL(viewTitleDialog()), app, SLOT(showTitleDialog()));
		connect(g, SIGNAL(showAxisTitleDialog()), app, SLOT(showAxisTitleDialog()));
		connect(g, SIGNAL(axisDblClicked(int)), app, SLOT(showScalePageFromAxisDialog(int)));
		connect(g, SIGNAL(showAxisDialog(int)), app, SLOT(showAxisPageFromAxisDialog(int)));
		connect(g, SIGNAL(enableTextEditor(Graph *)), app, SLOT(enableTextEditor(Graph *)));
		connect(g, SIGNAL(showCurveContextMenu(QwtPlotItem *)), app, SLOT(showCurveContextMenu(QwtPlotItem *)));
	}
	connect (g,SIGNAL(drawLineEnded(bool)), this, SIGNAL(drawLineEnded(bool)));
	connect (g,SIGNAL(viewLineDialog()),this,SIGNAL(showLineDialog()));
	connect (g,SIGNAL(cursorInfo(const QString&)),this,SIGNAL(cursorInfo(const QString&)));
	connect (g,SIGNAL(modifiedGraph()),this,SIGNAL(modifiedPlot()));
	connect (g,SIGNAL(modifiedGraph()),this,SLOT(notifyChanges()));
	connect (g,SIGNAL(selectedGraph(Graph*)),this, SLOT(setActiveLayer(Graph*)));
	connect (g,SIGNAL(selectedCanvas(Graph*)), this, SLOT(selectLayerCanvas(Graph*)));
	connect (g,SIGNAL(currentFontChanged(const QFont&)), this, SIGNAL(currentFontChanged(const QFont&)));
	connect (g,SIGNAL(currentColorChanged(const QColor&)), this, SIGNAL(currentColorChanged(const QColor&)));
	if (d_link_x_axes)
		connect(g, SIGNAL(axisDivChanged(Graph *, int)), this, SLOT(updateLayerAxes(Graph *, int)));
}

bool MultiLayer::eventFilter(QObject *object, QEvent *e)
{
	if(e->type() == QEvent::Resize && object == (QObject *)d_canvas){
		d_canvas->setUpdatesEnabled(false);
		resizeLayers((QResizeEvent *)e);
		d_canvas->setUpdatesEnabled(true);
		d_canvas_size = d_canvas->size();
	} else if (e->type() == QEvent::MouseButtonPress && object == (QObject *)d_canvas){
		const QMouseEvent *me = (const QMouseEvent *)e;
        if (me->button() == Qt::RightButton && applicationWindow())
        {
			applicationWindow()->showWindowContextMenu();
			return true;
        }
        else if (me->button() == Qt::MiddleButton)
			return QMdiSubWindow::eventFilter(object, e);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QPoint pos = d_canvas->mapFromGlobal(me->globalPos());
#else
        QPoint pos = d_canvas->mapFromGlobal(me->globalPosition().toPoint());
#endif
		// iterate backwards, so layers on top are preferred for selection
		QList<Graph*>::iterator i = graphsList.end();
		while (i != graphsList.begin()){
			--i;
			Graph *g = *i;
			if (g->hasSeletedItems()){
				g->deselect();
				return true;
			}

			QRect igeo = g->frameGeometry();
			if (igeo.contains(pos)){
				if (me->modifiers() & Qt::ShiftModifier){
					if (d_layers_selector)
						d_layers_selector->add((*i)->canvas());
					else {
						d_layers_selector = new SelectionMoveResizer(g->canvas());
						connect(d_layers_selector, SIGNAL(targetsChanged()), this, SIGNAL(modifiedPlot()));
					}
				} else {
					setActiveLayer(g);
					if (!g->mousePressed(e) && !d_layers_selector){
						d_layers_selector = new SelectionMoveResizer(g->canvas());
						connect(d_layers_selector, SIGNAL(targetsChanged()), this, SIGNAL(modifiedPlot()));
					}
				}
				return true;
			}
		}
		deselect();
	} else if (e->type() == QEvent::MouseButtonDblClick && object == (QObject *)d_canvas){
		if (applicationWindow())
			applicationWindow()->showPlotDialog(-100);
		return true;
	}

	return MdiSubWindow::eventFilter(object, e);
}

void MultiLayer::mouseReleaseEvent( QMouseEvent * e)
{
	d_layer_coordinates.clear();
	return QMdiSubWindow::mouseReleaseEvent(e);
}

void MultiLayer::showEvent (QShowEvent * e)
{
	d_layer_coordinates.clear();
	return QMdiSubWindow::showEvent(e);
}

void MultiLayer::keyPressEvent(QKeyEvent * e)
{
	if (e->key() == Qt::Key_Escape){
		if (d_layers_selector)
			delete d_layers_selector;
		else {
			foreach (Graph *g, graphsList)
				g->deselect();
		}
		return;
	}

	if (e->key() == Qt::Key_F12){
		if (d_layers_selector)
			delete d_layers_selector;
		int index = graphsList.indexOf(active_graph) + 1;
		if (index >= graphsList.size())
			index = 0;
		Graph *g = (Graph *)graphsList.at(index);
		if (g)
			setActiveLayer(g);
		return;
	}

	if (e->key() == Qt::Key_F10){
		deselect();
		int index = graphsList.indexOf(active_graph) - 1;
		if (index < 0)
			index = graphsList.size() - 1;
		Graph *g = (Graph *)graphsList.at(index);
		if (g)
			setActiveLayer(g);
		return;
	}

	if (e->key() == Qt::Key_F11 && applicationWindow()){
		applicationWindow()->showWindowContextMenu();
		return;
	}
}

void MultiLayer::wheelEvent ( QWheelEvent * e )
{
	QApplication::setOverrideCursor(Qt::WaitCursor);

	bool resize=false;
	QPoint aux;
	QSize intSize;
	Graph *resize_graph = 0;
	// Get the position of the mouse
    qreal xMouse = e->position().x();
    qreal yMouse = e->position().y();
	foreach (Graph *g, graphsList){
		intSize = g->size();
		aux = g->pos();
		if(xMouse>aux.x() && xMouse<(aux.x()+intSize.width())){
			if(yMouse>aux.y() && yMouse<(aux.y()+intSize.height())){
				resize_graph = g;
				resize = true;
			}
		}
	}
	if(resize && (e->modifiers()==Qt::AltModifier || e->modifiers()==Qt::ControlModifier || e->modifiers()==Qt::ShiftModifier))
	{
		intSize = resize_graph->size();
		if(e->modifiers() == Qt::AltModifier){// If alt is pressed then change the width
            if (e->angleDelta().y() > 0)
				intSize.rwidth() += 5;
            else if (e->angleDelta().y() < 0)
				intSize.rwidth() -= 5;
		} else if(e->modifiers() == Qt::ControlModifier){// If crt is pressed then changed the height
            if (e->angleDelta().y() > 0)
				intSize.rheight() += 5;
            else if (e->angleDelta().y() < 0)
				intSize.rheight() -= 5;
		} else if(e->modifiers() == Qt::ShiftModifier){// If shift is pressed then resize
            if (e->angleDelta().y() > 0)
            {
				intSize.rwidth() += 5;
				intSize.rheight() += 5;
            }
            else if (e->angleDelta().y() < 0)
            {
				intSize.rwidth() -= 5;
				intSize.rheight() -= 5;
			}
		}
		resize_graph->resize(intSize);
		emit modifiedPlot();
	}
	QApplication::restoreOverrideCursor();
}

bool MultiLayer::isEmpty ()
{
	if (graphsList.count() <= 0)
		return true;
	else
		return false;
}

void MultiLayer::save(const QString &fn, const QString &geometry, bool saveAsTemplate)
{
	QFile f(fn);
	if (!f.isOpen()){
		if (!f.open(QIODevice::Append))
			return;
	}
	QTextStream t( &f );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    t.setCodec("UTF-8");
#endif
	t << "<multiLayer>\n";

    bool notTemplate = !saveAsTemplate;
	if (notTemplate)
        t << QString(objectName())+"\t";
	t << QString::number(d_cols)+"\t";
	t << QString::number(d_rows)+"\t";
	if (notTemplate)
        t << birthDate()+"\n";
	t << geometry;
	if (notTemplate)
        t << "WindowLabel\t" + windowLabel() + "\t" + QString::number(captionPolicy()) + "\n";
	t << "Margins\t"+QString::number(left_margin)+"\t"+QString::number(right_margin)+"\t"+
		QString::number(top_margin)+"\t"+QString::number(bottom_margin)+"\n";
	t << "Spacing\t"+QString::number(rowsSpace)+"\t"+QString::number(colsSpace)+"\n";
    
    //+++ 2020
    int maxw=0;
    int maxh=0;
    foreach(Graph *g, layersList())
    {
        if (g->pos().x()+g->geometry().width() > maxw) maxw=g->pos().x()+g->geometry().width();
        if (g->pos().y()+g->geometry().height() > maxh) maxh=g->pos().y()+g->geometry().height();
        
        foreach(LegendWidget *l, g->textsList())
        {
            if (l->pos().x()+l->geometry().width() > maxw) maxw=l->pos().x()+l->geometry().width();
            if (l->pos().y()+l->geometry().height() > maxh) maxh=l->pos().y()+l->geometry().height();
        }
        
        foreach(FrameWidget *f, g->enrichmentsList())
        {
            if (f->pos().x()+f->geometry().width() > maxw) maxw=f->pos().x()+f->geometry().width();
            if (f->pos().y()+f->geometry().height() > maxh) maxh=f->pos().y()+f->geometry().height();
        }
    }
    //---
    t << "LayerCanvasSize\t"+QString::number(maxw)+"\t"+QString::number(maxh)+"\n";
	//++ 2020  t << "LayerCanvasSize\t"+QString::number(l_canvas_width)+"\t"+QString::number(l_canvas_height)+"\n";
    
	t << "Alignement\t"+QString::number(hor_align)+"\t"+QString::number(vert_align)+"\n";
	t << "<AlignPolicy>" + QString::number(d_align_policy) + "</AlignPolicy>\n";
	t << "<CommonAxes>" + QString::number(d_common_axes_layout) + "</CommonAxes>\n";

	foreach (Graph *g, graphsList)
		t << g->saveToString(saveAsTemplate);

	if (d_is_waterfall_plot)
		t << "<waterfall>1</waterfall>\n";

	t << "<LinkXAxes>" + QString::number(d_link_x_axes) + "</LinkXAxes>\n";
	t << "<ScaleLayers>" + QString::number(d_scale_layers) + "</ScaleLayers>\n";
	t << "</multiLayer>\n";
}

void MultiLayer::setMargins (int lm, int rm, int tm, int bm)
{
	if (left_margin != lm)
		left_margin = lm;
	if (right_margin != rm)
		right_margin = rm;
	if (top_margin != tm)
		top_margin = tm;
	if (bottom_margin != bm)
		bottom_margin = bm;
}

void MultiLayer::setSpacing (int rgap, int cgap)
{
	if (rowsSpace != rgap)
		rowsSpace = rgap;
	if (colsSpace != cgap)
		colsSpace = cgap;
}

void MultiLayer::setLayerCanvasSize (int w, int h)
{
	if (l_canvas_width != w)
		l_canvas_width = w;
	if (l_canvas_height != h)
		l_canvas_height = h;
}

void MultiLayer::setAlignement (int ha, int va)
{
	if (hor_align != ha)
		hor_align = ha;

	if (vert_align != va)
		vert_align = va;
}

void MultiLayer::setNumLayers(int n)
{
	if (graphsList.size() == n)
		return;

	int dn = graphsList.size() - n;
	if (dn > 0){
		for (int i = 0; i < dn; i++){//remove layer buttons
			LayerButton *btn=(LayerButton*)buttonsList.last();
			if (btn){
				btn->close();
				buttonsList.removeLast();
			}

			Graph *g = (Graph *)graphsList.last();
			if (g){//remove layers
				if (g->zoomOn() || g->activeTool())
					setPointerCursor();

				g->close();
				graphsList.removeLast();
			}
		}
		if (graphsList.size() <= 0){
			active_graph = 0;
			return;
		}

		// check whether the active Graph.has been deleted
		if(graphsList.indexOf(active_graph) == -1)
			active_graph=(Graph*) graphsList.last();
		for (int j=0;j<(int)graphsList.count();j++){
			Graph *gr=(Graph *)graphsList.at(j);
			if (gr == active_graph){
				LayerButton *button=(LayerButton *)buttonsList.at(j);
				button->setChecked(true);
				break;
			}
		}
	} else {
		for (int i = 0; i < abs(dn); i++)
			addLayer();
	}

	emit modifiedWindow(this);
}

void MultiLayer::copy(MultiLayer* ml)
{
    resize(ml->size());
	hide();

	setSpacing(ml->rowsSpacing(), ml->colsSpacing());
	setAlignement(ml->horizontalAlignement(), ml->verticalAlignement());
	setMargins(ml->leftMargin(), ml->rightMargin(), ml->topMargin(), ml->bottomMargin());
	d_size_policy = ml->sizePolicy();
	d_align_policy = ml->alignPolicy();
	d_scale_on_print = ml->scaleLayersOnPrint();
	d_print_cropmarks = ml->printCropmarksEnabled();

	QList<Graph*> lst = ml->layersList();
	foreach (Graph *g, lst){
		Graph* g2 = addLayer(g->pos().x(), g->pos().y(), g->width(), g->height());
		g2->setAutoscaleFonts(false);
		g2->copy(g);
		g2->setAutoscaleFonts(g->autoscaleFonts());
	}

	if (ml->isWaterfallPlot())
		setWaterfallLayout(true);

	foreach (Graph *g, lst){
		ImageProfilesTool *ipt = g->imageProfilesTool();
		if (ipt){
			Graph *g2 = layer(ml->layerIndex(g) + 1);
			if (g2)
				g2->setActiveTool(ipt->clone(g2));
			break;
		}
	}

	d_scale_layers = ml->scaleLayersOnResize();
	linkXLayerAxes(ml->hasLinkedXLayerAxes());
	d_common_axes_layout = ml->hasCommonAxes();
	show();
}

bool MultiLayer::swapLayers(int src, int dest)
{
	Graph *layerSrc = layer(src);
	Graph *layerDest = layer(dest);
	if (!layerSrc || !layerDest)
		return false;

	QRect rectSrc = layerSrc->geometry();
	QRect rectDest = layerDest->geometry();

	layerSrc->setGeometry(rectDest);
	layerDest->setGeometry(rectSrc);

	graphsList[src-1] = layerDest;
	graphsList[dest-1] = layerSrc;

	emit modifiedPlot();
	return true;
}

QString MultiLayer::sizeToString()
{
	int layers = graphsList.size();
	int size = sizeof(MultiLayer) + layers*sizeof(Graph);
	foreach(Graph *g, graphsList){
		QList<QwtPlotItem *> items = g->curvesList();
		foreach(QwtPlotItem *i, items){
			if (i->rtti() == QwtPlotItem::Rtti_PlotSpectrogram){
            	Spectrogram *sp = (Spectrogram *)i;
				int cells = sp->matrix()->numRows() * sp->matrix()->numCols();
            	size += cells*sizeof(double);
        	} else
				size += ((QwtPlotCurve *)i)->dataSize()*sizeof(double);
		}
	}
	return QString::number((double)size/1024.0, 'f', 1) + " " + tr("kB");
}

Graph* MultiLayer::layerAt(const QPoint& pos)
{
    foreach(Graph *g, graphsList){
		if (g->geometry().contains(pos))
            return g;
	}
	return nullptr;
}

bool MultiLayer::hasSelectedLayers()
{
    if (d_layers_selector)
        return true;
    return false;
}

void MultiLayer::setWaterfallLayout(bool on)
{
	if (graphsList.isEmpty())
		return;

	d_is_waterfall_plot = on;

	if (on){
		createWaterfallBox();
		updateWaterfalls();
	} else {
		for (int i = 0; i < waterfallBox->count(); i++){
			QLayoutItem *item = waterfallBox->itemAt(i);
			if (item){
				waterfallBox->removeItem(item);
				delete item;
			}
		}
	}
}

void MultiLayer::createWaterfallBox()
{
	if (waterfallBox->count() > 0)
		return;

	QPushButton *btn = new QPushButton(tr("Offset Amount..."));
	connect (btn, SIGNAL(clicked()), this, SLOT(showWaterfallOffsetDialog()));

	waterfallBox->addWidget(btn);
	btn = new QPushButton(tr("Reverse Order"));
	connect (btn, SIGNAL(clicked()), this, SLOT(reverseWaterfallOrder()));

	waterfallBox->addWidget(btn);
	btn = new QPushButton(tr("Fill Area..."));
	connect (btn, SIGNAL(clicked()), this, SLOT(showWaterfallFillDialog()));
	waterfallBox->addWidget(btn);
}

void MultiLayer::updateWaterfalls()
{
	if (!d_is_waterfall_plot || graphsList.isEmpty())
		return;

	foreach(Graph *g, graphsList){
		if (g->isWaterfallPlot())
			g->updateDataCurves();
	}
}

void MultiLayer::showWaterfallOffsetDialog()
{
	if (graphsList.isEmpty() || !active_graph)
		return;
	if (active_graph->curvesList().isEmpty())
		return;

	QDialog *offsetDialog = new QDialog(this);
	offsetDialog->setWindowTitle(tr("Offset Dialog"));

	QGroupBox *gb1 = new QGroupBox();
	QGridLayout *hl1 = new QGridLayout(gb1);

	hl1->addWidget(new QLabel(tr("Total Y Offset (%)")), 0, 0);
	QSpinBox *yOffsetBox = new QSpinBox();
	yOffsetBox->setRange(-INT_MAX, INT_MAX);
	yOffsetBox->setValue(active_graph->waterfallYOffset());
	hl1->addWidget(yOffsetBox, 0, 1);

	hl1->addWidget(new QLabel(tr("Total X Offset (%)")), 1, 0);
	QSpinBox *xOffsetBox = new QSpinBox();
	xOffsetBox->setRange(-INT_MAX, INT_MAX);
	xOffsetBox->setValue(active_graph->waterfallXOffset());
	hl1->addWidget(xOffsetBox, 1, 1);
	hl1->setRowStretch(2, 1);

	connect(yOffsetBox, SIGNAL(valueChanged(int)), active_graph, SLOT(setWaterfallYOffset(int)));
	connect(xOffsetBox, SIGNAL(valueChanged(int)), active_graph, SLOT(setWaterfallXOffset(int)));

	QPushButton *applyBtn = new QPushButton(tr("&Apply"));
	connect(applyBtn, SIGNAL(clicked()), this, SLOT(updateWaterfalls()));

	QPushButton *closeBtn = new QPushButton(tr("&Close"));
	connect(closeBtn, SIGNAL(clicked()), offsetDialog, SLOT(reject()));

	QHBoxLayout *hl2 = new QHBoxLayout();
	hl2->addStretch();
	hl2->addWidget(applyBtn);
	hl2->addWidget(closeBtn);

	QVBoxLayout *vl = new QVBoxLayout(offsetDialog);
	vl->addWidget(gb1);
	vl->addLayout(hl2);
	offsetDialog->exec();
}

void MultiLayer::reverseWaterfallOrder()
{
	if (graphsList.isEmpty() || !active_graph)
		return;

	active_graph->reverseCurveOrder();
	active_graph->updateDataCurves();
	emit modifiedWindow(this);
}

void MultiLayer::showWaterfallFillDialog()
{
	if (graphsList.isEmpty() || !active_graph)
		return;
	if (active_graph->curvesList().isEmpty())
		return;

	QDialog *waterfallFillDialog = new QDialog(this);
	waterfallFillDialog->setWindowTitle(tr("Fill Curves"));

	QGroupBox *gb1 = new QGroupBox(tr("Enable Fill"));
	gb1->setCheckable(true);

	QGridLayout *hl1 = new QGridLayout(gb1);
	hl1->addWidget(new QLabel(tr("Fill with Color")), 0, 0);
	ColorButton *fillColorBox = new ColorButton();
	hl1->addWidget(fillColorBox, 0, 1);

	QCheckBox *sideLinesBox = new QCheckBox(tr("Side Lines"));
	sideLinesBox->setChecked(active_graph->curve(0)->sideLinesEnabled());
	hl1->addWidget(sideLinesBox, 1, 0);
	hl1->setRowStretch(2, 1);

	QBrush brush = active_graph->curve(0)->brush();
	fillColorBox->setColor(brush.style() != Qt::NoBrush ? brush.color() : d_waterfall_fill_color);
	gb1->setChecked(brush.style() != Qt::NoBrush);

	connect(gb1, SIGNAL(toggled(bool)), active_graph, SLOT(updateWaterfallFill(bool)));
	connect(fillColorBox, SIGNAL(colorChanged(const QColor&)), this, SLOT(setWaterfallFillColor(const QColor&)));
	connect(sideLinesBox, SIGNAL(toggled(bool)), active_graph, SLOT(setWaterfallSideLines(bool)));

	QPushButton *closeBtn = new QPushButton(tr("&Close"));
	connect(closeBtn, SIGNAL(clicked()), waterfallFillDialog, SLOT(reject()));

	QHBoxLayout *hl2 = new QHBoxLayout();
	hl2->addStretch();
	hl2->addWidget(closeBtn);

	QVBoxLayout *vl = new QVBoxLayout(waterfallFillDialog);
	vl->addWidget(gb1);
	vl->addLayout(hl2);
	waterfallFillDialog->exec();
}

void MultiLayer::setWaterfallFillColor(const QColor& c)
{
	d_waterfall_fill_color = c;
	if (active_graph)
		active_graph->setWaterfallFillColor(c);
}

void MultiLayer::dragEnterEvent( QDragEnterEvent* e )
{
	Graph *g = qobject_cast<Graph*>(e->source());
	if (!g || g->multiLayer() == this)
		return;

	if (e->mimeData()->hasFormat("text/plain"))
		e->acceptProposedAction();
}

void MultiLayer::dropEvent(QDropEvent* event)
{
	Graph *g = qobject_cast<Graph*>(event->source());
	if (!g)
		return;

	if (g->multiLayer() == this)
		return;

	QStringList lst = event->mimeData()->text().split(";");

	QPoint pos = d_canvas->mapFromGlobal(QCursor::pos());
	pos = QPoint(pos.x() - lst[0].toInt(), pos.y() - lst[1].toInt());
	Graph *clone = addLayer(pos.x(), pos.y(), g->width(), g->height());
	if (clone)
		clone->copy(g);
}

void MultiLayer::plotProfiles(Matrix* m)
{
	if (!m)
		return;

	double mmin, mmax;
	m->range(&mmin, &mmax);
	mmin = floor(mmin);
	mmax = ceil(mmax);

    double scaleFactor = this->applicationWindow()->d_layer_canvas_height / (380.0 + 15.0 + 100.0);
    double spectrogramWidthFactor = 1.0;
    if (m->numRows() > 0 && m->numRows() != m->numCols())
        spectrogramWidthFactor *= static_cast<double>(m->numCols()) / static_cast<double>(m->numRows());

	Graph* g = addLayer();
    Spectrogram *s = g->plotSpectrogram(m, Graph::ColorMap);
	if (!s)
		return;

    s->clearLabels();
    g->enableAxis(QwtPlot::yLeft, false);
    g->enableAxis(QwtPlot::xTop, false);
    g->setScale(QwtPlot::xTop, qMin(m->xStart(), m->xEnd()), qMax(m->xStart(), m->xEnd()));
    g->setScale(QwtPlot::xBottom, qMin(m->xStart(), m->xEnd()), qMax(m->xStart(), m->xEnd()), 0.0, 10, 5, Graph::Linear,
                false);
    g->enableAxis(QwtPlot::xBottom, true);
	g->enableAxis(QwtPlot::yRight, false);
    g->setScale(QwtPlot::yLeft, qMin(m->yStart(), m->yEnd()), qMax(m->yStart(), m->yEnd()), 0.0, 10, 5, Graph::Linear,
                false);
    g->setAxisTitle(QwtPlot::yLeft, QString());
    g->setAxisTitle(QwtPlot::xTop, QString());
    g->setTitle(QString());
    g->setAxisLabelAlignment(QwtPlot::xBottom, Qt::AlignCenter | Qt::AlignVCenter);
	g->enableAutoscaling(false);
    g->setCanvasGeometry(QRect(static_cast<int>(scaleFactor * 40), static_cast<int>(scaleFactor * (10 + 100 + 15)),
                               static_cast<int>(scaleFactor * spectrogramWidthFactor * 380),
                               static_cast<int>(scaleFactor * 380)));
    g->setCanvasGeometry(QRect(static_cast<int>(scaleFactor * 40), static_cast<int>(scaleFactor * (10 + 100 + 15)),
                               static_cast<int>(scaleFactor * spectrogramWidthFactor * 380),
                               static_cast<int>(scaleFactor * 380)));

	g = addLayer();

    g->enableAxis(QwtPlot::yLeft, false);
    g->setScale(QwtPlot::yLeft, mmin, mmax);
    g->enableAxis(QwtPlot::xBottom, false);
	g->setScale(QwtPlot::xBottom, qMin(m->xStart(), m->xEnd()), qMax(m->xStart(), m->xEnd()));
	g->enableAxisLabels(QwtPlot::xBottom, false);

    g->enableAxis(QwtPlot::yRight, true);
    g->setScale(QwtPlot::yRight, mmin, mmax);
    g->setAxisLabelAlignment(QwtPlot::yRight, Qt::AlignRight | Qt::AlignVCenter);
    g->setAxisTitle(QwtPlot::yLeft, QString());
    g->setAxisTitle(QwtPlot::yRight, QString());
    g->setAxisTitle(QwtPlot::xBottom, QString());
    g->setTitle(QString());
	g->enableAutoscaling(false);
    g->setCanvasGeometry(QRect(static_cast<int>(scaleFactor * 40), static_cast<int>(scaleFactor * 10),
                               static_cast<int>(scaleFactor * spectrogramWidthFactor * 380),
                               static_cast<int>(scaleFactor * 100)));

	g = addLayer();

    g->enableAxis(QwtPlot::xBottom, true);
    g->setScale(QwtPlot::xBottom, mmin, mmax);
    g->setAxisLabelRotation(QwtPlot::xBottom, 90);

    g->setScale(QwtPlot::xTop, mmin, mmax);
    g->enableAxis(QwtPlot::xTop, false);

    g->enableAxis(QwtPlot::yRight, true);
    g->setScale(QwtPlot::yRight, qMin(m->yStart(), m->yEnd()), qMax(m->yStart(), m->yEnd()), 0.0, 10, 5, Graph::Linear,
                false);
    g->setAxisLabelAlignment(QwtPlot::yRight, Qt::AlignRight | Qt::AlignVCenter);
    g->setScale(QwtPlot::yLeft, qMin(m->yStart(), m->yEnd()), qMax(m->yStart(), m->yEnd()), 0.0, 10, 5, Graph::Linear,
                false);
    g->enableAxis(QwtPlot::yLeft, false);

    g->setAxisTitle(QwtPlot::yLeft, QString());
    g->setAxisTitle(QwtPlot::xTop, QString());
    g->setAxisTitle(QwtPlot::xBottom, QString());
    g->setTitle(QString());
    g->enableAutoscaling(false);
    g->setCanvasGeometry(QRect(static_cast<int>(scaleFactor * (40 + 15 + spectrogramWidthFactor * 380)),
                               static_cast<int>(scaleFactor * (10 + 100 + 15)), static_cast<int>(scaleFactor * 100),
                               static_cast<int>(scaleFactor * 380)));
	QColor color = Qt::white;
	color.setAlpha(0);
	foreach(Graph *g, graphsList)
		g->setBackgroundColor(color);
}

void MultiLayer::linkXLayerAxes(bool link)
{
	d_link_x_axes = link;

	if (link){
		foreach(Graph *g, graphsList)
			connect(g, SIGNAL(axisDivChanged(Graph *, int)), this, SLOT(updateLayerAxes(Graph *, int)));
	} else {
		foreach(Graph *g, graphsList)
			disconnect(g, SIGNAL(axisDivChanged(Graph *, int)), this, SLOT(updateLayerAxes(Graph *, int)));
	}
}

void MultiLayer::updateLayerAxes(Graph *g, int axis)
{
	if (!g || (axis != Graph::xBottom && axis != Graph::xTop))
		return;

	ScaleEngine *se = (ScaleEngine *)g->axisScaleEngine (axis);
	const QwtScaleDiv *sd = g->axisScaleDiv(axis);
	if (!se || !sd)
		return;

	int majorTicks = g->axisMaxMajor(axis);
	int minorTicks = g->axisMaxMinor(axis);
	double step = g->axisStep(axis);

	int oppositeAxis = Graph::xTop;
	if (axis == Graph::xTop)
		oppositeAxis = Graph::xBottom;

	bool synchronizeScales = true;
	if (this->applicationWindow())
		synchronizeScales = this->applicationWindow()->d_synchronize_graph_scales;

	foreach(Graph *l, graphsList){
		if (l == g)
			continue;

		l->blockSignals(true);
		l->setScale(axis, sd->lowerBound(), sd->upperBound(), step, majorTicks, minorTicks,
					se->type(), se->testAttribute(QwtScaleEngine::Inverted),
					se->axisBreakLeft(), se->axisBreakRight(), se->breakPosition(),
					se->stepBeforeBreak(), se->stepAfterBreak(), se->minTicksBeforeBreak(),
					se->minTicksAfterBreak(), se->log10ScaleAfterBreak(), se->breakWidth(), se->hasBreakDecoration());
		if (synchronizeScales){
			l->setScale(oppositeAxis, sd->lowerBound(), sd->upperBound(), step, majorTicks, minorTicks,
					se->type(), se->testAttribute(QwtScaleEngine::Inverted),
					se->axisBreakLeft(), se->axisBreakRight(), se->breakPosition(),
					se->stepBeforeBreak(), se->stepAfterBreak(), se->minTicksBeforeBreak(),
					se->minTicksAfterBreak(), se->log10ScaleAfterBreak(), se->breakWidth(), se->hasBreakDecoration());
		}

		l->replot();
		l->blockSignals(false);
	}

	if (synchronizeScales){
		g->blockSignals(true);
		g->setScale(oppositeAxis, sd->lowerBound(), sd->upperBound(), step, majorTicks, minorTicks,
				se->type(), se->testAttribute(QwtScaleEngine::Inverted),
				se->axisBreakLeft(), se->axisBreakRight(), se->breakPosition(),
				se->stepBeforeBreak(), se->stepAfterBreak(), se->minTicksBeforeBreak(),
				se->minTicksAfterBreak(), se->log10ScaleAfterBreak(), se->breakWidth(), se->hasBreakDecoration());
		g->blockSignals(false);
		g->replot();
	}
}

void MultiLayer::updateLayersLayout(Graph *g)
{
	if (!g || g != graphsList.last())
		return;

	disconnect (g, SIGNAL(updatedLayout(Graph *)), this, SLOT(updateLayersLayout(Graph *)));
	arrangeLayers(false, true);
	foreach(Graph *ag, graphsList){
		if (ag->curveCount())
			ag->newLegend();
	}
}

void MultiLayer::deselect()
{
    if (d_layers_selector)
        d_layers_selector->hide();
}

//+
void MultiLayer::updateMagicMenu()
{
    magicMenu->clear();
    magicMenu->addSeparator();
    for (int i = 0; i < applicationWindow()->magicList.count(); i++)
        magicMenu->addAction(applicationWindow()->magicList[i]);
    magicMenu->addSeparator();
}

void MultiLayer::magicMenuSelected(QAction *action)
{
    int id = magicTemplate->menu()->actions().indexOf(action);
	id--;
    ApplicationWindow *app = applicationWindow();
    app->setMagicTemplate(app->sasPath+"/templates/"+app->magicList[id]);
}
//-


MultiLayer::~MultiLayer()
{
	deselect();

	foreach(Graph *g, graphsList)
		delete g;
}

