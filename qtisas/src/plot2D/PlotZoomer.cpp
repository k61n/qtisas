/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Rectangle zoomer for log axes: minimum zoom size on the axis scale, exact zoom bounds
 ******************************************************************************/

#include <cmath>
#include <utility>

#include <qwt/qwt_plot.h>
#include <qwt/qwt_scale_div.h>
#include <qwt/qwt_scale_map.h>
#include <qwt/qwt_transform.h>

#include "PlotZoomer.h"

namespace
{
QRectF rectOf(double x1, double x2, double y1, double y2)
{
    return QRectF(QPointF(x1, y1), QPointF(x2, y2)).normalized();
}
} // namespace

PlotZoomer::PlotZoomer(int xAxis, int yAxis, QWidget *canvas) : QwtPlotZoomer(xAxis, yAxis, canvas)
{
}

QSizeF PlotZoomer::minZoomSize() const
{
    return {};
}

void PlotZoomer::setZoomBase(bool doReplot)
{
    QwtPlotZoomer::setZoomBase(doReplot);
    d_bounds.clear();
    const QwtPlot *plt = plot();
    if (!plt)
        return;
    const QwtScaleDiv &xs = plt->axisScaleDiv(xAxis());
    const QwtScaleDiv &ys = plt->axisScaleDiv(yAxis());
    const double x1 = qMin(xs.lowerBound(), xs.upperBound());
    const double x2 = qMax(xs.lowerBound(), xs.upperBound());
    const double y1 = qMin(ys.lowerBound(), ys.upperBound());
    const double y2 = qMax(ys.lowerBound(), ys.upperBound());
    if (!zoomStack().isEmpty() && zoomStack().first() == rectOf(x1, x2, y1, y2))
        d_bounds.append({zoomStack().first(), x1, x2, y1, y2});
}

bool PlotZoomer::end(bool ok)
{
    // QwtPlotZoomer::end() without its rectangle: the bounds straight from the pixels of the selection
    ok = QwtPlotPicker::end(ok);
    if (!ok)
        return false;
    const QwtPlot *plt = plot();
    if (!plt)
        return false;
    const QPolygon &pa = selection();
    if (pa.count() < 2)
        return false;
    const QRect r = QRect(pa.first(), pa.last()).normalized();
    const QwtScaleMap xMap = plt->canvasMap(xAxis());
    const QwtScaleMap yMap = plt->canvasMap(yAxis());
    zoomTo(xMap.invTransform(r.left()), xMap.invTransform(r.right()), yMap.invTransform(r.top()),
           yMap.invTransform(r.bottom()));
    return true;
}

void PlotZoomer::zoom(const QRectF &rect)
{
    const QRectF r = rect.normalized();
    zoomTo(r.left(), r.right(), r.top(), r.bottom());
}

void PlotZoomer::zoomTo(double x1, double x2, double y1, double y2)
{
    if (x1 > x2)
        std::swap(x1, x2);
    if (y1 > y2)
        std::swap(y1, y2);
    const QRectF base = zoomBase().normalized();
    Bounds b{base, base.left(), base.right(), base.top(), base.bottom()};
    for (const Bounds &kept : d_bounds)
        if (kept.rect == base)
        {
            b = kept;
            break;
        }
    widen(xAxis(), b.x1, b.x2, x1, x2);
    widen(yAxis(), b.y1, b.y2, y1, y2);
    // known before QwtPlotZoomer::zoom() pushes the rectangle and calls rescale()
    const QRectF rect = rectOf(x1, x2, y1, y2);
    d_bounds.append({rect, x1, x2, y1, y2});
    QwtPlotZoomer::zoom(rect);
    forgetUnstacked();
}

void PlotZoomer::rescale()
{
    // QwtPlotZoomer::rescale(), with the kept bounds of the rectangle when there are any
    QwtPlot *plt = plot();
    if (!plt)
        return;
    const QRectF &rect = zoomRect();
    if (rect == scaleRect())
        return;
    double x1 = rect.left();
    double x2 = rect.right();
    double y1 = rect.top();
    double y2 = rect.bottom();
    for (auto it = d_bounds.crbegin(); it != d_bounds.crend(); ++it)
        if (it->rect == rect)
        {
            x1 = it->x1;
            x2 = it->x2;
            y1 = it->y1;
            y2 = it->y2;
            break;
        }

    const bool doReplot = plt->autoReplot();
    plt->setAutoReplot(false);
    if (!plt->axisScaleDiv(xAxis()).isIncreasing())
        std::swap(x1, x2);
    plt->setAxisScale(xAxis(), x1, x2);
    if (!plt->axisScaleDiv(yAxis()).isIncreasing())
        std::swap(y1, y2);
    plt->setAxisScale(yAxis(), y1, y2);
    plt->setAutoReplot(doReplot);
    plt->replot();
}

void PlotZoomer::forgetUnstacked()
{
    const QStack<QRectF> &stack = zoomStack();
    for (auto i = d_bounds.size() - 1; i >= 0; i--)
        if (!stack.contains(d_bounds[i].rect))
            d_bounds.removeAt(i);
}

void PlotZoomer::widen(int axis, double baseLow, double baseHigh, double &low, double &high) const
{
    // the same minimum as QwtPlotZoomer::minZoomSize() (1/10^5 of the zoom base), on the axis' transformed scale
    const QwtPlot *plt = plot();
    if (!plt)
        return;
    const QwtScaleMap map = plt->canvasMap(axis);
    const QwtTransform *transform = map.transformation();
    auto forward = [transform](double v) { return transform ? transform->transform(v) : v; };
    auto back = [transform](double v) { return transform ? transform->invTransform(v) : v; };

    const double minSpan = std::fabs(forward(baseHigh) - forward(baseLow)) / 1e5;
    const double a = forward(low);
    const double b = forward(high);
    if (!std::isfinite(minSpan) || !std::isfinite(a) || !std::isfinite(b) || std::fabs(b - a) >= minSpan)
        return;

    const double center = 0.5 * (a + b);
    const double half = (b >= a ? 0.5 : -0.5) * minSpan;
    low = back(center - half);
    high = back(center + half);
    if (low > high)
        std::swap(low, high);
}
