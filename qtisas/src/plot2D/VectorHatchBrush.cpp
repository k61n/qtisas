/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Vector (line/dot based) replacement for Qt's builtin hatch brush
             styles during print/export, where QSvgGenerator/QPrinter render
             them at the wrong tile density compared to on-screen painting.
 ******************************************************************************/

#include <cmath>

#include <QColor>
#include <QLineF>
#include <QPainter>
#include <QPainterPath>

#include "VectorHatchBrush.h"

namespace VectorHatchBrush
{
bool isHatchStyle(Qt::BrushStyle style)
{
    return style >= Qt::Dense1Pattern && style <= Qt::DiagCrossPattern;
}

void fillPath(QPainter *p, const QPainterPath &path, const QColor &color, Qt::BrushStyle style, double spacing)
{
    const QBrush fillBrush(color);
    auto strokeSegment = [&](const QPointF &a, const QPointF &b) {
        QLineF line(a, b);
        QLineF normal = line.normalVector();
        QPointF unit = normal.p2() - normal.p1();
        const double len = std::hypot(unit.x(), unit.y());
        if (len < 1e-9)
            return;
        // 0.25 half-width matches Qt's native ink-coverage ratio at this tile pitch.
        const QPointF offset = unit * (0.25 / len);
        QPainterPath stripe;
        stripe.moveTo(a + offset);
        stripe.lineTo(b + offset);
        stripe.lineTo(b - offset);
        stripe.lineTo(a - offset);
        stripe.closeSubpath();
        const QPainterPath clipped = path.intersected(stripe);
        if (!clipped.isEmpty())
            p->fillPath(clipped, fillBrush);
    };

    p->save();
    const QRectF r = path.boundingRect();
    // Integer step count avoids floating-point loop induction (accumulated rounding error in a
    // "value += spacing" comparison can silently skip or repeat the final tile).
    auto steps = [&](double span) { return static_cast<int>(span / spacing); };

    if (style >= Qt::Dense1Pattern && style <= Qt::Dense7Pattern)
    {
        // Qt's native 8x8 dither bitmap per density level, extracted from the real QBrush pattern.
        static const quint8 bits[7][8] = {
            {0xFF, 0xDD, 0xFF, 0xFF, 0xFF, 0xDD, 0xFF, 0xFF}, // Dense1, fill 0.938
            {0xEE, 0xFF, 0xBB, 0xFF, 0xEE, 0xFF, 0xBB, 0xFF}, // Dense2, fill 0.875
            {0xAA, 0xDD, 0xAA, 0x77, 0xAA, 0xDD, 0xAA, 0x77}, // Dense3, fill 0.625
            {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA}, // Dense4, fill 0.500
            {0x55, 0x22, 0x55, 0x88, 0x55, 0x22, 0x55, 0x88}, // Dense5, fill 0.375
            {0x11, 0x00, 0x44, 0x00, 0x11, 0x00, 0x44, 0x00}, // Dense6, fill 0.125
            {0x00, 0x22, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00}, // Dense7, fill 0.062
        };
        const int level = style - Qt::Dense1Pattern;
        // Tile spans `spacing`, same convention as the line patterns below (half of native's raw
        // 8px tile, which Retina's devicePixelRatio already halves on screen).
        const double tile = spacing;
        const double cell = tile / 8.0;
        const int ySteps = static_cast<int>(r.height() / tile);
        const int xSteps = static_cast<int>(r.width() / tile);

        // Fill one tile (<=64 rects) at a time -- combining every tile into one path makes an SVG
        // "d" attribute large enough that QSvgRenderer (used for PNG/JPEG export) discards it as
        // invalid, rendering nothing.
        for (int ty = 0; ty <= ySteps; ++ty)
        {
            for (int tx = 0; tx <= xSteps; ++tx)
            {
                QPainterPath tileDots;
                for (int row = 0; row < 8; ++row)
                {
                    const quint8 mask = bits[level][row];
                    if (!mask)
                        continue;
                    const double y = r.top() + ty * tile + row * cell;
                    for (int col = 0; col < 8; ++col)
                    {
                        if (!(mask & (0x80 >> col)))
                            continue;
                        const double x = r.left() + tx * tile + col * cell;
                        tileDots.addRect(QRectF(x, y, cell, cell));
                    }
                }
                const QPainterPath clipped = path.intersected(tileDots);
                if (!clipped.isEmpty())
                    p->fillPath(clipped, fillBrush);
            }
        }
        p->restore();
        return;
    }

    const bool drawHor = style == Qt::HorPattern || style == Qt::CrossPattern;
    const bool drawVer = style == Qt::VerPattern || style == Qt::CrossPattern;
    // BDiag renders as "/", FDiag as "\" (opposite of what the names suggest, matched to native).
    const bool drawBDiag = style == Qt::BDiagPattern || style == Qt::DiagCrossPattern;
    const bool drawFDiag = style == Qt::FDiagPattern || style == Qt::DiagCrossPattern;

    if (drawHor)
    {
        const int n = steps(r.height());
        for (int i = 0; i <= n; ++i)
        {
            const double y = r.top() + i * spacing;
            strokeSegment(QPointF(r.left(), y), QPointF(r.right(), y));
        }
    }
    if (drawVer)
    {
        const int n = steps(r.width());
        for (int i = 0; i <= n; ++i)
        {
            const double x = r.left() + i * spacing;
            strokeSegment(QPointF(x, r.top()), QPointF(x, r.bottom()));
        }
    }
    if (drawBDiag)
    {
        const int n = steps(r.width() + r.height());
        for (int i = 0; i <= n; ++i)
        {
            const double c = -r.height() + i * spacing;
            strokeSegment(QPointF(r.left() + c, r.bottom()), QPointF(r.left() + c + r.height(), r.top()));
        }
    }
    if (drawFDiag)
    {
        const int n = steps(r.width() + r.height());
        for (int i = 0; i <= n; ++i)
        {
            const double c = -r.height() + i * spacing;
            strokeSegment(QPointF(r.left() + c, r.top()), QPointF(r.left() + c + r.height(), r.bottom()));
        }
    }

    p->restore();
}
} // namespace VectorHatchBrush
