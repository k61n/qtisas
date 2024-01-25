/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Plot tool for substracting a straight line
 ******************************************************************************/

#ifndef SUBSTRACT_LINE_TOOL
#define SUBSTRACT_LINE_TOOL

#include "PlotToolInterface.h"
#include <QObject>
#include <QLineF>

#include "ScreenPickerTool.h"

class ApplicationWindow;
class QwtPlotCurve;
class QwtPlotMarker;

/*! Plot tool for substracting a straight line.
 */
class SubtractLineTool : public QObject, public PlotToolInterface
{
	Q_OBJECT
	public:
		SubtractLineTool(Graph *graph, ApplicationWindow *app, const QObject *status_target, const char *status_slot);
		virtual ~SubtractLineTool();

		virtual int rtti() const {return PlotToolInterface::Rtti_SubtractLineTool;};
	signals:
		/*! Emitted whenever a new message should be presented to the user.
		 *
		 * You don't have to connect to this signal if you alreay specified a reciever during initialization.
		 */
		void statusText(const QString&);
	protected slots:
		void selectPoint(const QwtDoublePoint &pos);

	private:
		void finalize();
		int d_selected_points;
		ScreenPickerTool *d_picker_tool;
		QwtPlotMarker *d_first_point_marker;
		QLineF d_line;
};

#endif
