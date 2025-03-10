/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Arrow marker (extension to QwtPlotMarker)
 ******************************************************************************/

#ifndef ARROWMARKER_H
#define ARROWMARKER_H

#include <qwt/qwt_plot_marker.h>

/*!\brief Draws lines and arrows on a QwtPlot.
 *
 * \section design Design Ideas
 * Move all the code for editing the end points from CanvasPicker here, so that we have it all in one place.
 * This approach will make adding new editable markers (e.g. polygons) much easier, and it will allow this
 * to be done in plug-ins.
 *
 * \sa ImageMarker, Legend
 */
class ArrowMarker: public QObject, public QwtPlotMarker
{
public:
	enum Operation { None, MoveStart, MoveEnd, MoveBoth };
	enum AttachPolicy {Page, Scales};
    ArrowMarker();

	//! Pixel coordinates of the start point
    [[nodiscard]] QPoint startPoint() const;
	//! Sets the start point in pixel coordinates
	void setStartPoint(const QPoint& p);

	//! Pixel coordinates of the end point
    [[nodiscard]] QPoint endPoint() const;
	//! Sets the end point in pixel coordinates
	void setEndPoint(const QPoint& p);

	//! Axes values coordinates of the start point
	QwtDoublePoint startPointCoord();
	//! Sets the start point in axes values coordinates
	void setStartPoint(double x, double y);

	//! Axes values coordinates of the end point
	QwtDoublePoint endPointCoord();
	//! Sets the end point in axes values coordinates
	void setEndPoint(double x, double y);

	void setColor(const QColor& c);
	QColor color(){return linePen().color();};

	//! Sets the width of the arrow line
	void setWidth(double w);
	//! The width of the arrow line
	double width(){return linePen().widthF();};

	//! Returns half of the arrow width;
	double arrowWidth();

	//! Sets the pen style for the arrow line
	void setStyle(Qt::PenStyle s);
	//! The pen style of the arrow line
	Qt::PenStyle style(){return linePen().style ();};

	//! Specifies weather the start arrow should be drawn
	void drawStartArrow(bool on = true){d_start_arrow = on;};
    [[nodiscard]] bool hasStartArrow() const
    {
        return d_start_arrow;
    }

	//! Specifies weather the end arrow should be drawn
	void drawEndArrow(bool on = true){d_end_arrow = on;};
    [[nodiscard]] bool hasEndArrow() const
    {
        return d_end_arrow;
    }

	//! Length of the arrow head
    [[nodiscard]] int headLength() const
    {
        return d_head_length;
    }
	//! Sets the length of the arrow head
	void setHeadLength(int l);

	//! The angle of the arrow head
    [[nodiscard]] int headAngle() const
    {
        return d_head_angle;
    }
	//! Sets the angle of the arrow head
	void setHeadAngle(int a);

    [[nodiscard]] bool filledArrowHead() const
    {
        return d_fill_head;
    }
	//! Specifies weather the arrow head should be filled with a brush
	void fillArrowHead(bool fill = true);

	//! Returns the shortest distance to the arrow line or to one of the arrow heads
	double dist(int x, int y);

	//! Returns the length of the arrow line
	double length();

    //! Returns the bounding rectangle in paint coordinates.
    [[nodiscard]] QRect rect() const
    {
        return QRect(startPoint(), endPoint()).normalized();
    }

	//! Returns the bounding rectangle in plot coordinates.
    [[nodiscard]] QwtDoubleRect boundingRect() const override;
	void setBoundingRect(double xs, double ys, double xe, double ye);

    //! Recalculates the bounding rectangle in values coordinates using the pixel coordinates when the scales change
	void updateBoundingRect();

	//! Returns the state of #d_editable.
    [[nodiscard]] bool editable() const
    {
        return d_editable;
    }
	//! Starts/ends editing of end points by the user.
	void setEditable(bool yes);

	//! Filters events for the canvas while #d_editable is true.
    bool eventFilter(QObject *o, QEvent *e) override;

	void setAttachPolicy(AttachPolicy attachTo);
	AttachPolicy attachPolicy(){return d_attach_policy;};

private:
	void displayInfo(bool clear = false);
    void draw(QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRect &r) const override;
    static double theta(int xs, int ys, int xe, int ye);

	//! Flag specifying if the start arrow is visible
    bool d_start_arrow{};

	//! Flag specifying if the end arrow is visible
	bool d_end_arrow;

	//! Flag specifying if the arrow head is filled with a brush
	bool d_fill_head;

	//! Angle of the arrow head
	int d_head_angle;

	//! Length of the arrow head
	int d_head_length;

	//! Pixel coordinates of the start point
	QPoint d_start;

	//! Pixel coordinates of the end point
	QPoint d_end;

	//! Bounding rectangle of the arrow in axes values coordinates
	QwtDoubleRect d_rect;
	//! Whether start and end point can be moved by the user.
	bool d_editable;

	//! What editing operation is in progress.
	Operation d_op;

	/*!\brief Difference between mouse position where a MoveBoth operation started and startPoint().
	 * When only one point is being moved, we can simply setStartPoint() or setEndPoint() to the
	 * current mouse position, but when the editing starts in the middle of the line, we
	 * need to remember this bit.
	 */
	QPoint d_op_startat;

	AttachPolicy d_attach_policy;
};

#endif
