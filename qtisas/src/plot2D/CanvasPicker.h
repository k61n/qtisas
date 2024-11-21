/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Knut Franke <knut.franke@gmx.de>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Canvas picker
 ******************************************************************************/

#ifndef CANVASPICKER_H
#define CANVASPICKER_H

#include <QObject>
#include <QPoint>
#include <QMouseEvent>

#include "ArrowMarker.h"
#include "Graph.h"

/**
 * \brief Handles parts of the user interaction for a Plot by registering itself as an event filter for its QwtPlotCanvas.
 *
 */
class CanvasPicker: public QObject
{
    Q_OBJECT
public:
    explicit CanvasPicker(Graph *plot);
    bool eventFilter(QObject *, QEvent *) override;

	 //! Disable editing of #d_editing_marker on a TAB-key event.
	 /*!\brief Called by Graph::focusNextPrevChild ()
	 */
	void disableEditing();

private:
	void drawLineMarker(const QPoint&, bool endArrow);

	//! Selects and highlights the marker at the given position.
	/**
	 * \return whether a marker was found at #pointSelected
	 */
	bool selectMarker(const QMouseEvent *e);

	/**
	 * \brief Return my parent as a Graph.
	 */
	Graph *plot(){return (Graph *)parent();};

    QPoint startLinePoint;

signals:
	void showPlotDialog(int);
	void viewLineDialog();
	void showMarkerPopupMenu();
	void modified();

private:
	bool pointSelected;
	/*!\brief The marker that is currently being edited, or nullptr.
     * Editing does explicitly _not_ include moving and resizing, which are being
	 * handled by SelectionMoveResizer (see Graph::d_markers_selector).
	 * Currently, only ArrowMarker provides any other form of editing, but this really
	 * should be generalized. See ImageMarker for details.
	 */
	ArrowMarker *d_editing_marker;
};

#endif
