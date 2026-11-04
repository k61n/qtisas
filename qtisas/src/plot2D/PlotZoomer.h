/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Rectangle zoomer for log axes: minimum zoom size on the axis scale, exact zoom bounds
 ******************************************************************************/

#ifndef PLOTZOOMER_H
#define PLOTZOOMER_H

#include <QList>
#include <QRectF>
#include <QStack>

#include <qwt/qwt_plot_zoomer.h>

//! QwtPlotZoomer that keeps log axes (also mirrored negative ones) on their domain.
/*!
  - QwtPlotZoomer widens a zoom rectangle smaller than 1/10^5 of the zoom base around its center in data units. On
    a log axis spanning many decades that pushes a box near the low end below zero, and the axis then runs from
    1e-150. Here the same minimum is applied in the axis' transformed coordinates (decades on a log axis); on a
    linear axis nothing changes.
  - The zoom stack holds QRectF, which stores the far bound as an offset: right = left + width. On a negative log
    axis the bound near zero is that offset, so -1e5..-1e-12 came back from zoom-out as -1e5..0. The bounds of every
    rectangle are kept here as well and set back exactly.
*/
class PlotZoomer : public QwtPlotZoomer
{
  public:
    PlotZoomer(int xAxis, int yAxis, QWidget *canvas);

    void setZoomBase(bool doReplot = true) override;
    using QwtPlotZoomer::setZoomBase;
    void zoom(const QRectF &rect) override;
    using QwtPlotZoomer::zoom;

  protected:
    //! Invalid: QwtPlotZoomer::end() widens nothing, zoom() does it per axis.
    QSizeF minZoomSize() const override;
    bool end(bool ok = true) override;
    void rescale() override;

  private:
    struct Bounds
    {
        QRectF rect;
        double x1{}, x2{}, y1{}, y2{};
    };

    void zoomTo(double x1, double x2, double y1, double y2);
    void widen(int axis, double baseLow, double baseHigh, double &low, double &high) const;
    void forgetUnstacked();

    QList<Bounds> d_bounds;
};

#endif
