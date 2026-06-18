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
        QwtSymbol sym = c->symbol();
        sym.setBrush(QBrush(color));
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
        QwtSymbol sym = c->symbol();
        sym.setPen(QPen(color));
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
        QwtSymbol sym = c->symbol();
        QPen pen = sym.pen();
        pen.setWidthF(width);
        sym.setPen(pen);
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
        QwtSymbol sym = c->symbol();
        QColor col = sym.brush().color();
        col.setAlpha(qBound(0, opacity, 255));
        sym.setBrush(QBrush(col));
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
        QwtSymbol sym = c->symbol();
        sym.setStyle(static_cast<QwtSymbol::Style>(style));
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
        QwtSymbol sym = c->symbol();
        int px = 2 * size + 1;
        sym.setSize(px, px);
        c->setSymbol(sym);
        replot();
        emit modifiedGraph();
    }
}
