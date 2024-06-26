/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Knut Franke <knut.franke@gmx.de>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Plot tool for selecting arbitrary points
 ******************************************************************************/

#ifndef SCREENPICKERTOOL_H
#define SCREENPICKERTOOL_H

#include <QLabel>
#include <QObject>
#include <QPointer>
#include <QSpinBox>

#include <qwt/qwt_double_rect.h>
#include <qwt/qwt_plot_marker.h>
#include <qwt/qwt_plot_picker.h>

#include "ApplicationWindow.h"
#include "DoubleSpinBox.h"
#include "Matrix.h"
#include "PlotCurve.h"
#include "PlotToolInterface.h"
#include "Table.h"

/*!Plot tool for selecting arbitrary points.
 *
 * This is a rather thin wrapper around QwtPlotPicker, providing selection of points
 * on a Graph/Plot and displaying coordinates.
 */
class ScreenPickerTool : public QwtPlotPicker, public PlotToolInterface
{
	Q_OBJECT
	public:
		enum MoveRestriction { NoRestriction, Vertical, Horizontal };
		ScreenPickerTool(Graph *graph, const QObject *status_target=nullptr, const char *status_slot="");
		virtual ~ScreenPickerTool();
		virtual void append(const QwtDoublePoint &pos);
		void setMoveRestriction(ScreenPickerTool::MoveRestriction r){d_move_restriction = r;};

		double xValue(){return d_selection_marker.xValue();};
		double yValue(){return d_selection_marker.yValue();};

	signals:
		/*! Emitted whenever a new message should be presented to the user.
		 *
		 * You don't have to connect to this signal if you alreay specified a reciever during initialization.
		 */
		void statusText(const QString&);
	protected:
        virtual bool eventFilter(QObject *obj, QEvent *event);
		virtual void append(const QPoint &point);
		QwtPlotMarker d_selection_marker;
		MoveRestriction d_move_restriction;
};

/*!Plot tool for drawing arbitrary points.
 *
 */
class DrawPointTool : public ScreenPickerTool
{
	Q_OBJECT
	public:
		DrawPointTool(ApplicationWindow *app, Graph *graph, const QObject *status_target=nullptr, const char *status_slot="");
		virtual int rtti() const { return Rtti_DrawDataPoints;};

	protected:
        virtual bool eventFilter(QObject *obj, QEvent *event);
		void appendPoint(const QwtDoublePoint &point);
		DataCurve *d_curve;
		Table *d_table;
		ApplicationWindow *d_app;
};

/*!Plot tool for image analysis.
 *
 */
class ImageProfilesTool : public ScreenPickerTool
{
	Q_OBJECT
	public:
		ImageProfilesTool(ApplicationWindow *app, Graph *graph, Matrix *m, Table *horTable, Table *verTable);
		void connectPlotLayers();

		ImageProfilesTool* clone(Graph *g);

		virtual ~ImageProfilesTool();
		virtual void append(const QwtDoublePoint &pos);
		virtual int rtti() const { return Rtti_ImageProfilesTool;};

		int averagePixels(){return averageBox->value();}
		void setAveragePixels(int pixels);

		QPointer<Matrix> matrix(){return d_matrix;};
		QPointer<Table> horizontalTable(){return d_hor_table;};
		QPointer<Table> verticalTable(){return d_ver_table;};

	private slots:
		void modifiedMatrix(Matrix *);
		void updateCursorPosition();
		void updateCursorWidth(int width);

	private:
		void setCursorWidth(int width);

	protected:
		ApplicationWindow *d_app;
		QPointer<Matrix> d_matrix;
		QPointer<Table> d_hor_table, d_ver_table;
		DoubleSpinBox *horSpinBox, *vertSpinBox;
		QSpinBox *averageBox;
		QLabel *zLabel;
		QWidget *d_box;
};

#endif
