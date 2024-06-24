/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Knut Franke <knut.franke@gmx.de>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Plot tool for doing multi-peak fitting
 ******************************************************************************/

#ifndef MULTIPEAKFITTOOL_H
#define MULTIPEAKFITTOOL_H

#include "PlotToolInterface.h"
#include "MultiPeakFit.h"
#include <QObject>

class DataPickerTool;
class ApplicationWindow;
class QwtPlotCurve;
class QwtPlotMarker;

/*! Plot tool for doing multi-peak fitting.
 *
 * This class can be seen as a user-interface wrapper around MultiPeakFit, providing functionality for visually
 * selecting estimated peak positions and finally executing the fit.
 *
 * Maybe some other parts of the multi-peak fitting process (namely, specifying the number of peaks and optionally
 * the peak profile) could be moved here as well.
 */
class MultiPeakFitTool : public QObject, public PlotToolInterface
{
	Q_OBJECT
	public:
		MultiPeakFitTool(Graph *graph, ApplicationWindow *app, MultiPeakFit::PeakProfile profile, int num_peaks, const QObject *status_target, const char *status_slot);
		virtual ~MultiPeakFitTool();

        virtual int rtti() const {return PlotToolInterface::Rtti_MultiPeakFitTool;};
	signals:
		/*! Emitted whenever a new message should be presented to the user.
		 *
		 * You don't have to connect to this signal if you alreay specified a reciever during initialization.
		 */
		void statusText(const QString&);
	protected slots:
		void selectPeak(QwtPlotCurve *curve, int point_index);
	private:
		void finalize();
		int d_selected_peaks;
		DataPickerTool *d_picker_tool;
		MultiPeakFit *d_fit;
		QwtPlotCurve *d_curve;
		QList<QwtPlotMarker *> d_lines;
};

#endif
