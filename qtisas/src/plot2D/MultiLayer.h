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

#ifndef MULTILAYER_H
#define MULTILAYER_H

#include <QLabel>
#include <QLayout>
#include <QPointer>
#include <QPushButton>
#include <QTextDocument>
#include <QToolButton>

#include "FrameWidget.h"
#include "Graph.h"
#include "LegendWidget.h"
#include "Matrix.h"
#include "MdiSubWindow.h"
#include "SelectionMoveResizer.h"

class LayerButton;

/**
 * \brief An MDI window (MdiSubWindow) managing one or more Graph objects.
 *
 * %Note that several parts of the code, as well as the user interface, refer to MultiLayer as "graph" or "plot",
 * practically guaranteeing confusion with the classes Graph and Plot.
 *
 * \section future Future Plans
 * Manage any QWidget instead of only Graph.
 * This would allow 3D graphs to be added as well, so you could produce mixed 2D/3D arrangements.
 * It would also allow text labels to be added directly instead of having to complicate things by wrapping them
 * up in a Graph (see documentation of ImageMarker for details) (see documentation of ImageMarker for details).
 *
 * The main problem to be figured out for this is how Graph would interface with the rest of the project.
 * A possible solution is outlined in the documentation of ApplicationWindow:
 * If MultiLayer exposes its parent Project to the widgets it manages, they could handle things like creating
 * tables by calling methods of Project instead of sending signals.
 */
