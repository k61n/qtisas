/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Graph methods exposed to the Python scripting API
 ******************************************************************************/

#include <ColorBox.h>
#include <qwt/qwt_symbol.h>

#include "ApplicationWindow.h"
#include "Graph.h"
#include "PlotCurve.h"

namespace
{
// QwtPlotCurve::symbol() returns null for any curve with no symbol assigned yet
// (e.g. every plain Lines-style curve, such as a fit/model curve). The setters
// below must still take effect on such curves instead of crashing or no-op'ing.
struct SymbolBase
{
    QwtSymbol::Style style;
    QBrush brush;
    QPen pen;
    QSize size;
};

SymbolBase symbolBase(const QwtSymbol *old)
{
    if (old)
        return {old->style(), old->brush(), old->pen(), old->size()};
    return {QwtSymbol::NoSymbol, QBrush(), QPen(Qt::black, 1), QSize(7, 7)};
}
} // namespace

void Graph::setCurveLineStyle(int curveIndex, Qt::PenStyle style)
{
    QwtPlotCurve *c = curve(curveIndex);
    if (c)
    {
        QPen pen = c->pen();
        pen.setStyle(style);
        c->setPen(pen);
        replot();
        emit modifiedGraph();
    }
}

void Graph::setCurveLineWidth(int curveIndex, double width)
{
    QwtPlotCurve *c = curve(curveIndex);
    if (c)
    {
        QPen pen = c->pen();
        pen.setWidthF(width);
        c->setPen(pen);
        replot();
        emit modifiedGraph();
    }
}

void Graph::setCurveLineColor(int curveIndex, int colorIndex)
{
    ApplicationWindow *app = aw();
    QList<QColor> colors = app ? app->indexedColors() : ColorBox::defaultColors();
    if (colorIndex >= 0 && colorIndex < colors.size())
        setCurveLineColor(curveIndex, colors[colorIndex]);
}

void Graph::setCurveLineColor(int curveIndex, QColor color)
{
    QwtPlotCurve *c = curve(curveIndex);
    if (c)
    {
        QPen pen = c->pen();
        pen.setColor(color);
        c->setPen(pen);
        replot();
        emit modifiedGraph();
    }
}

void Graph::setCurveSymbolColor(int curveIndex, int colorIndex)
{
    ApplicationWindow *app = aw();
    QList<QColor> colors = app ? app->indexedColors() : ColorBox::defaultColors();
    if (colorIndex >= 0 && colorIndex < colors.size())
        setCurveSymbolColor(curveIndex, colors[colorIndex]);
}

void Graph::setCurveSymbolColor(int curveIndex, QColor color)
{
    PlotCurve *c = curve(curveIndex);
    if (c)
    {
        SymbolBase base = symbolBase(c->symbol());
        auto *sym = new QwtSymbol(base.style);
        sym->setBrush(QBrush(color));
        sym->setPen(base.pen);
        sym->setSize(base.size);
        c->setSymbol(sym);
        replot();
        emit modifiedGraph();
    }
}

void Graph::setCurveSymbolBorderColor(int curveIndex, int colorIndex)
{
    ApplicationWindow *app = aw();
    QList<QColor> colors = app ? app->indexedColors() : ColorBox::defaultColors();
    if (colorIndex >= 0 && colorIndex < colors.size())
        setCurveSymbolBorderColor(curveIndex, colors[colorIndex]);
}

void Graph::setCurveSymbolBorderColor(int curveIndex, QColor color)
{
    PlotCurve *c = curve(curveIndex);
    if (c)
    {
        SymbolBase base = symbolBase(c->symbol());
        auto *sym = new QwtSymbol(base.style);
        sym->setBrush(base.brush);
        sym->setPen(QPen(color));
        sym->setSize(base.size);
        c->setSymbol(sym);
        replot();
        emit modifiedGraph();
    }
}

void Graph::setCurveSymbolBorderWidth(int curveIndex, double width)
{
    PlotCurve *c = curve(curveIndex);
    if (c)
    {
        SymbolBase base = symbolBase(c->symbol());
        auto *sym = new QwtSymbol(base.style);
        sym->setBrush(base.brush);
        QPen pen = base.pen;
        pen.setWidthF(width);
        sym->setPen(pen);
        sym->setSize(base.size);
        c->setSymbol(sym);
        replot();
        emit modifiedGraph();
    }
}

void Graph::setCurveSymbolOpacity(int curveIndex, int opacity)
{
    PlotCurve *c = curve(curveIndex);
    if (c)
    {
        SymbolBase base = symbolBase(c->symbol());
        auto *sym = new QwtSymbol(base.style);
        QColor col = base.brush.color();
        col.setAlpha(qBound(0, opacity, 255));
        sym->setBrush(QBrush(col));
        sym->setPen(base.pen);
        sym->setSize(base.size);
        c->setSymbol(sym);
        replot();
        emit modifiedGraph();
    }
}

void Graph::setCurveSymbolStyle(int curveIndex, int style)
{
    PlotCurve *c = curve(curveIndex);
    if (c)
    {
        SymbolBase base = symbolBase(c->symbol());
        auto *sym = new QwtSymbol(static_cast<QwtSymbol::Style>(style));
        sym->setBrush(base.brush);
        sym->setPen(base.pen);
        sym->setSize(base.size);
        c->setSymbol(sym);
        replot();
        emit modifiedGraph();
    }
}

void Graph::setCurveSymbolSize(int curveIndex, int size)
{
    PlotCurve *c = curve(curveIndex);
    if (c)
    {
        SymbolBase base = symbolBase(c->symbol());
        auto *sym = new QwtSymbol(base.style);
        sym->setBrush(base.brush);
        sym->setPen(base.pen);
        int px = 2 * size + 1;
        sym->setSize(px, px);
        c->setSymbol(sym);
        replot();
        emit modifiedGraph();
    }
}
