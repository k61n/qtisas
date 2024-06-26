/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Selection of Widgets and QwtPlotMarkers
 ******************************************************************************/

#ifndef SELECTIONMOVERESIZER_H
#define SELECTIONMOVERESIZER_H

#include <QList>
#include <QPoint>
#include <QRect>
#include <QWidget>

#include "ArrowMarker.h"
#include "LegendWidget.h"

/*!\brief Handler for modifying one or more QWidget and/or QwtPlotMarker with mouse or keyboard.
 *
 * During initialization, the SelectionMoveResizer adds itself as a new child widget to
 * the target's parentWidget() or QwtPlotCanvas. Because Qt 4.1 and beyond propagate a widget's
 * background by default, frame and resize handlers can then simply be drawn on top of
 * the usual content of the parent.
 * Also, the SelectionMoveResizer gets to see mouse and key events first without the need to filter
 * events for its parent.
 * %Note however that the latter must still be done in order to catch geometry changes and
 * stay the same size as the parent.
 *
 * At every point in time, the list of managed targets must be non-empty,
 * so that the SelectionMoveResizer has a well-defined bounding rectangle.
 * When there are no more selected targets, it deletes itself, so references to a SelectionMoveResizer
 * should be maintained by QPointer and checked for validity before use.
 *
 * QWidget targets are automatically removed when they are deleted (possibly also deleting the
 * SelectionMoveResizer), using the QObject::destroyed() signal. With QwtPlotMarker, this is not
 * possible since it's not a QObject. Thus, care has to be taken to remove them before or shortly after deletion,
 * lest the wrath of Segmentation Fault shall come upon you.
 *
 * \section design Design Ideas
 *   - Keep as much of the select/move/resize code as possible in one class to ease maintanance.
 *   - Use the same class for layer and marker handling to avoid duplicating code.
 *     Good for bugfixing and for maintaining a consistent user interface.
 *   - Keep this class generic so that it can later be used for other parents
 *     (than MultiLayer's canvas QWidget and Plot's QwtPlotCanvas)
 *     and/or targets (e.g. new QwtPlotMarker subclasses) with little to no modifications.
 *   - Differentiate between move/resize, which are generic operations, and editing (like moving the
 *     endpoints of a ArrowMarker), which requires detailed knowledge about the target.
 *     This way, new types of markers or MultiLayer children support moving/resizing without further
 *     work. Editing can then be implemented in the targets themselves, as shown in ArrowMarker.
 *   .
 *
 * \section future Future Plans
 * See the documentation of ImageMarker for ideas about replacing QwtPlotMarker with a new QwtPlotItem subclass.
 * For SelectionMoveResizer this would mean supporting new enrichments without modifications and being able
 * to automatically remove them upon deletion (if the new class inherits QObject).
 * Not to mention that the implementation would be a lot cleaner if we would only have to handle two target classes
 * instead of four.
 *
 * Think about turning SelectionMoveResizer into an input method for general affine transformations.
 * Particularly, this would add rotation, flipping and shearing.
 * Markers could implement this using QPainter's coordinate transformations.
 * Definitely not a priority, though.
 */
class SelectionMoveResizer : public QWidget
{
	Q_OBJECT

	public:
		//! Construct a new MoveResizer with the given marker as the only target.
		SelectionMoveResizer(ArrowMarker *target);
		//! Construct a new MoveResizer with the given widget as the only target.
		SelectionMoveResizer(QWidget *target);
		//! Clean up after myself.
		~SelectionMoveResizer();
		/*!\brief React on geometry changes of parent and targets.
		 */
		virtual bool eventFilter(QObject *o, QEvent *e);
		//! Returns true if w is one of targets, false else.
		bool contains(QWidget *w) const { return d_widgets.contains(w); };
		QList <QWidget *> widgetsList() const { return d_widgets; };
		//! Returns true if m is one of targets, false else.
		bool contains(ArrowMarker *m) const { return d_line_markers.contains(m); };
		void raiseTargets(bool on = true);
		void alignTargetsLeft();
		void alignTargetsRight();
		void alignTargetsTop();
		void alignTargetsBottom();

	public slots:
		//! Add target to the list of items to be moved/resized together.
		void add(ArrowMarker *target);
		//! Add target to the list of items to be moved/resized together.
		void add(QWidget *target);
		//! Remove target from the list of items to be moved/resized together and returns the number of occurences removed.
		int removeAll(ArrowMarker *target);
		//! Remove target from the list of items to be moved/resized together and returns the number of occurences removed.
		int removeAll(QWidget *target);
		//! Calculate #d_bounding_rect based on the bounding rectangles of all targets.
		void recalcBoundingRect();

	signals:
		//! Emitted when the targets are modified (moved or resized).
		void targetsChanged();

	protected:
		//! Available modes of operation.
		enum Operation { None=-2, Move, Resize_N, Resize_NE, Resize_E, Resize_SE, Resize_S, Resize_SW, Resize_W, Resize_NW };

		/*!\brief Draw frame and resize handlers.
		 *
		 * Besides managing resize operations, this also provides a visual feedback
		 * on the selection state.
		 */
		virtual void paintEvent(QPaintEvent *e);
		/*!\brief Mouse button presses start move/resize operations.
		 *
		 * Clicks outside of #d_bounding_rect or with anything else than the left button
		 * are propagated to the parent as usual.
		 */
		virtual void mousePressEvent(QMouseEvent *e);
		/*!\brief Mouse movements need to be monitored for updating the frame during operation.
		 *
		 * When no operation is in progress, the mouse cursor is updated based on its position
		 * before the event is passed on to the parent.
		 */
		virtual void mouseMoveEvent(QMouseEvent *e);
		/*!\brief Mouse releases end the current operation and apply it to the targets.
		 *
		 * When there is no operation in progress, the event is passed on to the parent.
		 */
		virtual void mouseReleaseEvent(QMouseEvent *e);
		/*!\brief Allow keyboard-based moving of the selection.
		 *
		 * Unused keys are passed on to the parent.
		 */
		virtual void keyPressEvent(QKeyEvent *e);
		//! Ignore double-clicks so that they can be used by my parent.
		virtual void mouseDoubleClickEvent(QMouseEvent *e);

	private:
		//! Size of resize handle in pixels.
		static const int handler_size = 10;
		//! Return one of the rectangles to be filled for starting resize operations.
		static const QRect handlerRect(QRect rect, Operation op);
		//! Apply operation specified by #d_op and #d_op_dp to in.
		QRect operateOn(const QRect in);
		//! Apply operation specified by #d_op and #d_op_dp to all targets.
		void operateOnTargets();
		//! The initialization part common to all constructors.
		void init();
		//! Get the bounding rectangle of a marker in paint coordinates.
		QRect boundingRectOf(QwtPlotMarker *target) const;
		//! Get the bounding rectangle of a plot canvas area in paint coordinates.
		QRect boundingRectOf(QwtPlotCanvas *canvas) const;

		//! Target markers I'm handling.
		QList <ArrowMarker *> d_line_markers;
		//! Target widgets I'm handling.
		QList <QWidget *> d_widgets;
		//! Bounding rectangle of all targets taken together, in drawing coordinates.
		QRect d_bounding_rect;
		//! The type of operation currently in progress.
		Operation d_op;
		//! Mouse position where an operation started; (-1,-1) means started by keyboard.
		QPoint d_op_start;
		//! Difference between current and start position during operation.
		QPoint d_op_dp;

	private slots:
		//! A non-typesafe version of remvoveAll(QWidget*) needed for QObject::destroyed().
		void removeWidget(QObject* w) { removeAll((QWidget*) w); }
};

#endif
