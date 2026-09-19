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
        // Dense1 (densest) .. Dense7 (sparsest): approximate each level's fill fraction with a dot grid
        // whose dot area is proportional to that fraction, rather than replicating Qt's exact bitmap.
        const int level = style - Qt::Dense1Pattern + 1;
        const double fillFraction = (8.0 - level) / 8.0;
        const double dotSize = spacing * std::sqrt(fillFraction);
        const int ySteps = steps(r.height());
        const int xSteps = steps(r.width());
        for (int iy = 0; iy <= ySteps; ++iy)
        {
            const double y = r.top() + iy * spacing;
            for (int ix = 0; ix <= xSteps; ++ix)
            {
                const double x = r.left() + ix * spacing;
                QPainterPath dot;
                dot.addRect(QRectF(x - dotSize / 2.0, y - dotSize / 2.0, dotSize, dotSize));
                const QPainterPath clipped = path.intersected(dot);
                if (!clipped.isEmpty())
                    p->fillPath(clipped, fillBrush);
            }
        }
        p->restore();
        return;
    }

    const bool drawHor = style == Qt::HorPattern || style == Qt::CrossPattern;
    const bool drawVer = style == Qt::VerPattern || style == Qt::CrossPattern;
    // BDiag = "\", FDiag = "/"; DiagCross draws both.
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
            strokeSegment(QPointF(r.left() + c, r.top()), QPointF(r.left() + c + r.height(), r.bottom()));
        }
    }
    if (drawFDiag)
    {
        const int n = steps(r.width() + r.height());
        for (int i = 0; i <= n; ++i)
        {
            const double c = -r.height() + i * spacing;
            strokeSegment(QPointF(r.left() + c, r.bottom()), QPointF(r.left() + c + r.height(), r.top()));
        }
    }

    p->restore();
}
} // namespace VectorHatchBrush
