/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Canvas picker
 ******************************************************************************/

#include <QObject>
#include <QPoint>
#include <QMouseEvent>

class Graph;
class ArrowMarker;

/**
 * \brief Handles parts of the user interaction for a Plot by registering itself as an event filter for its QwtPlotCanvas.
 *
 */
class CanvasPicker: public QObject
{
    Q_OBJECT
public:
	 CanvasPicker(Graph *plot);
	 virtual bool eventFilter(QObject *, QEvent *);
	 void selectPoints(int n);
	 void selectPeak(const QPoint& p);

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

	QPoint startLinePoint, endLinePoint;

signals:
	void showPlotDialog(int);
	void viewLineDialog();
	void showMarkerPopupMenu();
	void modified();

private:
	bool pointSelected;
	/*!\brief The marker that is currently being edited, or nullptr.
	 * Editing does explicitly _not_ inlude moving and resizing, which are being
	 * handled by SelectionMoveResizer (see Graph::d_markers_selector).
	 * Currently, only ArrowMarker provides any other form of editing, but this really
	 * should be generalized. See ImageMarker for details.
	 */
	ArrowMarker *d_editing_marker;
};
