/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2007 Knut Franke <knut.franke@gmx.de>
Description: Plot tool for translating curves
 ******************************************************************************/

#ifndef TRANSLATE_CURVE_TOOL_H
#define TRANSLATE_CURVE_TOOL_H

#include "PlotToolInterface.h"
#include <QObject>
#include <qwt_double_rect.h>

class ApplicationWindow;
class QwtPlotCurve;

//! Plot tool for translating curves.
class TranslateCurveTool : public QObject, public PlotToolInterface
{
	Q_OBJECT
	public:
		enum Direction { Vertical, Horizontal };
		/*!\brief Standard constructor.
		 * \param graph the Graph to operate on (or rather, on whose image markers to operate on)
		 * \param app parent window of graph
		 * \param dir the direction in which to translate curves
		 * \param status_target target to which the statusText(const QString&) signal will be connected
		 * \param status_slot slot on status_target to which the statusText(const QString&) signal will be connected
		 * The status_target/status_slot arguments are provided, because statusText(const QString&) is emitted
		 * during initialization, before there's any other chance of connecting to it.
		 */
		TranslateCurveTool(Graph *graph, ApplicationWindow *app, Direction dir, const QObject *status_target=nullptr, const char *status_slot="");

        virtual int rtti() const {return PlotToolInterface::Rtti_TranslateCurveTool;};
	signals:
		/*!\brief Emitted whenever a new message should be presented to the user.
		 *
		 * You don't have to connect to this signal if you alreay specified a reciever during initialization.
		 */
		void statusText(const QString&);
	public slots:
		//! Select curve to translate, using the specified point as base point.
		void selectCurvePoint(QwtPlotCurve *curve, int point_index);
		//! Select the coordinates where the base point should end up (modulo projection on #d_dir direction).
		void selectDestination(const QwtDoublePoint &point);
	private:
		Direction d_dir;
		PlotToolInterface *d_sub_tool;
		QwtPlotCurve *d_selected_curve;
		QwtDoublePoint d_curve_point;
		ApplicationWindow *d_app;
};

#endif // TRANSLATE_CURVE_TOOL_H
