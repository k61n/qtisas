/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Vector (line/dot based) replacement for Qt's builtin hatch brush
             styles during print/export, where QSvgGenerator/QPrinter render
             them at the wrong tile density compared to on-screen painting.
 ******************************************************************************/

#ifndef VECTORHATCHBRUSH_H
#define VECTORHATCHBRUSH_H

#include <Qt>

class QPainter;
class QPainterPath;
class QColor;

namespace VectorHatchBrush
{
//! True for the builtin styles this namespace can replace (Dense1-7, Hor/Ver/Cross/BDiag/FDiag/DiagCross).
bool isHatchStyle(Qt::BrushStyle style);

//! Fills path with a vector approximation of style (lines, or a dot grid for Dense* levels), clipped via
//! geometric intersection rather than painter clipping. spacing is the tile pitch; pass a smaller value
//! for a small area (e.g. a legend swatch) than the default full-size pitch.
void fillPath(QPainter *painter, const QPainterPath &path, const QColor &color, Qt::BrushStyle style,
              double spacing = 4.0);
} // namespace VectorHatchBrush

#endif // VECTORHATCHBRUSH_H
