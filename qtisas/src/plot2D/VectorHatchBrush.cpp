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

#include <QByteArray>
#include <QColor>
#include <QLineF>
#include <QPainter>
#include <QPainterPath>
#include <QSvgGenerator>
#include <QVector>

#include "VectorHatchBrush.h"

namespace VectorHatchBrush
{
bool isHatchStyle(Qt::BrushStyle style)
{
    return style >= Qt::Dense1Pattern && style <= Qt::DiagCrossPattern;
}

// Qt's native 8x8 dither bitmap per Dense1-7 density level, extracted from the real QBrush pattern.
static const quint8 g_denseBits[7][8] = {
    {0xFF, 0xDD, 0xFF, 0xFF, 0xFF, 0xDD, 0xFF, 0xFF}, // Dense1, fill 0.938
    {0xEE, 0xFF, 0xBB, 0xFF, 0xEE, 0xFF, 0xBB, 0xFF}, // Dense2, fill 0.875
    {0xAA, 0xDD, 0xAA, 0x77, 0xAA, 0xDD, 0xAA, 0x77}, // Dense3, fill 0.625
    {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA}, // Dense4, fill 0.500
    {0x55, 0x22, 0x55, 0x88, 0x55, 0x22, 0x55, 0x88}, // Dense5, fill 0.375
    {0x11, 0x00, 0x44, 0x00, 0x11, 0x00, 0x44, 0x00}, // Dense6, fill 0.125
    {0x00, 0x22, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00}, // Dense7, fill 0.062
};

namespace
{
struct PatternEntry
{
    QColor color;
    Qt::BrushStyle style;
    double spacing;
};

QVector<PatternEntry> &patternRegistry()
{
    static QVector<PatternEntry> registry;
    return registry;
}

// Reserved near-black marker palette (R=1 prefix) -- astronomically unlikely to collide with a real
// user-chosen fill color, and even in that edge case the marker looks like the real color on screen.
QColor markerColorForIndex(int index)
{
    return {1, (index >> 8) & 0xFF, index & 0xFF};
}

int registerPattern(const QColor &color, Qt::BrushStyle style, double spacing)
{
    QVector<PatternEntry> &registry = patternRegistry();
    for (int i = 0; i < registry.size(); ++i)
    {
        const PatternEntry &e = registry[i];
        if (e.color == color && e.style == style && e.spacing == spacing)
            return i;
    }
    registry.append({color, style, spacing});
    return static_cast<int>(registry.size()) - 1;
}

// The Dense1-7 bitmap's on-cells for one tile (8x8), as merged rects (row-band + run-length merged,
// same technique as the geometry fallback below) -- used as a <pattern>'s one-time tile content.
QString patternTileRectsSvg(int level, double cell, const QColor &color)
{
    QString fillAttr = QStringLiteral(" fill=\"%1\"").arg(color.name(QColor::HexRgb));
    if (color.alpha() != 255)
        fillAttr += QStringLiteral(" fill-opacity=\"%1\"").arg(color.alphaF());

    QString s;
    for (int row = 0; row < 8;)
    {
        const quint8 mask = g_denseBits[level][row];
        int rowSpan = 1;
        while (row + rowSpan < 8 && g_denseBits[level][row + rowSpan] == mask)
            ++rowSpan;
        if (mask)
        {
            const double y = row * cell;
            const double h = rowSpan * cell;
            int col = 0;
            while (col < 8)
            {
                if (!(mask & (0x80 >> col)))
                {
                    ++col;
                    continue;
                }
                const int start = col;
                do
                {
                    ++col;
                } while (col < 8 && (mask & (0x80 >> col)));
                const double x = start * cell;
                const double w = (col - start) * cell;
                s += QStringLiteral("<rect x=\"%1\" y=\"%2\" width=\"%3\" height=\"%4\"%5/>")
                         .arg(x)
                         .arg(y)
                         .arg(w)
                         .arg(h)
                         .arg(fillAttr);
            }
        }
        row += rowSpan;
    }
    return s;
}
} // namespace

void resetPatternRegistry()
{
    patternRegistry().clear();
}

QByteArray applyPatternRegistry(const QByteArray &svg)
{
    const QVector<PatternEntry> &registry = patternRegistry();
    if (registry.isEmpty())
        return svg;

    QString s = QString::fromUtf8(svg);
    QString defsAddition;
    for (int i = 0; i < registry.size(); ++i)
    {
        const PatternEntry &e = registry[i];
        const int level = e.style - Qt::Dense1Pattern;
        const double cell = e.spacing / 8.0;
        defsAddition += QStringLiteral("<pattern id=\"vhp%1\" x=\"0\" y=\"0\" width=\"%2\" height=\"%2\" "
                                       "patternUnits=\"userSpaceOnUse\">%3</pattern>")
                            .arg(i)
                            .arg(e.spacing)
                            .arg(patternTileRectsSvg(level, cell, e.color));

        const QString markerHex = markerColorForIndex(i).name(QColor::HexRgb);
        s.replace(QStringLiteral("fill=\"%1\"").arg(markerHex), QStringLiteral("fill=\"url(#vhp%1)\"").arg(i));
    }

    const int defsPos = static_cast<int>(s.indexOf(QStringLiteral("<defs>")));
    if (defsPos >= 0)
        s.insert(defsPos + 6, defsAddition);
    else
        s.replace(QStringLiteral("</svg>"), QStringLiteral("<defs>") + defsAddition + QStringLiteral("</defs></svg>"));

    return s.toUtf8();
}

void fillPath(QPainter *p, const QPainterPath &path, const QColor &color, Qt::BrushStyle style, double spacing)
{
    p->save();
    // Clip to the painter's clip first: an extreme value (e.g. near-zero on a log scale) can produce
    // a fill path far taller than the visible canvas, ballooning the tile count for no visible gain.
    QPainterPath boundedPath = path;
    if (p->hasClipping())
    {
        QPainterPath clipRectPath;
        clipRectPath.addRect(p->clipBoundingRect());
        boundedPath = path.intersected(clipRectPath);
    }
    const QRectF r = boundedPath.boundingRect();

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
        const QPainterPath clipped = boundedPath.intersected(stripe);
        if (!clipped.isEmpty())
            p->fillPath(clipped, fillBrush);
    };

