/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2007 Knut Franke <knut.franke@gmx.de>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Plot tool for selecting individual points of a curve
 ******************************************************************************/

#ifndef DATAPICKERTOOL_H
#define DATAPICKERTOOL_H

#include <QPoint>

#include <qwt/qwt_plot_marker.h>
#include <qwt/qwt_plot_picker.h>

#include "ApplicationWindow.h"
#include "PlotToolInterface.h"

//! Plot tool for selecting, moving or removing individual points of a curve.
class DataPickerTool : public QwtPlotPicker, public PlotToolInterface
{
	Q_OBJECT
	public:
		enum Mode { Display, Move, Remove, MoveCurve};
		enum MoveMode {Free, Vertical, Horizontal};
		DataPickerTool(Graph *graph, ApplicationWindow *app, Mode mode, const QObject *status_target=nullptr, const char *status_slot="");
		virtual ~DataPickerTool();
		virtual bool eventFilter(QObject *obj, QEvent *event);
		bool keyEventFilter(QKeyEvent *ke);
		QwtPlotCurve *selectedCurve() const { return d_selected_curve; }
		void setSelectedCurve(QwtPlotCurve *c){if (c) setSelection(c, 0);};
		int selectedPointIndex(){return d_selected_point;};

        void copySelection();
        void cutSelection();
        void pasteSelection();
        void removePoint();

        //! Returns the information about the functioning mode of the tool
        Mode mode(){return d_mode;};
        void setMode(Mode m){d_mode = m;};

        //! Searches the index of the closest point to the given x coordinate
        int findClosestPoint(QwtPlotCurve *c, double x, bool up);

        virtual int rtti() const {return PlotToolInterface::Rtti_DataPicker;};

        void selectTableRow();

		ApplicationWindow *applicationWindow(){return d_app;};

	public slots:
		void pasteSelectionAsLayerText();

	signals:
		/*! Emitted whenever a new message should be presented to the user.
		 *
		 * You don't have to connect to this signal if you alreay specified a reciever during initialization.
		 */
		void statusText(const QString&);
		//! Emitted whenever a new data point has been selected.
		void selected(QwtPlotCurve*, int);
	protected:
		void movePoint(const QPoint &cursor);
		virtual void append(const QPoint &point);
		virtual void move(const QPoint &point);
		virtual bool end(bool ok);
		virtual void setSelection(QwtPlotCurve *curve, int point_index);
		void moveBy(int dx, int dy);

		ApplicationWindow *d_app;
		QwtPlotMarker d_selection_marker;
		Mode d_mode;
		QwtPlotCurve *d_selected_curve;
		int d_selected_point;
		MoveMode d_move_mode;
		QPoint d_restricted_move_pos;
};

//! Plot tool for selecting and moving individual points of a baseline curve.
class BaselineTool : public DataPickerTool
{
	Q_OBJECT

	public:
		BaselineTool(QwtPlotCurve *curve, Graph *graph, ApplicationWindow *app);

	protected:
		void setSelection(QwtPlotCurve *curve, int point_index);
};

#endif