class MultiLayer: public MdiSubWindow
{
	Q_OBJECT

public:
    explicit MultiLayer(ApplicationWindow *parent = nullptr, int layers = 1, int rows = 1, int cols = 1,
                        const QString &label = "", const char *name = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	~MultiLayer();

	QList<Graph *> layersList(){return graphsList;};
	QList<Graph*> stackOrderedLayersList();
	Graph *layer(int num);
	int layerIndex(Graph *g){return graphsList.indexOf(g);};

    int numLayers(){return graphsList.size();};
    void setNumLayers(int n);

	void copy(MultiLayer* ml);

	enum HorAlignement{HCenter, Left, Right};
	enum VertAlignement{VCenter, Top, Bottom};
	enum AlignPolicy{AlignLayers = 0, AlignCanvases};
	enum SizePolicy{Expanding = 0, UserSize};

	bool scaleLayersOnPrint(){return d_scale_on_print;};
	void setScaleLayersOnPrint(bool on){d_scale_on_print = on;};

	bool printCropmarksEnabled(){return d_print_cropmarks;};
	void printCropmarks(bool on){d_print_cropmarks = on;};

	bool scaleLayersOnResize(){return d_scale_layers;};
	void setScaleLayersOnResize(bool ok){d_scale_layers = ok;};

	QWidget *canvas(){return d_canvas;};
	QRect canvasRect(){return d_canvas->rect();};
	QRect canvasChildrenRect();
	virtual QString sizeToString();

	void setWaterfallLayout(bool on = true);

	void setEqualSizedLayers();

	void plotProfiles(Matrix* m);

	QHBoxLayout *toolBox(){return toolbuttonsBox;};

	AlignPolicy alignPolicy(){return d_align_policy;};
	void setAlignPolicy(const AlignPolicy& policy){d_align_policy = policy;};

	SizePolicy sizePolicy(){return d_size_policy;};
	void setSizePolicy(const SizePolicy& policy){d_size_policy = policy;};

	void setCommonLayerAxes(bool verticalAxis = true, bool horizontalAxis = true);
	void linkXLayerAxes(bool link = true);
	bool hasLinkedXLayerAxes(){return d_link_x_axes;};
	bool hasCommonAxes(){return d_common_axes_layout;};
	void setCommonAxesLayout(bool on = true){d_common_axes_layout = on;};

	void deselect();
	bool hasSelectedLayers();
	bool isLayerSelected(Graph*);

	void adjustLayersToCanvasSize();
    //+++//
    QToolButton *d_loglog, *d_linlin;
    QToolButton *magicTemplate;
    QMenu *magicMenu;
    //---//
public slots:
	Graph* addLayer(int x = 0, int y = 0, int width = 0, int height = 0, bool = false);

	bool isEmpty();
    bool removeLayer(Graph *g);
    bool removeActiveLayer();
	void confirmRemoveLayer();

	Graph* activeLayer(){return active_graph;};
	void setActiveLayer(Graph* g);
	void activateGraph(LayerButton* button);
	void selectLayerCanvas(Graph* g);

    //! Returns the layer at the given position; returns 0 if there is no such layer.
	Graph* layerAt(const QPoint& pos);
	void setGraphGeometry(int x, int y, int w, int h);

	void findBestLayout(int &rows, int &cols);

	QSize arrangeLayers(bool userSize);
	bool arrangeLayers(bool fit, bool userSize);
	bool swapLayers(int src, int dest);
	void updateLayersLayout(Graph *);

	int getRows(){return d_rows;};
	void setRows(int r);

	int getCols(){return d_cols;};
	void setCols(int c);

	int colsSpacing(){return colsSpace;};
	int rowsSpacing(){return rowsSpace;};
	void setSpacing (int rgap, int cgap);

	int leftMargin(){return left_margin;};
	int rightMargin(){return right_margin;};
	int topMargin(){return top_margin;};
	int bottomMargin(){return bottom_margin;};
	void setMargins (int lm, int rm, int tm, int bm);

	QSize layerCanvasSize(){return QSize(l_canvas_width, l_canvas_height);};
	void setLayerCanvasSize (int w, int h);

	int horizontalAlignement(){return hor_align;};
	int verticalAlignement(){return vert_align;};
	void setAlignement (int ha, int va);

	//! \name Print and Export
	//@{
	QPixmap canvasPixmap(const QSize& size = QSize(), double scaleFontsFactor = 1.0, bool transparent = false);
	void exportToFile(const QString& fileName);
	void exportImage(QTextDocument *document, int quality = 100, bool transparent = false,
		int dpi = 0, const QSizeF& customSize = QSizeF (), int unit = FrameWidget::Pixel, double fontsFactor = 1.0);
	void exportImage(const QString& fileName, int quality = 100, bool transparent = false, int dpi = 0,
		const QSizeF& customSize = QSizeF (), int unit = FrameWidget::Pixel, double fontsFactor = 1.0, int compression = 0);
    QByteArray arraySVG(int reso = defaultResolution, const QSizeF &customSize = QSizeF(),
                         int unit = FrameWidget::Pixel, double fontsFactor = 1.0);
    bool exportSVG(const QString &fname, int reso = defaultResolution, const QSizeF &customSize = QSizeF(),
                   int unit = FrameWidget::Pixel, double fontsFactor = 1.0);
    void exportPDF(const QString& fname);
	void exportVector(const QString& fileName, bool fontEmbedding, int res = 0, bool color = true,
		const QSizeF& customSize = QSizeF (), int unit = FrameWidget::Pixel, double fontsFactor = 1.0);
	void exportVector(QPrinter *printer, bool fontEmbedding, int res = 0, bool color = true,
		const QSizeF& customSize = QSizeF (), int unit = FrameWidget::Pixel, double fontsFactor = 1.0);

    void draw(QPaintDevice *, const QSizeF &customSize, int unit, int res, double fontsFactor = 1.0,
              int reso = defaultResolution);

	void exportEMF(const QString& fname, const QSizeF& customSize = QSizeF(), int unit = FrameWidget::Pixel, double fontsFactor = 1.0);
	void exportTeX(const QString& fname, bool color = true, bool escapeStrings = true, bool fontSizes = true,
					const QSizeF& customSize = QSizeF(), int unit = FrameWidget::Pixel, double fontsFactor = 1.0);

	void copyAllLayers();
	void print();
	void print(QPrinter *);
	void printAllLayers(QPainter *painter);
	void printActiveLayer();
	//@}

	void setFonts(const QFont& titleFnt, const QFont& scaleFnt,
							const QFont& numbersFnt, const QFont& legendFnt);

	void connectLayer(Graph *g);

	void save(const QString& fn, const QString& geometry, bool = false);

    //! \name Waterfall Plots
	//@{
    void showWaterfallOffsetDialog();
    void reverseWaterfallOrder();
    void showWaterfallFillDialog();
	void updateWaterfalls();
	bool isWaterfallPlot(){return d_is_waterfall_plot;}
	QColor waterfallFillColor(){return d_waterfall_fill_color;}
	void setWaterfallFillColor(const QColor& c);
    //@}

	void updateLayerAxes(Graph *g, int axis);

    //+
    void updateMagicMenu();
    void magicMenuSelected(QAction *action);
    //-
signals:
	void showEnrichementDialog();
	void showCurvesDialog();
	void drawLineEnded(bool);
	void showMarkerPopupMenu();
	void modifiedPlot();
	void cursorInfo(const QString&);
	void showLineDialog();
	void pasteMarker();
	void setPointerCursor();
	void currentFontChanged(const QFont&);
	void currentColorChanged(const QColor&);

private:
	//! \name Event Handlers
	//@{
	void mouseReleaseEvent(QMouseEvent *);
	void showEvent(QShowEvent *);
	void dropEvent(QDropEvent*);
	void dragEnterEvent(QDragEnterEvent*);
	void wheelEvent(QWheelEvent *);
	void keyPressEvent(QKeyEvent *);
	bool eventFilter(QObject *object, QEvent *);
	void releaseLayer();
	void resizeLayers(QResizeEvent *);
	//! Try to guess on which layer the user clicked if the layers are superposed
	Graph* clickedLayer(Graph*);
	//@}

	LayerButton* addLayerButton();
	void createWaterfallBox();

	Graph* active_graph;
	//! Used for resizing of layers.
	int d_cols, d_rows, graph_width, graph_height, colsSpace, rowsSpace;
	int left_margin, right_margin, top_margin, bottom_margin;
	int l_canvas_width, l_canvas_height, hor_align, vert_align;
	bool d_scale_on_print, d_print_cropmarks;
    //! Flag telling if layers should be rescaled on the plot window is resized by the user.
	bool d_scale_layers;

    QList<LayerButton *> buttonsList;
    QList<Graph *> graphsList;
	QHBoxLayout *layerButtonsBox, *waterfallBox, *toolbuttonsBox;
    QWidget *d_canvas;

	QPointer<SelectionMoveResizer> d_layers_selector;

	bool d_is_waterfall_plot;
	QColor d_waterfall_fill_color;

	QToolButton *d_add_layer_btn, *d_remove_layer_btn;


    
	QSize d_canvas_size;

	AlignPolicy d_align_policy;
	SizePolicy d_size_policy;
	bool d_link_x_axes;
	bool d_common_axes_layout;

	QVector<QRectF> d_layer_coordinates;
};

//! Button with layer number
class LayerButton: public QPushButton
{
	Q_OBJECT

public:
    explicit LayerButton(const QString &text = QString(), QWidget *parent = nullptr);
	static int btnSize(){return 20;};

protected:
	void mousePressEvent( QMouseEvent * );
	void mouseDoubleClickEvent ( QMouseEvent * );

signals:
	void showCurvesDialog();
	void clicked(LayerButton*);
	void showLayerContextMenu();
};

#endif