    // Integer step count avoids floating-point loop induction (accumulated rounding error in a
    // "value += spacing" comparison can silently skip or repeat the final tile).
    auto steps = [&](double span) { return static_cast<int>(span / spacing); };

    if (style >= Qt::Dense1Pattern && style <= Qt::Dense7Pattern)
    {
        if (dynamic_cast<QSvgGenerator *>(p->device()) != nullptr)
        {
            // SVG (and PNG/JPEG, via the same pipeline): mark now, applyPatternRegistry() resolves
            // it to a compact native <pattern> after generation. PDF has no equivalent
            // post-processing step, so it uses the geometry fallback below instead.
            const int index = registerPattern(color, style, spacing);
            p->fillPath(boundedPath, QBrush(markerColorForIndex(index)));
            p->restore();
            return;
        }
        const int level = style - Qt::Dense1Pattern;
        // Tile spans `spacing`, same convention as the line patterns below (half of native's raw
        // 8px tile, which Retina's devicePixelRatio already halves on screen).
        const double tile = spacing;
        const double cell = tile / 8.0;
        const int ySteps = static_cast<int>(r.height() / tile);
        const int xSteps = static_cast<int>(r.width() / tile);

        // One row-band (full width) at a time, adjacent "on" cells merged into wider rects via
        // run-length encoding -- per-cell paths made an SVG "d" attribute large enough to be
        // rejected outright, and ballooned file size regardless.
        const int totalCols = (xSteps + 1) * 8;
        for (int ty = 0; ty <= ySteps; ++ty)
        {
            // Consecutive rows sharing the same mask (e.g. Dense1's rows 2-4) become one taller
            // band instead of separate same-shaped rects stacked on top of each other.
            for (int row = 0; row < 8;)
            {
                const quint8 mask = g_denseBits[level][row];
                int rowSpan = 1;
                while (row + rowSpan < 8 && g_denseBits[level][row + rowSpan] == mask)
                    ++rowSpan;
                if (!mask)
                {
                    row += rowSpan;
                    continue;
                }
                const double y = r.top() + ty * tile + row * cell;
                const double bandHeight = rowSpan * cell;

                QPainterPath rowRuns;
                int col = 0;
                while (col < totalCols)
                {
                    if (!(mask & (0x80 >> (col % 8))))
                    {
                        ++col;
                        continue;
                    }
                    const int start = col;
                    do
                    {
                        ++col;
                    } while (col < totalCols && (mask & (0x80 >> (col % 8))));
                    const double x = r.left() + start * cell;
                    rowRuns.addRect(QRectF(x, y, (col - start) * cell, bandHeight));
                }
                const QPainterPath clipped = boundedPath.intersected(rowRuns);
                if (!clipped.isEmpty())
                    p->fillPath(clipped, QBrush(color));
                row += rowSpan;
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
