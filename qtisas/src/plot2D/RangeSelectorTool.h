/***************************************************************************
    File                 : RangeSelectorTool.h
    Project              : QtiSAS
    --------------------------------------------------------------------
    Copyright /QtiPlot/  : (C) 2006,2007 by Ion Vasilief, Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, knut.franke*gmx.de
    Description          : Plot tool for selecting ranges on curves.

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef RANGE_SELECTOR_TOOL_H
#define RANGE_SELECTOR_TOOL_H

#include "PlotToolInterface.h"

#include <QPointer>

#include <qwt_double_rect.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>

class QwtPlotCurve;
class QPoint;
class QEvent;
class QDialog;
class QCheckBox;

/*! Plot tool for selecting ranges on curves.
 *
 * As detailed in the documentation of PlotToolInterface, this one is different
 * from most other plot tools in that other tools depend on it.
 * Thus, either RangeSelectorTool should cease to inherit from PlotToolInterface and be renamed to
 * RangeSelector, drawing a clear destinction to the other tools, or the notion of multiple tools being
 * active in parallel and possibly depending on each other should be generalized somehow.
 *
 * In any case, gathering the code specific to range selection in a seperate class makes Graph/CanvasPicker
 * more manageable; maybe something similar can be done for zooming.
 */
class RangeSelectorTool : public QwtPlotPicker, public PlotToolInterface
{
	Q_OBJECT
	public:
		RangeSelectorTool(Graph *graph, const QObject *status_target=NULL, const char *status_slot="");
		virtual ~RangeSelectorTool();
		double minXValue() const { return QMIN(d_active_marker.xValue(), d_inactive_marker.xValue()); }
		double maxXValue() const { return QMAX(d_active_marker.xValue(), d_inactive_marker.xValue()); }
		double minYValue() const { return QMIN(d_active_marker.yValue(), d_inactive_marker.yValue()); }
		double maxYValue() const { return QMAX(d_active_marker.yValue(), d_inactive_marker.yValue()); }
		int dataSize() const { return qAbs(d_active_point - d_inactive_point); }
		virtual bool eventFilter(QObject *obj, QEvent *event);
		bool keyEventFilter(QKeyEvent *ke);

		QwtPlotCurve *selectedCurve() const { return d_selected_curve; }
		//! Caller is responsible for replot.
		void setSelectedCurve(QwtPlotCurve *curve);

        void copySelection();
        void cutSelection();
        void clearSelection();
        void pasteSelection();
        virtual int rtti() const {return PlotToolInterface::Rtti_RangeSelector;};

		void setVisible(bool on);
		bool isVisible(){return d_visible;};

	public slots:
		virtual void pointSelected(const QPoint &point);
        void setCurveRange();
        void setEnabled(bool on = true);

	private slots:
		void copyMultipleSelection();
		void clearMultipleSelection();
		void cutMultipleSelection();

	signals:
		/*! Emitted whenever a new message should be presented to the user.
		 *
		 * You don't have to connect to this signal if you alreay specified a reciever during initialization.
		 */
		void statusText(const QString&);
		//! Emitted whenever the selected curve and/or range have changed.
		void changed();

	protected:
		virtual void append(const QPoint& point) { pointSelected(point); }
		void emitStatusText();
		void switchActiveMarker();
		//! Caller is responsible for replot.
		void setActivePoint(int index);

	private:
		enum RangeEditOperation{Copy, Cut, Delete};

		void showSelectionDialog(RangeEditOperation op = Copy);
		bool mightNeedMultipleSelection();
		//! Copies data range from the selected curve to clipboard
		void copySelectedCurve();
		//! Clears data range from the selected curve
		void clearSelectedCurve();

		QwtPlotMarker d_active_marker, d_inactive_marker;
		int d_active_point, d_inactive_point;
		QwtPlotCurve *d_selected_curve;
		bool d_enabled;
		bool d_visible;

		QPointer <QDialog> d_selection_dialog;
		//! Keeps track of the multiple curves selection
		QList<QCheckBox *> d_selection_lst;
};

#endif // ifndef RANGE_SELECTOR_TOOL_H
