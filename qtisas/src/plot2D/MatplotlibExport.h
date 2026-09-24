/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Matplotlib script export shared by Graph and MultiLayer (header-only, inline)
 ******************************************************************************/

#ifndef MATPLOTLIBEXPORT_H
#define MATPLOTLIBEXPORT_H

#include <cmath>

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QList>
#include <QMap>
#include <QPen>
#include <QPolygonF>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <qwt/qwt_curve_fitter.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_scale_div.h>
#include <qwt/qwt_scale_map.h>
#include <qwt/qwt_symbol.h>

#include "ArrowMarker.h"
#include "ErrorBarsCurve.h"
#include "Graph.h"
#include "Grid.h"
#include "LegendWidget.h"
#include "MultiLayer.h"
#include "PlotCurve.h"
#include "QwtBarCurve.h"
#include "ScaleDraw.h"
#include "ScaleEngine.h"
#include "Table.h"

namespace MatplotlibExport
{
// Qt pen widths and tick lengths are device pixels; matplotlib widths/lengths are points (1/72 in).
// The figure's dpi is the screen's logical dpi, so every pixel size goes through this conversion.
inline double pxToPt(double px, Graph *g)
{
    const double dpi = g && g->logicalDpiY() > 0 ? g->logicalDpiY() : 96.0;
    return px * 72.0 / dpi;
}

// A pixel-sized font reports pointSizeF() == -1: convert its pixelSize() instead of dropping fontsize=.
inline double pyFontPointSize(const QFont &font, Graph *g)
{
    if (font.pointSizeF() > 0)
        return font.pointSizeF();
    if (font.pixelSize() > 0)
        return pxToPt(font.pixelSize(), g);
    return 10.0;
}

// ScaleDraw::label() returns QtiSAS's RICH text: "<sup>"/"<sub>" markup plus a U+2212 minus.
// Emitted verbatim, matplotlib prints the tags literally ("10<sup>-4</sup>"), so translate the
// markup into mathtext and let the plain case through untouched.
inline QString pyTickLabelText(const QString &rich)
{
    if (!rich.contains(QLatin1String("<sup>"), Qt::CaseInsensitive) &&
        !rich.contains(QLatin1String("<sub>"), Qt::CaseInsensitive))
        return rich;

    QString math = rich;
    math.replace(QRegularExpression(QStringLiteral("<sup>(.*?)</sup>"), QRegularExpression::CaseInsensitiveOption),
                 QStringLiteral("^{\\1}"));
    math.replace(QRegularExpression(QStringLiteral("<sub>(.*?)</sub>"), QRegularExpression::CaseInsensitiveOption),
                 QStringLiteral("_{\\1}"));
    math.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
    // Inside mathtext the typographic minus is not valid; a literal space would end the expression.
    math.replace(QChar(0x2212), QLatin1Char('-'));
    math.replace(QLatin1Char(' '), QStringLiteral("\\ "));
    // \mathregular keeps the figure's normal font instead of switching to the italic math face.
    return QStringLiteral("$\\mathregular{%1}$").arg(math);
}

// QtiSAS rich text for titles and text boxes: <sup>/<sub> become mathtext segments, other tags are
// dropped, entities decoded, and a literal "$" escaped (unpaired, it breaks mathtext when drawn).
// Inline bold/italic is lost: matplotlib can't switch weight within one plain string.
inline QString pyRichText(const QString &rich)
{
    if (!rich.contains(QLatin1Char('<')) && !rich.contains(QLatin1Char('$')) && !rich.contains(QLatin1Char('&')))
        return rich;

    // Named tags only: a bare "<" is legitimate title text ("x < 5"), not markup to strip.
    static const QRegularExpression tag(QStringLiteral("<(/?)(sup|sub|b|i|u|font|span|p|br)(\\s[^>]*)?/?>"),
                                        QRegularExpression::CaseInsensitiveOption);
    auto decode = [](QString s) {
        s.replace(QLatin1String("&lt;"), QLatin1String("<"));
        s.replace(QLatin1String("&gt;"), QLatin1String(">"));
        s.replace(QLatin1String("&quot;"), QLatin1String("\""));
        s.replace(QLatin1String("&nbsp;"), QLatin1String(" "));
        s.replace(QLatin1String("&amp;"), QLatin1String("&"));
        return s;
    };
    // Every character mathtext would parse, or reject, mapped to a form that just prints it --
    // checked one by one against matplotlib's parser: a bare \# swallows the next letter into the
    // command name, hence the trailing {}, and \" \` \^ \~ are accents, hence \text{}.
    auto mathEscape = [](const QString &s) {
        QString m;
        for (const QChar c : s)
        {
            switch (c.unicode())
            {
            case '\\':
                m += QLatin1String("\\backslash{}");
                break;
            case '#':
            case '$':
            case '%':
            case '_':
            case '{':
            case '}':
                m += QLatin1Char('\\') + QString(c) + QLatin1String("{}");
                break;
            case '"':
            case '`':
            case '^':
            case '~':
                m += QLatin1String("\\text{") + QString(c) + QLatin1Char('}');
                break;
            case ' ':
                m += QLatin1String("\\ ");
                break;
            case 0x2212:
                m += QLatin1Char('-');
                break;
            default:
                m += c;
            }
        }
        return m;
    };

    // Plain text outside any <sup>/<sub>; one "$^{\mathregular{...}}$" segment per outermost one,
    // with nested levels as plain ^{}/_{} groups inside it. A line break can't live inside mathtext,
    // so it closes every open group and reopens them on the next line.
    QString result;
    QStringList openOps; // "^" / "_" per open sup/sub level, outermost first
    auto openMath = [&]() {
        result += QLatin1Char('$');
        for (int k = 0; k < openOps.size(); ++k)
            result += openOps[k] + (k == 0 ? QStringLiteral("{\\mathregular{") : QStringLiteral("{"));
    };
    auto closeMath = [&]() {
        for (int k = static_cast<int>(openOps.size()) - 1; k >= 0; --k)
            result += k == 0 ? QStringLiteral("}}") : QStringLiteral("}");
        result += QLatin1Char('$');
    };
    auto addText = [&](const QString &raw) {
        const QString text = decode(raw);
        if (openOps.isEmpty())
        {
            result += QString(text).replace(QLatin1Char('$'), QLatin1String("\\$"));
            return;
        }
        const QStringList lines = text.split(QLatin1Char('\n'));
        for (int k = 0; k < lines.size(); ++k)
        {
            if (k > 0)
            {
                closeMath();
                result += QLatin1Char('\n');
                openMath();
            }
            result += mathEscape(lines[k]);
        }
    };

    int lastEnd = 0;
    QRegularExpressionMatchIterator it = tag.globalMatch(rich);
    while (it.hasNext())
    {
        const QRegularExpressionMatch m = it.next();
        addText(rich.mid(lastEnd, m.capturedStart() - lastEnd));
        lastEnd = static_cast<int>(m.capturedEnd());

        const QString name = m.captured(2).toLower();
        const bool closing = !m.captured(1).isEmpty();
        if (name == QLatin1String("br"))
            addText(QStringLiteral("\n"));
        else if (name == QLatin1String("sup") || name == QLatin1String("sub"))
        {
            const QString op = name == QLatin1String("sup") ? QStringLiteral("^") : QStringLiteral("_");
            if (!closing)
            {
                if (openOps.isEmpty())
                {
                    openOps << op;
                    openMath();
                }
                else
                {
                    openOps << op;
                    result += op + QLatin1Char('{');
                }
            }
            else if (!openOps.isEmpty())
            {
                if (openOps.size() == 1)
                {
                    closeMath();
                    openOps.clear();
                }
                else
                {
                    openOps.removeLast();
                    result += QLatin1Char('}');
                }
            }
        }
        // b/i/u/font/span/p: dropped, see above.
    }
    addText(rich.mid(lastEnd));
    if (!openOps.isEmpty()) // unclosed <sup>: close it rather than leave mathtext unbalanced
        closeMath();

    // An empty script ("x<sup></sup>") leaves "$^{\mathregular{}}$", which parses but is noise.
    static const QRegularExpression emptyMath(QStringLiteral("\\$[\\^_]\\{\\\\mathregular\\{\\}\\}\\$"));
    result.remove(emptyMath);
    return result;
}

// NaN/Inf must not be written bare: Qt renders them as "nan"/"inf", which are NameErrors in Python,
// so a single masked cell would make the whole script unrunnable.
inline QString pyNumber(double v)
{
    if (std::isnan(v))
        return QStringLiteral("float(\"nan\")");
    if (std::isinf(v))
        return v > 0 ? QStringLiteral("float(\"inf\")") : QStringLiteral("float(\"-inf\")");
    return QString::number(v, 'g', 12);
}

// ", alpha=..." only when the colour is actually translucent, so opaque output stays uncluttered.
inline QString pyAlphaArg(const QColor &c)
{
    if (!c.isValid() || c.alphaF() >= 1.0)
        return {};
    return QStringLiteral(", alpha=%1").arg(c.alphaF(), 0, 'g', 3);
}

inline QString pyStringLiteral(const QString &s)
{
    QString escaped = s;
    // Backslash first, so the escapes added after it aren't doubled; newlines become \n so a
    // multi-line text box stays a valid one-line Python literal.
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"))
        .replace(QLatin1Char('"'), QStringLiteral("\\\""))
        .replace(QLatin1Char('\n'), QStringLiteral("\\n"))
        .replace(QLatin1Char('\r'), QStringLiteral("\\r"));
    return QStringLiteral("\"%1\"").arg(escaped);
}

// matplotlib defaults to DejaVu Sans: put the QtiSAS family first in the sans-serif fallback list,
// keeping matplotlib's default last, so a missing font degrades quietly instead of warning.
inline void writeFontDefaults(QTextStream &out, const QFont &font)
{
    if (font.family().isEmpty())
        return;
    out << "plt.rcParams[\"font.family\"] = \"sans-serif\"\n";
    out << QStringLiteral("plt.rcParams[\"font.sans-serif\"] = [%1, \"Helvetica\", \"Arial\", \"DejaVu Sans\"]\n\n")
               .arg(pyStringLiteral(font.family()));
}

// Per-element override, for the elements whose font QtiSAS tracks separately from the axis numbers.
inline QString pyFontFamilyArg(const QFont &font)
{
    if (font.family().isEmpty())
        return {};
    return QStringLiteral(", fontfamily=%1").arg(pyStringLiteral(font.family()));
}

// matplotlib reads 8-digit hex as #RRGGBBAA (Qt's HexArgb is #AARRGGBB); alpha travels in the colour,
// since an artist-wide alpha= would also tint the edge.
inline QString pyColorRGBA(const QColor &c)
{
    if (c.alphaF() >= 1.0)
        return pyStringLiteral(c.name(QColor::HexRgb));
    return pyStringLiteral(c.name(QColor::HexRgb) + QStringLiteral("%1").arg(c.alpha(), 2, 16, QLatin1Char('0')));
}

// matplotlib linestyle for a Qt::PenStyle (dash-dot-dot has no own code: dash-dot).
inline QString pyLineStyle(Qt::PenStyle style)
{
    switch (style)
    {
    case Qt::NoPen:
        return QStringLiteral("None");
    case Qt::DashLine:
        return QStringLiteral("--");
    case Qt::DotLine:
        return QStringLiteral(":");
    case Qt::DashDotLine:
    case Qt::DashDotDotLine:
        return QStringLiteral("-.");
    case Qt::SolidLine:
    default:
        return QStringLiteral("-");
    }
}

// matplotlib marker codes -- checked directly against QwtSymbol::Style.
inline QString pyMarker(QwtSymbol::Style style)
{
    switch (style)
    {
    case QwtSymbol::Ellipse:
        return QStringLiteral("o");
    case QwtSymbol::Rect:
        return QStringLiteral("s");
    case QwtSymbol::Diamond:
        return QStringLiteral("D");
    case QwtSymbol::Triangle:
    case QwtSymbol::UTriangle:
        return QStringLiteral("^");
    case QwtSymbol::DTriangle:
        return QStringLiteral("v");
    case QwtSymbol::LTriangle:
        return QStringLiteral("<");
    case QwtSymbol::RTriangle:
        return QStringLiteral(">");
    case QwtSymbol::Cross:
        return QStringLiteral("+");
    case QwtSymbol::XCross:
        return QStringLiteral("x");
    case QwtSymbol::HLine:
        return QStringLiteral("_");
    case QwtSymbol::VLine:
        return QStringLiteral("|");
    case QwtSymbol::Star1:
    case QwtSymbol::Star2:
        return QStringLiteral("*");
    case QwtSymbol::Hexagon:
        return QStringLiteral("h");
    case QwtSymbol::NoSymbol:
    default:
        return QStringLiteral("None");
    }
}

// Python helpers for scales matplotlib has no name for, written once per script (needsScaleHelpers()).
// qtisas_scale(kind) is one ScaleEngine::newTransform() value transform; qtisas_break_scale() is
// ScaleEngineTransformation: each side of the break on its own scale, the gap collapsed to its centre.
inline void writeScaleHelpers(QTextStream &out)
{
    out << R"PY(from statistics import NormalDist as _NormalDist


def _qtisas_ppf(v):
    try:
        return _NormalDist().inv_cdf(0.01 * v)
    except (ValueError, ZeroDivisionError):
        return float("nan")


def qtisas_scale(kind):
    """QtiSAS value transform (forward, inverse) for set_xscale("function", functions=...)."""
    if kind == "log":
        return (lambda v: np.log(np.clip(np.asarray(v, dtype=float), 1e-150, 1e100)), np.exp)
    if kind == "neglog":  # a negative log half, positioned by log|v| (NegLogTransform), made increasing
        return (lambda v: -np.log(np.clip(-np.asarray(v, dtype=float), 1e-150, 1e100)), lambda t: -np.exp(-t))
    if kind == "reciprocal":
        def recip(v):
            v = np.asarray(v, dtype=float)
            return np.divide(1.0, v, out=np.zeros_like(v), where=v != 0)
        return (recip, recip)
    if kind == "probability":
        return (np.vectorize(_qtisas_ppf, otypes=[float]),
                np.vectorize(lambda t: 100.0 * _NormalDist().cdf(t), otypes=[float]))
    if kind == "logit":
        return (lambda v: np.log(np.asarray(v, dtype=float) / (100.0 - np.asarray(v, dtype=float))),
                lambda t: 100.0 * np.exp(t) / (1.0 + np.exp(t)))
    return (lambda v: np.asarray(v, dtype=float), lambda t: np.asarray(t, dtype=float))


def qtisas_break_scale(s1, s2, lb, rb, left, right, below, above):
    """QtiSAS axis break (ScaleEngineTransformation) as (forward, inverse)."""
    fb, ib = qtisas_scale(below)
    fa, ia = qtisas_scale(above)
    b0, b1 = fb(s1), fb(lb)
    a0, a1 = fa(rb), fa(s2)
    centre = 0.5 * (left + right)

    def forward(v):
        v = np.asarray(v, dtype=float)
        with np.errstate(all="ignore"):
            lo = (fb(v) - b0) / (b1 - b0) * left if b1 != b0 else np.zeros_like(v)
            hi = right + (fa(v) - a0) / (a1 - a0) * (1.0 - right) if a1 != a0 else np.full_like(v, right)
        return np.where(v <= lb, lo, np.where(v >= rb, hi, centre))

    def inverse(p):
        p = np.asarray(p, dtype=float)
        with np.errstate(all="ignore"):
            lo = ib(b0 + p / left * (b1 - b0)) if left > 0 else np.full_like(p, s1)
            hi = ia(a0 + (p - right) / (1.0 - right) * (a1 - a0)) if right < 1 else np.full_like(p, s2)
        return np.where(p <= left, lo, np.where(p >= right, hi, lb))

    return forward, inverse


)PY";
}

// qtisas_scale() kind for one resolved QtiSAS scale type (the base cancels in the positioning).
inline QString pyScaleKind(ScaleTransformation::Type type)
{
    if (ScaleEngine::isNegLog(type))
        return QStringLiteral("neglog");
    if (ScaleEngine::isLog(type))
        return QStringLiteral("log");
    switch (type)
    {
    case ScaleTransformation::Reciprocal:
        return QStringLiteral("reciprocal");
    case ScaleTransformation::Probability:
        return QStringLiteral("probability");
    case ScaleTransformation::Logit:
        return QStringLiteral("logit");
    default:
        return QStringLiteral("linear");
    }
}

// True when any axis of the layer uses qtisas_scale()/qtisas_break_scale() (see pyAxisScaleCall()).
inline bool needsScaleHelpers(Graph *g)
{
    for (int axis = 0; axis < QwtPlot::axisCnt; axis++)
    {
        auto *se = dynamic_cast<ScaleEngine *>(g->axisScaleEngine(axis));
        if (!se)
            continue;
        if (se->hasBreak() || se->type() == ScaleTransformation::Probability)
            return true;
        // An unbroken all-negative log range: the mirrored qtisas_scale("neglog").
        const QwtScaleDiv div = g->axisScaleDiv(axis);
        const ScaleTransformation::Type t = ScaleEngine::resolveHalfType(
            se->type(), qMin(div.lowerBound(), div.upperBound()), qMax(div.lowerBound(), div.upperBound()));
        if (ScaleEngine::isNegLog(t))
            return true;
    }
    return false;
}

// One axis's scale. A break: qtisas_break_scale() with the gap edges read from the live transform
// (T(lb), T(rb) are their axes fractions), each half resolved as ScaleEngine does, a negative log
// half mirrored. No break: log with its base, mirrored log, logit, reciprocal or probability.
inline QString pyAxisScaleCall(Graph *g, const QString &axVar, const QString &axisLetter, ScaleEngine *se, double lo,
                               double hi)
{
    if (!se)
        return {};

    const QString head = QStringLiteral("%1.set_%2scale(").arg(axVar, axisLetter);
    const double s1 = qMin(lo, hi);
    const double s2 = qMax(lo, hi);

    double lb = qBound(s1, se->axisBreakLeft(), s2);
    double rb = qBound(s1, se->axisBreakRight(), s2);
    if (lb > rb)
        qSwap(lb, rb);
    if (se->hasBreak() && lb > s1 && rb < s2 && lb < rb) // ScaleEngine::buildTransformation()'s own test
    {
        double left = -1.0;
        double right = -1.0;
        if (QwtTransform *tr = se->transformation())
        {
            // Only trusted when it really is this range's break transform (T(s1) = 0, T(s2) = 1).
            if (qAbs(tr->transform(s1)) < 1e-9 && qAbs(tr->transform(s2) - 1.0) < 1e-9)
            {
                left = tr->transform(lb);
                right = tr->transform(rb);
            }
            delete tr;
        }
        if (left < 0.0 || right <= left)
        {
            // Rebuilt as buildTransformation() does, the gap sized against the canvas length.
            const double length = axisLetter == QStringLiteral("x") ? g->canvas()->width() : g->canvas()->height();
            const double halfGap = qBound(0.0, length > 0 ? se->breakWidth() / length : se->breakWidth() / 400.0, 0.45);
            const double pos =
                qBound(halfGap + 0.01, qBound(0.0, se->breakPosition() / 100.0, 1.0), 1.0 - halfGap - 0.01);
            left = pos - halfGap;
            right = pos + halfGap;
        }
        const QString below = pyScaleKind(ScaleEngine::resolveHalfType(se->type(), s1, lb));
        const QString above = pyScaleKind(ScaleEngine::resolveHalfType(se->typeAfterBreak(), rb, s2));
        return head + QStringLiteral("\"function\", functions=qtisas_break_scale(%1, %2, %3, %4, %5, %6, %7, %8))\n")
                          .arg(pyNumber(s1), pyNumber(s2), pyNumber(lb), pyNumber(rb))
                          .arg(left, 0, 'g', 10)
                          .arg(right, 0, 'g', 10)
                          .arg(pyStringLiteral(below), pyStringLiteral(above));
    }

    const ScaleTransformation::Type type = ScaleEngine::resolveHalfType(se->type(), s1, s2);
    if (ScaleEngine::isNegLog(type))
        return head + QStringLiteral("\"function\", functions=qtisas_scale(\"neglog\"))\n");
    if (ScaleEngine::isLog(type))
    {
        QString base = QStringLiteral("10");
        if (type == ScaleTransformation::Ln)
            base = QStringLiteral("np.e");
        else if (type == ScaleTransformation::Log2)
            base = QStringLiteral("2");
        return head + QStringLiteral("\"log\", base=%1)\n").arg(base);
    }
    switch (type)
    {
    case ScaleTransformation::Logit:
        return head + QStringLiteral("\"logit\")\n");
    case ScaleTransformation::Reciprocal:
        return head + QStringLiteral("\"function\", functions=(lambda v: 1.0 / np.where(v == 0, np.nan, v), "
                                     "lambda v: 1.0 / np.where(v == 0, np.nan, v)))\n");
    case ScaleTransformation::Probability:
        return head + QStringLiteral("\"function\", functions=qtisas_scale(\"probability\"))\n");
    default:
        return {}; // linear
    }
}

// QtiSAS's pattern pitch: one 8 px tile per devicePixelRatio (VectorHatchBrush::nativeTilePitch()),
// so 4 logical px on a Retina screen, 8 on a standard one.
inline double patternPitchPx(Graph *g)
{
    const double dpr = g && g->devicePixelRatioF() > 0 ? g->devicePixelRatioF() : 1.0;
    return 8.0 / dpr;
}

// Line patterns as hatches at QtiSAS's pitch. A hatch tile is one inch with 6 lines per character,
// so the count is the nearest match to QtiSAS's lines per inch. Dense1-7 return "" (pyBrushFace()).
inline QString pyHatch(Qt::BrushStyle style, Graph *g)
{
    QString c;
    switch (style)
    {
    case Qt::HorPattern:
        c = QStringLiteral("-");
        break;
    case Qt::VerPattern:
        c = QStringLiteral("|");
        break;
    case Qt::CrossPattern:
        c = QStringLiteral("+");
        break;
    case Qt::BDiagPattern:
        c = QStringLiteral("/");
        break;
    case Qt::FDiagPattern:
        c = QStringLiteral("\\");
        break;
    case Qt::DiagCrossPattern:
        c = QStringLiteral("x");
        break;
    default:
        return {};
    }
    const double dpi = g && g->logicalDpiY() > 0 ? g->logicalDpiY() : 96.0;
    // matplotlib's diagonal hatches space their lines 2/n apart along the axis, not 1/n (NorthEastHatch
    // runs n+1 lines over offsets -1..1), while VectorHatchBrush steps a diagonal's intercept by one
    // pitch -- so a diagonal needs twice the characters for the same spacing.
    const bool diagonal = style == Qt::BDiagPattern || style == Qt::FDiagPattern || style == Qt::DiagCrossPattern;
    const int count = qMax(1, qRound(dpi / patternPitchPx(g) / 6.0 * (diagonal ? 2.0 : 1.0)));
    return c.repeated(count);
}

// VectorHatchBrush strokes each pattern line 0.5 logical px wide (a 0.25 half-width stripe).
inline double pyHatchLinewidthPt(Graph *g)
{
    return pxToPt(0.5, g);
}

// Face colour for a brush. Dense1-7 (8x8 dither bitmaps) have no hatch equivalent, so they become
// the brush colour at alpha x the bitmap's coverage: the colour the halftone averages to.
inline QString pyBrushFace(const QBrush &brush)
{
    if (brush.style() == Qt::NoBrush)
        return QStringLiteral("\"none\"");
    QColor c = brush.color();
    if (brush.style() >= Qt::Dense1Pattern && brush.style() <= Qt::Dense7Pattern)
    {
        static const double coverage[7] = {0.9375, 0.875, 0.625, 0.5, 0.375, 0.125, 0.0625};
        c.setAlphaF(static_cast<float>(c.alphaF() * coverage[brush.style() - Qt::Dense1Pattern]));
    }
    return pyColorRGBA(c);
}

// Brush layer without outline. A Qt pattern paints only its lines over a transparent background, and
// matplotlib hatches in the EDGE colour, so a hatched brush is facecolor="none", edgecolor=brush.
inline QString pyFillLayerArgs(const QBrush &brush, Graph *g)
{
    const QString hatch = pyHatch(brush.style(), g);
    if (hatch.isEmpty())
        return QStringLiteral("facecolor=%1, edgecolor=\"none\", linewidth=0").arg(pyBrushFace(brush));
    return QStringLiteral("facecolor=\"none\", edgecolor=%1, linewidth=0, hatch=%2")
        .arg(pyColorRGBA(brush.color()), pyStringLiteral(hatch));
}

// tick_params() for major or minor ticks of one axis: In/Out/Both map to matplotlib's directions,
// no ticks to length=0. Width is Graph::axesLinewidth(), colour Graph::axisColor(axisId), both in pt.
inline QString pyTickParams(const QString &axVar, const QString &axisLetter, const QString &which, int ticksStyle,
                            double penWidth, const QColor &color, double tickLengthPt)
{
    const QString colorArg =
        color.isValid() ? QStringLiteral(", color=%1").arg(pyStringLiteral(color.name(QColor::HexRgb))) : QString();

    if (ticksStyle == ScaleDraw::None)
        return QStringLiteral("%1.tick_params(axis=%2, which=%3, length=0)\n")
            .arg(axVar, pyStringLiteral(axisLetter), pyStringLiteral(which));

    QString direction;
    switch (ticksStyle)
    {
    case ScaleDraw::In:
        direction = QStringLiteral("in");
        break;
    case ScaleDraw::Both:
        direction = QStringLiteral("inout");
        break;
    case ScaleDraw::Out:
    default:
        direction = QStringLiteral("out");
        break;
    }
    return QStringLiteral("%1.tick_params(axis=%2, which=%3, direction=%4, width=%5, length=%6%7)\n")
        .arg(axVar, pyStringLiteral(axisLetter), pyStringLiteral(which), pyStringLiteral(direction))
        .arg(penWidth > 0 ? penWidth : 1.0, 0, 'g', 4)
        .arg(tickLengthPt > 0 ? tickLengthPt : 3.5, 0, 'g', 4)
        .arg(colorArg);
}

// The matplotlib spine sitting where a QwtPlot axis is drawn.
inline QString pySideName(int axisId)
{
    switch (axisId)
    {
    case QwtPlot::yRight:
        return QStringLiteral("right");
    case QwtPlot::xBottom:
        return QStringLiteral("bottom");
    case QwtPlot::xTop:
        return QStringLiteral("top");
    default:
        return QStringLiteral("left");
    }
}

// One spine shown with the given width/colour.
inline QString pySpineStyle(const QString &axVar, const QString &side, double widthPt, const QColor &color)
{
    QString s = QStringLiteral("%1.spines[%2].set_visible(True)\n").arg(axVar, pyStringLiteral(side));
    s += QStringLiteral("%1.spines[%2].set_linewidth(%3)\n").arg(axVar, pyStringLiteral(side)).arg(widthPt, 0, 'g', 4);
    if (color.isValid())
        s += QStringLiteral("%1.spines[%2].set_color(%3)\n")
                 .arg(axVar, pyStringLiteral(side), pyStringLiteral(color.name(QColor::HexRgb)));
    return s;
}

// Scale type, then range, of one axis. The scale div is already inverted for an inverted axis, so its
// bounds go to set_xlim() in their own order; set_xscale() first, since it re-runs autoscaling.
inline void writeAxisScale(QTextStream &out, Graph *g, const QString &axVar, int axisId, const QString &axisLetter)
{
    const QwtScaleDiv div = g->axisScaleDiv(axisId);
    auto *se = dynamic_cast<ScaleEngine *>(g->axisScaleEngine(axisId));
    out << pyAxisScaleCall(g, axVar, axisLetter, se, div.lowerBound(), div.upperBound());
    out << QStringLiteral("%1.set_%2lim(%3, %4)\n")
               .arg(axVar, axisLetter, pyNumber(div.lowerBound()), pyNumber(div.upperBound()));
}

// Title, scale, ticks, labels, break decoration and backbone of one QwtPlot axis on one matplotlib axes
// (the primary or a twin). axisEnabled=false keeps the scale and tick positions but draws no title,
// labels, decoration or spine.
inline void writeAxisProperties(QTextStream &out, Graph *g, const QString &axVar, int axisId, const QString &axisLetter,
                                bool axisEnabled = true)
{
    if (axisEnabled)
    {
        const QwtText axisTitle = g->axisTitle(axisId);
        if (!axisTitle.text().isEmpty())
        {
            QString extra = QStringLiteral(", fontsize=%1").arg(pyFontPointSize(axisTitle.font(), g), 0, 'g', 4);
            extra += pyFontFamilyArg(axisTitle.font());
            if (axisTitle.color().isValid())
                extra += QStringLiteral(", color=%1").arg(pyStringLiteral(axisTitle.color().name(QColor::HexRgb)));
            if (axisTitle.font().bold())
                extra += QStringLiteral(", fontweight=\"bold\"");
            if (axisTitle.font().italic())
                extra += QStringLiteral(", fontstyle=\"italic\"");
            out << QStringLiteral("%1.set_%2label(%3%4)\n")
                       .arg(axVar, axisLetter, pyStringLiteral(pyRichText(axisTitle.text())), extra);
        }
    }

    writeAxisScale(out, g, axVar, axisId, axisLetter);
    const QwtScaleDiv div = g->axisScaleDiv(axisId);

    auto *draw = dynamic_cast<ScaleDraw *>(g->axisScaleDraw(axisId));
    if (draw)
    {
        const double penWidth = pxToPt(g->axesLinewidth(), g);
        const QColor color = g->axisColor(axisId);
        const double majLen = pxToPt(g->majorTickLength(), g);
        const double minLen = pxToPt(g->minorTickLength(), g);
        out << pyTickParams(axVar, axisLetter, QStringLiteral("major"), draw->majorTicksStyle(), penWidth, color,
                            majLen);
        out << pyTickParams(axVar, axisLetter, QStringLiteral("minor"), draw->minorTicksStyle(), penWidth, color,
                            minLen);

        // Break decoration as ScaleDraw::drawBreak(): shown axes only, own side, one "/" per break edge.
        // An axis without its own break borrows the opposite axis's when no curve is plotted against it.
        auto *ownSe = dynamic_cast<ScaleEngine *>(g->axisScaleEngine(axisId));
        int srcAxisId = axisId;
        if (ownSe && !ownSe->hasBreak())
        {
            const bool isY = axisId == QwtPlot::yLeft || axisId == QwtPlot::yRight;
            bool hasCurves = false;
            foreach (QwtPlotItem *it, g->curvesList())
                hasCurves |= isY ? it->yAxis() == axisId : it->xAxis() == axisId;
            if (!hasCurves)
            {
                switch (axisId)
                {
                case QwtPlot::yLeft:
                    srcAxisId = QwtPlot::yRight;
                    break;
                case QwtPlot::yRight:
                    srcAxisId = QwtPlot::yLeft;
                    break;
                case QwtPlot::xBottom:
                    srcAxisId = QwtPlot::xTop;
                    break;
                default:
                    srcAxisId = QwtPlot::xBottom;
                    break;
                }
            }
        }
        auto *breakSe = dynamic_cast<ScaleEngine *>(g->axisScaleEngine(srcAxisId));
        auto *srcDraw = dynamic_cast<ScaleDraw *>(g->axisScaleDraw(srcAxisId));
        if (axisEnabled && breakSe && srcDraw && breakSe->hasBreak() && breakSe->hasBreakDecoration())
        {
            const QwtScaleMap map = srcDraw->scaleMap();
            const double canvasW = g->canvas()->width();
            const double canvasH = g->canvas()->height();
            const bool vertical = (axisLetter == QStringLiteral("y"));
            const double len = g->majorTickLength();
            const double dx = canvasW > 0 ? len / canvasW : 0.0;
            const double dy = canvasH > 0 ? len / canvasH : 0.0;
            const double at = (axisId == QwtPlot::yRight || axisId == QwtPlot::xTop) ? 1.0 : 0.0;

            for (double edge : {breakSe->axisBreakLeft(), breakSe->axisBreakRight()})
            {
                const double pix = map.transform(edge);
                if (!std::isfinite(pix))
                    continue;
                const double span = map.p2() - map.p1();
                if (qFuzzyIsNull(span))
                    continue;
                // p1 is the axis start: the left end, or the BOTTOM of a vertical axis (pixel y
                // grows downwards) -- so this is already matplotlib's bottom-left based fraction.
                const double frac = (pix - map.p1()) / span;
                // "/" in both orientations: from lower-left to upper-right.
                const QString xs = vertical
                                       ? QStringLiteral("[%1, %2]").arg(at - dx, 0, 'f', 4).arg(at + dx, 0, 'f', 4)
                                       : QStringLiteral("[%1, %2]").arg(frac - dx, 0, 'f', 4).arg(frac + dx, 0, 'f', 4);
                const QString ys = vertical
                                       ? QStringLiteral("[%1, %2]").arg(frac - dy, 0, 'f', 4).arg(frac + dy, 0, 'f', 4)
                                       : QStringLiteral("[%1, %2]").arg(at - dy, 0, 'f', 4).arg(at + dy, 0, 'f', 4);
                out << QStringLiteral("%1.add_line(mlines.Line2D(%2, %3, transform=%1.transAxes, "
                                      "color=%4, linewidth=%5, clip_on=False))\n")
                           .arg(axVar, xs, ys,
                                color.isValid() ? pyStringLiteral(color.name(QColor::HexRgb))
                                                : QStringLiteral("\"black\""))
                           .arg(penWidth, 0, 'f', 2);
            }
        }

        // Tick POSITIONS always, enabled or not: the grid is drawn at them, and leaving a hidden axis
        // to matplotlib's own locator put the grid lines somewhere QtiSAS never draws them.
        QStringList majPos, minPos;
        for (double v : div.ticks(QwtScaleDiv::MajorTick))
            majPos << pyNumber(v);
        for (int t = QwtScaleDiv::MinorTick; t < QwtScaleDiv::MajorTick; t++)
            for (double v : div.ticks(t))
                minPos << pyNumber(v);
        if (!majPos.isEmpty())
            out << QStringLiteral("%1.set_%2ticks([%3])\n").arg(axVar, axisLetter, majPos.join(QStringLiteral(", ")));
        if (!minPos.isEmpty())
        {
            out << QStringLiteral("%1.set_%2ticks([%3], minor=True)\n")
                       .arg(axVar, axisLetter, minPos.join(QStringLiteral(", ")));
            // Without this matplotlib's log minor formatter labels the minor ticks too
            // (2x10^-4, 3x10^-4, ...), which QtiSAS never shows -- only the majors carry text.
            out << QStringLiteral("%1.%2axis.set_minor_formatter(mticker.NullFormatter())\n").arg(axVar, axisLetter);
        }

        // Tick label appearance, and the tick text itself taken from this very ScaleDraw: matplotlib would
        // otherwise re-derive the labels with its own formatter, losing QtiSAS's numeric format/precision,
        // its log superscripts, and the day/month/time/date scales entirely.
        if (axisEnabled)
        {
            QString labelExtra =
                QStringLiteral(", labelsize=%1").arg(pyFontPointSize(g->axisFont(axisId), g), 0, 'g', 4);
            const QColor lc = g->axisLabelsColor(axisId);
            if (lc.isValid())
                labelExtra += QStringLiteral(", labelcolor=%1").arg(pyStringLiteral(lc.name(QColor::HexRgb)));
            // Qwt rotates labels clockwise for a positive angle (QPainter::rotate() on a y-down
            // device); matplotlib's labelrotation turns counter-clockwise.
            if (draw->labelRotation() != 0.0)
                labelExtra += QStringLiteral(", labelrotation=%1").arg(-draw->labelRotation(), 0, 'g', 4);
            out << QStringLiteral("%1.tick_params(axis=%2, which=\"both\"%3)\n")
                       .arg(axVar, pyStringLiteral(axisLetter), labelExtra);

            // Blank where ScaleDraw::drawLabel() draws nothing: inside the break [left, right), and the
            // first/last label under the axis's "hide begin/end" policy.
            const bool showLabels = draw->hasComponent(QwtAbstractScaleDraw::Labels);
            const QList<double> majors = div.ticks(QwtScaleDiv::MajorTick);
            const ScaleDraw::ShowTicksPolicy policy = draw->showTicksPolicy();
            QStringList majText;
            for (double v : majors)
            {
                bool hidden = !showLabels;
                if (ownSe && ownSe->hasBreak() && ownSe->axisBreakLeft() <= v && ownSe->axisBreakRight() > v)
                    hidden = true;
                if (v == majors.first() && (policy == ScaleDraw::HideBegin || policy == ScaleDraw::HideBeginEnd))
                    hidden = true;
                if (v == majors.last() && (policy == ScaleDraw::HideEnd || policy == ScaleDraw::HideBeginEnd))
                    hidden = true;
                majText << pyStringLiteral(hidden ? QString() : pyTickLabelText(draw->label(v).text()));
            }
            if (!majText.isEmpty())
                out << QStringLiteral("%1.set_%2ticklabels([%3])\n")
                           .arg(axVar, axisLetter, majText.join(QStringLiteral(", ")));
        }
        // A drawn backbone replaces this side's canvas-frame spine with the axis's own width/colour.
        if (axisEnabled && draw->hasComponent(QwtAbstractScaleDraw::Backbone))
            out << pySpineStyle(axVar, pySideName(axisId), penWidth, color);
    }
}

// Marker keyword arguments for a QwtSymbol, or marker "None". A symbol with NoBrush is hollow in
// QtiSAS; its brush colour is a meaningless default (black), so the face has to be "none" explicitly.
inline QString pySymbolArgs(const QwtSymbol *sym, Graph *g)
{
    if (!sym || sym->style() == QwtSymbol::NoSymbol)
        return QStringLiteral(", marker=\"None\"");
    const QString face =
        sym->brush().style() == Qt::NoBrush ? QStringLiteral("\"none\"") : pyColorRGBA(sym->brush().color());
    return QStringLiteral(", marker=%1, markersize=%2, markerfacecolor=%3, markeredgecolor=%4, markeredgewidth=%5")
        .arg(pyStringLiteral(pyMarker(sym->style())))
        .arg(pxToPt(sym->size().width(), g), 0, 'f', 1)
        .arg(face, pyColorRGBA(sym->pen().color()))
        .arg(pxToPt(sym->pen().widthF() > 0 ? sym->pen().widthF() : 1.0, g), 0, 'f', 2);
}

// Legend sample as LegendWidget::drawSymbol() draws it, as an expression (usable for another layer's
// curve): a filled swatch for bars/histograms/boxes (a tuple when hatched), else a line plus symbol.
inline QString pyLegendProxy(PlotCurve *cv, Graph *g)
{
    const QPen pen = cv->pen();
    const double lineWidth = pxToPt(pen.widthF() > 0 ? pen.widthF() : 1.0, g);
    const int type = cv->type();
    if (type == Graph::VerticalBars || type == Graph::HorizontalBars || type == Graph::Histogram || type == Graph::Box)
    {
        const QBrush &brush = cv->brush();
        const QString outline =
            pen.style() == Qt::NoPen
                ? QStringLiteral("edgecolor=\"none\", linewidth=0")
                : QStringLiteral("edgecolor=%1, linewidth=%2").arg(pyColorRGBA(pen.color())).arg(lineWidth, 0, 'f', 2);
        if (pyHatch(brush.style(), g).isEmpty())
            return QStringLiteral("mpatches.Patch(facecolor=%1, %2)").arg(pyBrushFace(brush), outline);
        return QStringLiteral("(mpatches.Patch(%1), "
                              "mpatches.Patch(facecolor=\"none\", %2))")
            .arg(pyFillLayerArgs(brush, g), outline);
    }

    const QwtSymbol *sym = cv->symbol();
    const QString sampleStyle =
        cv->style() == QwtPlotCurve::NoCurve ? QStringLiteral("None") : pyLineStyle(pen.style());
    // LegendWidget draws a symbol only when its pen is visible.
    const bool drawSymbol = sym && sym->pen().style() != Qt::NoPen;
    return QStringLiteral("mlines.Line2D([], [], color=%1, linestyle=%2, linewidth=%3%4)")
        .arg(pyStringLiteral(pen.color().name(QColor::HexRgb)), pyStringLiteral(sampleStyle))
        .arg(lineWidth, 0, 'f', 2)
        .arg(pySymbolArgs(drawSymbol ? sym : nullptr, g));
}

// A legend's curve reference, "n" or "L.n", resolved the way LegendWidget::getCurve() does: n is
// 1-based within the legend's own layer, L.n is curve n of layer L (1-based) of the same MultiLayer.
// Sets *owner to the layer the curve belongs to and *index to its 0-based index there.
inline PlotCurve *legendCurveRef(Graph *g, const QString &spec, Graph **owner, int *index)
{
    const QStringList parts = spec.trimmed().split(QLatin1Char('.'));
    Graph *layer = g;
    bool ok = false;
    int n = parts.last().toInt(&ok);
    if (!ok || parts.size() > 2)
        return nullptr;
    if (parts.size() == 2)
    {
        MultiLayer *ml = g->multiLayer();
        const int l = parts.first().toInt(&ok);
        layer = ok && ml ? ml->layer(l) : nullptr;
    }
    if (!layer || n < 1 || n > layer->curveCount())
        return nullptr;
    *owner = layer;
    *index = n - 1;
    return layer->curve(n - 1);
}

// A "%(n[,flags])" caption resolved like LegendWidget::parse(): @D curve title, @C column label,
// @W table name, @WL table label, @L/@U or no flag the column comment (label if empty).
// Function/Histogram curves have no table and always give their title.
inline QString resolveCurvePlaceholder(Graph *g, int curveIndex, const QString &flags)
{
    PlotCurve *c = g->curve(curveIndex);
    if (!c)
        return {};
    if (c->type() == Graph::Function || c->type() == Graph::Histogram)
        return c->title().text();

    const QString f = flags.toLower();
    if (f.contains(QStringLiteral("@d")))
        return c->title().text();

    auto *dc = dynamic_cast<DataCurve *>(c);
    Table *t = dc ? dc->table() : nullptr;
    if (!t)
        return c->title().text();

    const int ycol = t->colIndex(c->title().text());
    QString colLabel = ycol >= 0 ? t->colLabel(ycol) : c->title().text();
    QString commentOrLabel = ycol >= 0 && !t->comment(ycol).isEmpty()
                                 ? QString(t->comment(ycol)).replace(QLatin1Char('\n'), QLatin1Char(' '))
                                 : colLabel;

    if (f.contains(QStringLiteral("@wl")))
        return !t->windowLabel().isEmpty() ? t->windowLabel() : colLabel;
    if (f.contains(QStringLiteral("@w")))
        return t->name();
    if (f.contains(QStringLiteral("@l")) || f.contains(QStringLiteral("@u")))
        return commentOrLabel;
    if (f.contains(QStringLiteral("@c")))
        return colLabel;
    // No flag (the Auto display mode): the column comment, as LegendWidget shows it.
    return commentOrLabel;
}

// A LegendWidget text as plain text: "\l(..)"/"\p{..}" sample markup removed, "%(n)"/"%(L.n)"
// captions resolved (legendCurveRef()), pie-chart point references dropped.
inline QString resolveLegendPlaceholders(const QString &raw, Graph *g)
{
    QString s = raw;
    static const QRegularExpression sampleMarkup(QStringLiteral("\\\\l\\([^)]*\\)|\\\\p\\{[^}]*\\}"),
                                                 QRegularExpression::CaseInsensitiveOption);
    s.remove(sampleMarkup);

    static const QRegularExpression placeholder(QStringLiteral("%\\(([^)]*)\\)"));
    QString resolved;
    int lastEnd = 0;
    QRegularExpressionMatchIterator it = placeholder.globalMatch(s);
    while (it.hasNext())
    {
        const QRegularExpressionMatch m = it.next();
        resolved += s.mid(lastEnd, m.capturedStart() - lastEnd);
        const QStringList parts = m.captured(1).split(QLatin1Char(','));
        Graph *owner = nullptr;
        int curveIndex = -1;
        if (legendCurveRef(g, parts.value(0), &owner, &curveIndex))
            resolved += resolveCurvePlaceholder(owner, curveIndex, parts.mid(1).join(QLatin1Char(',')));
        lastEnd = static_cast<int>(m.capturedEnd());
    }
    resolved += s.mid(lastEnd);
    return resolved;
}

// FrameWidget::drawFrame()'s shadow strip width, in pixels (d_shadow_width, fixed, no accessor).
constexpr double frameShadowPx = 5.0;

// True when a layer has a text box, legend or rectangle drawn with the "Shadow" frame style -- the
// generated script then needs QtisasShadow (writeShadowHelper()). Ellipses never draw a shadow.
inline bool needsShadowHelper(Graph *g)
{
    foreach (FrameWidget *fw, g->enrichmentsList())
        if (fw && fw->isVisible() && fw->frameStyle() == FrameWidget::Shadow && !fw->inherits("EllipseWidget"))
            return true;
    return false;
}

// The "Shadow" frame style as a vector artist: FrameWidget::drawFrame() fills a black strip right of
// and below the box, offset by the shadow width. Measured from the target's own box at draw time,
// since a legend's or text box's size is only known once matplotlib lays it out; drawn just below it.
inline void writeShadowHelper(QTextStream &out)
{
    out << R"PY(import matplotlib.artist as _martist
from matplotlib.path import Path as _Path
from matplotlib.transforms import IdentityTransform as _IdentityTransform


class QtisasShadow(_martist.Artist):
    """QtiSAS "Shadow" frame: a black strip offset_pt wide, right of and below target's box."""

    def __init__(self, target, offset_pt):
        super().__init__()
        self._target = target
        self._offset = offset_pt
        self.set_zorder(target.get_zorder() - 0.01)

    def draw(self, renderer):
        t = self._target
        if hasattr(t, "get_bbox_patch") and t.get_bbox_patch() is not None:  # a text in its box
            t.update_bbox_position_size(renderer)
            bb = t.get_bbox_patch().get_window_extent(renderer)
        else:
            bb = t.get_window_extent(renderer)
        d = renderer.points_to_pixels(self._offset)
        gc = renderer.new_gc()
        gc.set_linewidth(0)
        for x0, y0, x1, y1 in ((bb.x1, bb.y0 - d, bb.x1 + d, bb.y1 - d), (bb.x0 + d, bb.y0 - d, bb.x1, bb.y0)):
            path = _Path([(x0, y0), (x1, y0), (x1, y1), (x0, y1), (x0, y0)], closed=True)
            renderer.draw_path(gc, path, _IdentityTransform(), (0.0, 0.0, 0.0, 1.0))
        gc.restore()


)PY";
}

// A FrameWidget's border -- its frame pen -- as matplotlib edge keyword arguments.
inline QString pyFrameEdgeArgs(FrameWidget *fw, Graph *g)
{
    return QStringLiteral("edgecolor=%1, linewidth=%2, linestyle=%3")
        .arg(pyColorRGBA(fw->frameColor()))
        .arg(pxToPt(fw->frameWidth() > 0 ? fw->frameWidth() : 1.0, g), 0, 'f', 2)
        .arg(pyStringLiteral(pyLineStyle(fw->frameLineStyle())));
}

// Bar width and the offset of its left/top edge from the sample, in PIXELS along the position axis,
// exactly as QwtBarCurve::drawSeries() computes them -- including its choice of neighbour pairs for
// the smallest spacing (it skips the second pair) and the half-canvas width of a single bar.
inline void barPixelGeometry(QwtBarCurve *bars, PlotCurve *cv, const QwtScaleMap &posMap, bool horizontal,
                             double *barWidth, double *halfWidth)
{
    const int size = static_cast<int>(cv->dataSize());
    auto px = [&](int i) { return posMap.transform(horizontal ? cv->sample(i).y() : cv->sample(i).x()); };
    double w = 1.0 - (bars ? bars->gap() : 0) * 0.01;
    if (size > 1)
    {
        double d = px(1) - px(0);
        if (horizontal)
            d = qAbs(d);
        for (int i = 2; i < size - 1; i++)
        {
            const double m = px(i + 1) - px(i);
            if (m <= d)
                d = m;
        }
        w *= d;
    }
    else
        w *= 0.5 * (posMap.transform(posMap.s2()) - posMap.transform(posMap.s1()));
    *barWidth = w;
    *halfWidth = (0.5 - (bars ? bars->offset() : 0) * 0.01) * w;
}

// QwtBarCurve::dataOffset() for a VISIBLE plot -- the data-unit shift ErrorBarsCurve applies to the
// error bars of an offset bar curve. Its own hidden-window branch returns a pixel-scaled value instead,
// so a Python export of a hidden graph would have moved the error bars off their bars.
inline double barDataOffset(QwtBarCurve *bars, Graph *g)
{
    if (!bars || bars->offset() == 0 || bars->dataSize() <= 0)
        return 0.0;
    const bool horizontal = bars->orientation() == QwtBarCurve::Horizontal;
    const QwtScaleMap map = g->canvasMap(horizontal ? bars->yAxis() : bars->xAxis());
    auto px = [&](int i) { return map.transform(horizontal ? bars->sample(i).y() : bars->sample(i).x()); };
    double w = 1.0 - bars->gap() * 0.01;
    if (bars->dataSize() > 1)
    {
        double d = px(1) - px(0);
        for (int i = 2; i < bars->dataSize(); i++)
        {
            const double m = px(i) - px(i - 1);
            if (m <= d)
                d = m;
        }
        w *= d;
    }
    else
        w *= 0.5 * (map.transform(map.s2()) - map.transform(map.s1()));
    const double minV = horizontal ? bars->minYValue() : bars->minXValue();
    return map.invTransform(map.transform(minV) + bars->offset() * 0.01 * w) - minV;
}

// Error bars segment by segment as ErrorBarsCurve::drawErrorBars() draws them (sides, through,
// skipped points, bar stack/offset); errorbar() can't, and doubles the cap length.
inline void writeErrorBars(QTextStream &out, ErrorBarsCurve *eb, Graph *g, const QString &cax, const QString &varName,
                           const QPen &pen, double lineWidthPt)
{
    DataCurve *master = eb ? eb->masterCurve() : nullptr;
    if (!master)
    {
        out << "# error bars without a master curve skipped\n\n";
        return;
    }
    const bool vertical = eb->direction() == ErrorBarsCurve::Vertical;
    const bool plus = eb->plusSide();
    const bool minus = eb->minusSide();
    const bool through = eb->throughSymbol();

    double xOffset = 0.0;
    double yOffset = 0.0;
    QwtBarCurve *barMaster = nullptr;
    QList<QwtBarCurve *> stack;
    if (master->type() == Graph::VerticalBars || master->type() == Graph::HorizontalBars)
    {
        barMaster = dynamic_cast<QwtBarCurve *>(master);
        (master->type() == Graph::VerticalBars ? xOffset : yOffset) = barDataOffset(barMaster, g);
        stack = barMaster->stackedCurvesList();
    }

    int skip = master->skipSymbolsCount() + eb->skipSymbolsCount();
    if (eb->skipSymbolsCount() > 0)
        skip--;
    if (skip <= 0)
        skip = 1;
    const int cap2 = (plus || minus) ? qRound(eb->capLength() * 0.5) : 0;

    QStringList segCross, segLo, segHi, capCross, capAlong;
    for (int i = 0; i < eb->dataSize(); i += skip)
    {
        const double err = eb->errorValue(i);
        if (err == 0.0)
            continue;
        double xStack = 0.0;
        double yStack = 0.0;
        if (!stack.isEmpty())
            (vertical ? yStack : xStack) = barMaster->stackOffset(i, stack);
        const double xval = eb->sample(i).x() + xStack;
        const double yval = eb->sample(i).y() + yStack;
        const double along = (vertical ? yval : xval) + (vertical ? yOffset : xOffset);
        const double cross = (vertical ? xval : yval) + (vertical ? xOffset : yOffset);

        auto segment = [&](double lo, double hi) {
            segCross << pyNumber(cross);
            segLo << pyNumber(lo);
            segHi << pyNumber(hi);
        };
        if (plus && (through || !minus))
            segment(along, along + err);
        if (minus && (through || !plus))
            segment(along - err, along);
        if (through && !plus && !minus)
            segment(along - err, along + err);
        if (cap2 > 0)
        {
            if (plus)
            {
                capCross << pyNumber(cross);
                capAlong << pyNumber(along + err);
            }
            if (minus)
            {
                capCross << pyNumber(cross);
                capAlong << pyNumber(along - err);
            }
        }
    }

    const QString color = pyColorRGBA(pen.color());
    if (!segCross.isEmpty())
    {
        out << QStringLiteral("%1_ec = [%2]\n").arg(varName, segCross.join(QStringLiteral(", ")));
        out << QStringLiteral("%1_elo = [%2]\n").arg(varName, segLo.join(QStringLiteral(", ")));
        out << QStringLiteral("%1_ehi = [%2]\n").arg(varName, segHi.join(QStringLiteral(", ")));
        // Square caps, like the pen drawErrorBars() sets up (Qt::SquareCap = matplotlib "projecting").
        out << QStringLiteral("%1.%2(%3_ec, %3_elo, %3_ehi, colors=%4, linewidth=%5, capstyle=\"projecting\")\n")
                   .arg(cax, vertical ? QStringLiteral("vlines") : QStringLiteral("hlines"), varName, color)
                   .arg(lineWidthPt, 0, 'f', 2);
    }
    if (!capCross.isEmpty())
    {
        out << QStringLiteral("%1_cc = [%2]\n").arg(varName, capCross.join(QStringLiteral(", ")));
        out << QStringLiteral("%1_ca = [%2]\n").arg(varName, capAlong.join(QStringLiteral(", ")));
        out << QStringLiteral("%1.plot(%2, linestyle=\"None\", marker=%3, markersize=%4, markeredgewidth=%5, "
                              "color=%6)\n")
                   .arg(cax, QString::fromLatin1(vertical ? "%1_cc, %1_ca" : "%1_ca, %1_cc").arg(varName),
                        vertical ? QStringLiteral("\"_\"") : QStringLiteral("\"|\""))
                   .arg(pxToPt(2 * cap2, g), 0, 'f', 2)
                   .arg(lineWidthPt, 0, 'f', 2)
                   .arg(color);
    }
    out << "\n";
}

// Script start shared by both exports: header, imports, the helpers any of the layers need, and the
// rcParams (font family, QtiSAS's 0.5 px pattern line width) taken from fontLayer.
inline void writeScriptPreamble(QTextStream &out, const QList<Graph *> &layers, Graph *fontLayer)
{
    // Python 3 reads source as UTF-8; Qt5 would write the locale's codec (cp1252 on Windows).
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#endif
    out << "# Generated by QtiSAS; requires matplotlib >= 3.3. Run it standalone (python file.py):\n";
    out << "# exec() inside QtiSAS loads matplotlib's Qt backend into QtiSAS's process and crashes the\n";
    out << "# next modal dialog. To preview from the QtiSAS console: subprocess.Popen([sys.executable, file]).\n";
    out << "import matplotlib.pyplot as plt\n";
    out << "import matplotlib.lines as mlines\n";
    out << "import matplotlib.patches as mpatches\n";
    out << "import matplotlib.ticker as mticker\n";
    out << "import numpy as np\n\n";

    bool scaleHelpers = false;
    bool shadowHelper = false;
    for (Graph *g : layers)
    {
        scaleHelpers |= needsScaleHelpers(g);
        shadowHelper |= needsShadowHelper(g);
    }
    if (scaleHelpers)
        writeScaleHelpers(out);
    if (shadowHelper)
        writeShadowHelper(out);

    if (fontLayer)
    {
        writeFontDefaults(out, fontLayer->axisFont(QwtPlot::yLeft));
        out << QStringLiteral("plt.rcParams[\"hatch.linewidth\"] = %1\n").arg(pyHatchLinewidthPt(fontLayer), 0, 'g', 4);
    }
}

// One layer onto an existing matplotlib axes: curves, axes, grid, title, texts, legends, arrows,
// shapes. Shared by the Graph and MultiLayer exports.
inline void writeMatplotlibLayer(QTextStream &out, Graph *g, const QString &axVar, int layerIndex)
{
    const QColor canvasColor = g->canvasBackground().color();
    out << QStringLiteral("%1.set_facecolor(%2)\n").arg(axVar, pyStringLiteral(canvasColor.name(QColor::HexRgb)));
    if (canvasColor.alphaF() < 1.0)
        out << QStringLiteral("%1.patch.set_alpha(%2)\n").arg(axVar).arg(canvasColor.alphaF(), 0, 'g', 3);

    // A canvas edge can have the canvas frame and the axis backbone; matplotlib has one spine per side.
    // The frame sets all four here; a drawn backbone overrides its own side in writeAxisProperties().
    {
        const double frameWidth = pxToPt(g->canvasFrameWidth(), g);
        const QColor frameColor = g->canvasFrameColor();
        const bool frameVisible = frameWidth > 0;
        for (const QString &side :
             {QStringLiteral("bottom"), QStringLiteral("top"), QStringLiteral("left"), QStringLiteral("right")})
        {
            out << QStringLiteral("%1.spines[%2].set_visible(%3)\n")
                       .arg(axVar, pyStringLiteral(side),
                            frameVisible ? QStringLiteral("True") : QStringLiteral("False"));
            if (!frameVisible)
                continue;
            out << QStringLiteral("%1.spines[%2].set_linewidth(%3)\n")
                       .arg(axVar, pyStringLiteral(side))
                       .arg(frameWidth, 0, 'g', 4);
            if (frameColor.isValid())
                out << QStringLiteral("%1.spines[%2].set_color(%3)\n")
                           .arg(axVar, pyStringLiteral(side), pyStringLiteral(frameColor.name(QColor::HexRgb)));
        }
    }

    // xTop/yRight get their own twin axes when shown or used by a curve: matplotlib styles both sides
    // of an Axis alike, and a curve on yRight needs yRight's scale. Created before the primary is
    // styled, since twinx()/twiny() re-enable the primary's ticks.
    bool curvesOnTop = false;
    bool curvesOnRight = false;
    bool curvesOnTopRight = false;
    foreach (QwtPlotItem *item, g->curvesList())
    {
        const bool top = item->xAxis() == QwtPlot::xTop;
        const bool right = item->yAxis() == QwtPlot::yRight;
        curvesOnTop |= top;
        curvesOnRight |= right;
        curvesOnTopRight |= top && right;
    }
    const bool topTwin = g->axisEnabled(QwtPlot::xTop) || curvesOnTop;
    const bool rightTwin = g->axisEnabled(QwtPlot::yRight) || curvesOnRight;
    const QString topVar = axVar + QStringLiteral("_top");
    const QString rightVar = axVar + QStringLiteral("_right");
    const QString topRightVar = axVar + QStringLiteral("_topright");
    // A twin brings four default spines of its own (black, 0.8pt) that would cover the canvas frame;
    // only its own side is shown again, by writeAxisProperties(), when that axis has a backbone.
    auto makeTwin = [&](const QString &var, const QString &call) {
        out << QStringLiteral("%1 = %2.%3()\n").arg(var, axVar, call);
        out << QStringLiteral("for _spine in %1.spines.values():\n    _spine.set_visible(False)\n").arg(var);
    };
    if (topTwin)
        makeTwin(topVar, QStringLiteral("twiny"));
    if (rightTwin)
        makeTwin(rightVar, QStringLiteral("twinx"));
    // A curve on xTop AND yRight: an invisible axes over the same rect, sharing x with the top twin
    // and y with the right one, so both of its coordinates follow the right scales.
    if (curvesOnTopRight)
        out << QStringLiteral("%1 = %2.figure.add_axes(%2.get_position(), sharex=%3, sharey=%4)\n%1.set_axis_off()\n")
                   .arg(topRightVar, axVar, topVar, rightVar);
    // Texts/legends/arrows go on the axes drawn LAST, so the curves of a twin don't cover them --
    // QtiSAS draws them above everything. All of these axes share one rect, so axes fractions hold.
    const QString annotVar = curvesOnTopRight ? topRightVar : rightTwin ? rightVar : topTwin ? topVar : axVar;
    auto curveAxes = [&](QwtPlotItem *item) {
        const bool top = item->xAxis() == QwtPlot::xTop;
        const bool right = item->yAxis() == QwtPlot::yRight;
        return top && right ? topRightVar : top ? topVar : right ? rightVar : axVar;
    };

    int curveIndex = 0;
    int curveNumber = 0;
    // Legend sample per curve: a proxy artist with the pen and symbol QtiSAS's sample shows.
    QMap<int, QString> curveHandle;
    foreach (QwtPlotItem *item, g->curvesList())
    {
        curveNumber++;
        if (item->rtti() == QwtPlotItem::Rtti_PlotSpectrogram)
        {
            out << "# spectrogram curve skipped (no matplotlib equivalent generated)\n";
            continue;
        }
        auto *cv = dynamic_cast<PlotCurve *>(item);
        if (!cv)
            continue;
        const QString cax = curveAxes(item);

        QString plotCall; // "plot" | "steps" | "bar" | "barh" | "droplines" | "errorbar"
        // Drop lines survive as Qwt's Sticks STYLE (Graph.cpp sets cl.connectType), while type()
        // stays Line/LineSymbols -- so switching on type() alone drew a connecting line instead.
        if (cv->style() == QwtPlotCurve::Sticks)
        {
            plotCall = QStringLiteral("droplines");
        }
        // Same for steps: the Steps STYLE is what Qwt draws, whatever type() reports -- a
        // VerticalSteps curve made by addCurve() still reports a plain line type.
        else if (cv->style() == QwtPlotCurve::Steps && cv->type() != Graph::ErrorBars)
        {
            plotCall = QStringLiteral("steps");
        }
        else
            switch (cv->type())
            {
            case Graph::Line:
            case Graph::Scatter:
            case Graph::LineSymbols:
            case Graph::Function:
            case Graph::Spline:
            case Graph::BSpline:
            case Graph::Bezier:
            case Graph::AkimaSpline:
            // An Area curve is a plain Lines curve plus a brush (Graph::updateCurveLayout()), which the
            // "plot" branch already fills down to the baseline under the line in the curve's pen.
            case Graph::Area:
                plotCall = QStringLiteral("plot");
                break;
            case Graph::HorizontalSteps:
            case Graph::VerticalSteps:
                plotCall = QStringLiteral("steps");
                break;
            case Graph::VerticalBars:
                plotCall = QStringLiteral("bar");
                break;
            case Graph::HorizontalBars:
                plotCall = QStringLiteral("barh");
                break;
            case Graph::Histogram:
                plotCall = QStringLiteral("bar");
                break;
            case Graph::VerticalDropLines:
                plotCall = QStringLiteral("droplines");
                break;
            case Graph::ErrorBars:
                plotCall = QStringLiteral("errorbar");
                break;
            default:
                out << QStringLiteral("# curve type %1 (%2) has no matplotlib equivalent -- skipped\n")
                           .arg(cv->type())
                           .arg(pyStringLiteral(cv->title().text()));
                continue;
            }

        if (!cv->isVisible())
        {
            out << QStringLiteral("# curve %1 skipped (hidden in QtiSAS)\n").arg(pyStringLiteral(cv->title().text()));
            continue;
        }

        const int size = static_cast<int>(cv->dataSize());
        QStringList xs, ys;
        xs.reserve(size);
        ys.reserve(size);
        for (int i = 0; i < size; ++i)
        {
            xs << pyNumber(cv->sample(i).x());
            ys << pyNumber(cv->sample(i).y());
        }

        // Fitted curves (spline/B-spline/Bezier/Akima): Qwt fits the mapped polyline in pixels, so map,
        // fit, invert for the line (_fx/_fy); symbols stay at the raw samples (_x/_y).
        QStringList fxs, fys;
        if (cv->testCurveAttribute(QwtPlotCurve::Fitted) && cv->curveFitter())
        {
            const QwtScaleMap xMap = g->canvasMap(cv->xAxis());
            const QwtScaleMap yMap = g->canvasMap(cv->yAxis());
            QPolygonF pixels;
            pixels.reserve(size);
            for (int i = 0; i < size; ++i)
                pixels << QPointF(xMap.transform(cv->sample(i).x()), yMap.transform(cv->sample(i).y()));
            for (const QPointF &pt : cv->curveFitter()->fitCurve(pixels))
            {
                fxs << pyNumber(xMap.invTransform(pt.x()));
                fys << pyNumber(yMap.invTransform(pt.y()));
            }
        }
        const bool fitted = !fxs.isEmpty();

        const QPen pen = cv->pen();
        const QString colorHex = pen.color().name(QColor::HexRgb);
        const double lineWidth = pxToPt(pen.widthF() > 0 ? pen.widthF() : 1.0, g);
        // label= metadata on the artists; escaped since a "$" in a column name would break mathtext.
        const QString label = pyRichText(cv->title().text());
        const QwtSymbol *sym = cv->symbol();
        const bool hasSymbol = sym && sym->style() != QwtSymbol::NoSymbol;

        const QString varName = QStringLiteral("layer%1_curve%2").arg(layerIndex).arg(curveIndex++);
        curveHandle[curveNumber] = varName + QStringLiteral("_lg");
        out << QStringLiteral("%1_x = [%2]\n").arg(varName, xs.join(QStringLiteral(", ")));
        out << QStringLiteral("%1_y = [%2]\n").arg(varName, ys.join(QStringLiteral(", ")));
        if (fitted)
        {
            out << QStringLiteral("%1_fx = [%2]\n").arg(varName, fxs.join(QStringLiteral(", ")));
            out << QStringLiteral("%1_fy = [%2]\n").arg(varName, fys.join(QStringLiteral(", ")));
        }

        out << QStringLiteral("%1_lg = %2\n").arg(varName, pyLegendProxy(cv, g));

        if (plotCall == QStringLiteral("plot"))
        {
            const bool noLine = cv->type() == Graph::Scatter || cv->style() == QwtPlotCurve::NoCurve;
            const QString lineStyle = noLine ? QStringLiteral("None") : pyLineStyle(pen.style());
            const QString lineXY =
                fitted ? QStringLiteral("%1_fx, %1_fy").arg(varName) : QStringLiteral("%1_x, %1_y").arg(varName);
            if (!fitted || !hasSymbol)
            {
                out << QStringLiteral("%1.plot(%2, color=%3, linestyle=%4, linewidth=%5%6, label=%7%8)\n")
                           .arg(cax, lineXY, pyStringLiteral(colorHex), pyStringLiteral(lineStyle))
                           .arg(lineWidth, 0, 'f', 2)
                           .arg(pySymbolArgs(sym, g), pyStringLiteral(label), pyAlphaArg(pen.color()));
            }
            else
            {
                out << QStringLiteral("%1.plot(%2, color=%3, linestyle=%4, linewidth=%5, label=%6%7)\n")
                           .arg(cax, lineXY, pyStringLiteral(colorHex), pyStringLiteral(lineStyle))
                           .arg(lineWidth, 0, 'f', 2)
                           .arg(pyStringLiteral(label), pyAlphaArg(pen.color()));
                out << QStringLiteral("%1.plot(%2_x, %2_y, linestyle=\"None\"%3%4)\n")
                           .arg(cax, varName, pySymbolArgs(sym, g), pyAlphaArg(pen.color()));
            }

            // A line curve can ALSO carry a brush, which Qwt fills between the (fitted) line and its
            // baseline (QwtPlotCurve::setBrush(), fillCurve()) -- this is also what an Area curve is.
            if (cv->brush().style() != Qt::NoBrush)
                out << QStringLiteral("%1.fill_between(%2, %3, %4)\n")
                           .arg(cax, lineXY, pyNumber(cv->baseline()), pyFillLayerArgs(cv->brush(), g));
            out << "\n";
        }
        else if (plotCall == QStringLiteral("steps"))
        {
            // Step direction as QwtPlotCurve::drawSteps() decides it (orientation and Inverted, not the
            // curve type): "inverted" = vertical segment first = matplotlib's "pre".
            bool inverted = cv->orientation() == Qt::Vertical;
            if (cv->testCurveAttribute(QwtPlotCurve::Inverted))
                inverted = !inverted;
            out << QStringLiteral("%1.step(%2_x, %2_y, where=%3, color=%4, linestyle=%5, linewidth=%6%7, "
                                  "label=%8%9)\n\n")
                       .arg(cax, varName, pyStringLiteral(inverted ? QStringLiteral("pre") : QStringLiteral("post")),
                            pyStringLiteral(colorHex), pyStringLiteral(pyLineStyle(pen.style())))
                       .arg(lineWidth, 0, 'f', 2)
                       .arg(pySymbolArgs(sym, g), pyStringLiteral(label), pyAlphaArg(pen.color()));
        }
        else if (plotCall == QStringLiteral("bar") || plotCall == QStringLiteral("barh"))
        {
            auto *bars = dynamic_cast<QwtBarCurve *>(cv);
            const bool horizontalBars = plotCall == QStringLiteral("barh");
            // barh(y, width): a horizontal bar's POSITION is its y sample.
            const QString lenVar = varName + (horizontalBars ? QStringLiteral("_x") : QStringLiteral("_y"));
            // Bar edges as QwtBarCurve::drawSeries() places them, in pixels, converted back per bar, so
            // widths and offsets stay right on non-linear axes.
            const QwtScaleMap posMap = g->canvasMap(horizontalBars ? cv->yAxis() : cv->xAxis());
            double barWidth = 0.0;
            double halfWidth = 0.0;
            barPixelGeometry(bars, cv, posMap, horizontalBars, &barWidth, &halfWidth);
            QStringList lefts, widths;
            for (int i = 0; i < size; ++i)
            {
                const double pos = horizontalBars ? cv->sample(i).y() : cv->sample(i).x();
                const double e1 = posMap.transform(pos) - halfWidth;
                const double d1 = posMap.invTransform(e1);
                const double d2 = posMap.invTransform(e1 + barWidth);
                lefts << pyNumber(qMin(d1, d2));
                widths << pyNumber(qAbs(d2 - d1));
            }
            out << QStringLiteral("%1_bpos = [%2]\n").arg(varName, lefts.join(QStringLiteral(", ")));
            out << QStringLiteral("%1_bw = [%2]\n").arg(varName, widths.join(QStringLiteral(", ")));
            QString xy = QStringLiteral("%1_bpos, %2, %3=%1_bw, align=\"edge\"")
                             .arg(varName, lenVar, horizontalBars ? QStringLiteral("height") : QStringLiteral("width"));

            // A stacked bar starts where the bars stacked below it end, per sample -- except a negative
            // one, which QwtBarCurve::drawSeries() still draws from 0.
            const QList<QwtBarCurve *> stack =
                bars && bars->isStacked() ? bars->stackedCurvesList() : QList<QwtBarCurve *>();
            if (!stack.isEmpty())
            {
                QStringList bottoms;
                bottoms.reserve(size);
                for (int i = 0; i < size; ++i)
                {
                    const double value = horizontalBars ? cv->sample(i).x() : cv->sample(i).y();
                    bottoms << pyNumber(value < 0 ? 0.0 : bars->stackOffset(i, stack));
                }
                out << QStringLiteral("%1_base = [%2]\n").arg(varName, bottoms.join(QStringLiteral(", ")));
                const QString baseParam = horizontalBars ? QStringLiteral("left") : QStringLiteral("bottom");
                xy += QStringLiteral(", %1=%2_base").arg(baseParam, varName);
            }

            // QwtBarCurve::drawSeries() fills each bar with the BRUSH and outlines it with the PEN.
            // Two layers when hatched, since matplotlib draws hatch lines in the edge colour: the
            // pattern (brush colour) first, then the outline (pen colour) over it.
            const QBrush &barBrush = cv->brush();
            const bool outlined = pen.style() != Qt::NoPen;
            const QString outlineArgs = outlined ? QStringLiteral("edgecolor=%1, linewidth=%2")
                                                       .arg(pyColorRGBA(pen.color()))
                                                       .arg(lineWidth, 0, 'f', 2)
                                                 : QStringLiteral("edgecolor=\"none\", linewidth=0");
            if (pyHatch(barBrush.style(), g).isEmpty())
            {
                out << QStringLiteral("%1.%2(%3, facecolor=%4, %5, label=%6)\n\n")
                           .arg(cax, plotCall, xy, pyBrushFace(barBrush), outlineArgs, pyStringLiteral(label));
            }
            else
            {
                out << QStringLiteral("%1.%2(%3, %4, label=%5)\n")
                           .arg(cax, plotCall, xy, pyFillLayerArgs(barBrush, g), pyStringLiteral(label));
                if (outlined)
                    out << QStringLiteral("%1.%2(%3, facecolor=\"none\", %4)\n").arg(cax, plotCall, xy, outlineArgs);
                out << "\n";
            }
        }
        else if (plotCall == QStringLiteral("droplines"))
        {
            out << QStringLiteral("%1.vlines(%2_x, %3, %2_y, color=%4, linewidth=%5, linestyle=%6, label=%7%8)\n")
                       .arg(cax, varName, pyNumber(cv->baseline()), pyStringLiteral(colorHex))
                       .arg(lineWidth, 0, 'f', 2)
                       .arg(pyStringLiteral(pyLineStyle(pen.style())), pyStringLiteral(label), pyAlphaArg(pen.color()));
            // Sticks suppresses the connecting line but NOT the symbols, which Qwt still draws.
            if (hasSymbol)
                out << QStringLiteral("%1.plot(%2_x, %2_y, linestyle=\"None\"%3)\n")
                           .arg(cax, varName, pySymbolArgs(sym, g));
            out << "\n";
        }
        else if (plotCall == QStringLiteral("errorbar"))
            writeErrorBars(out, dynamic_cast<ErrorBarsCurve *>(cv), g, cax, varName, pen, lineWidth);
    }

    // Scale applied even to a disabled axis: it still positions the data (e.g. common axes).
    // Hidden per side with tick_params(), not set_visible(), which would hide both sides.
    const bool xBottomEnabled = g->axisEnabled(QwtPlot::xBottom);
    writeAxisProperties(out, g, axVar, QwtPlot::xBottom, QStringLiteral("x"), xBottomEnabled);
    if (!xBottomEnabled)
        out << QStringLiteral("%1.tick_params(axis=\"x\", which=\"both\", bottom=False, labelbottom=False)\n")
                   .arg(axVar);

    const bool yLeftEnabled = g->axisEnabled(QwtPlot::yLeft);
    writeAxisProperties(out, g, axVar, QwtPlot::yLeft, QStringLiteral("y"), yLeftEnabled);
    if (!yLeftEnabled)
        out << QStringLiteral("%1.tick_params(axis=\"y\", which=\"both\", left=False, labelleft=False)\n").arg(axVar);

    // xTop/yRight on their own twins (created above): full properties through the same
    // writeAxisProperties() as the primary. A twin that exists only because a curve uses its scale
    // (the axis itself disabled) keeps the scale and hides its ticks, as QtiSAS does.
    auto writeSecondary = [&](int axisId, const QString &axisLetter, const QString &twinVar, const QString &side) {
        const bool enabled = g->axisEnabled(axisId);
        writeAxisProperties(out, g, twinVar, axisId, axisLetter, enabled);
        if (!enabled)
            out << QStringLiteral("%1.tick_params(axis=%2, which=\"both\", %3=False, label%3=False)\n")
                       .arg(twinVar, pyStringLiteral(axisLetter), side);
    };
    if (topTwin)
        writeSecondary(QwtPlot::xTop, QStringLiteral("x"), topVar, QStringLiteral("top"));
    if (rightTwin)
        writeSecondary(QwtPlot::yRight, QStringLiteral("y"), rightVar, QStringLiteral("right"));

    Grid *grid = g->grid();
    if (grid)
    {
        // Style and z-order too: colour/width alone lose dashed grids, and QtiSAS can draw the grid
        // over the curves, which is matplotlib's non-default (axisbelow=True).
        auto gridLine = [&](const QString &axis, const QString &which, const QPen &pen, double fallback) {
            out << QStringLiteral("%1.grid(True, axis=%2, which=%3, color=%4, linewidth=%5, linestyle=%6%7)\n")
                       .arg(axVar, pyStringLiteral(axis), pyStringLiteral(which),
                            pyStringLiteral(pen.color().name(QColor::HexRgb)))
                       .arg(pxToPt(pen.widthF() > 0 ? pen.widthF() : fallback, g), 0, 'f', 2)
                       .arg(pyStringLiteral(pyLineStyle(pen.style())), pyAlphaArg(pen.color()));
        };
        if (grid->xEnabled())
            gridLine(QStringLiteral("x"), QStringLiteral("major"), grid->majPenX(), 1.0);
        if (grid->xMinEnabled())
            gridLine(QStringLiteral("x"), QStringLiteral("minor"), grid->minPenX(), 0.5);
        if (grid->yEnabled())
            gridLine(QStringLiteral("y"), QStringLiteral("major"), grid->majPenY(), 1.0);
        if (grid->yMinEnabled())
            gridLine(QStringLiteral("y"), QStringLiteral("minor"), grid->minPenY(), 0.5);
        // No minorticks_on(): the minor grid runs along the minor ticks writeAxisProperties() already
        // placed at QtiSAS's own positions, and minorticks_on() would swap in matplotlib's locator.
        out << QStringLiteral("%1.set_axisbelow(%2)\n")
                   .arg(axVar, g->hasGridOnTop() ? QStringLiteral("False") : QStringLiteral("True"));
    }

    const QwtText gTitle = g->title();
    if (!gTitle.text().isEmpty())
    {
        QString titleExtra = QStringLiteral(", fontsize=%1").arg(pyFontPointSize(gTitle.font(), g), 0, 'g', 4);
        titleExtra += pyFontFamilyArg(gTitle.font());
        if (gTitle.color().isValid())
            titleExtra += QStringLiteral(", color=%1").arg(pyStringLiteral(gTitle.color().name(QColor::HexRgb)));
        if (gTitle.font().bold())
            titleExtra += QStringLiteral(", fontweight=\"bold\"");
        if (gTitle.font().italic())
            titleExtra += QStringLiteral(", fontstyle=\"italic\"");
        out << QStringLiteral("%1.set_title(%2%3)\n")
                   .arg(axVar, pyStringLiteral(pyRichText(gTitle.text())), titleExtra);
    }

    // Text boxes and legends at their real position, font and frame. The widgets live in the
    // MultiLayer canvas's pixel space, like g->pos(), hence the canvas-origin offset.
    const QPoint canvasOrigin = g->pos() + g->canvas()->pos();
    const int canvasW = g->canvas()->width();
    const int canvasH = g->canvas()->height();
    const auto texts = g->textsList();
    for (LegendWidget *lw : texts)
    {
        if (!lw->isVisible())
            continue;

        const double fracX = canvasW > 0 ? double(lw->pos().x() - canvasOrigin.x()) / canvasW : 0.0;
        const double fracYFromTop = canvasH > 0 ? double(lw->pos().y() - canvasOrigin.y()) / canvasH : 0.0;
        const double fracY = 1.0 - fracYFromTop; // matplotlib axes-fraction origin is bottom-left
        const QFont font = lw->font();
        const bool framed = lw->frameStyle() != FrameWidget::None;

        // A curve legend contains a "\l(n)" sample; isAutoUpdateEnabled() would also match an
        // auto-updated fit-result box.
        if (lw->text().contains(QStringLiteral("\\l("), Qt::CaseInsensitive))
        {
            // Curve legend: becomes a legend() call, each entry a sample plus its caption.
            QString extra;
            // legend() takes neither fontfamily nor a fontsize alongside prop: the family has to
            // travel inside prop, which then carries the size too.
            if (font.family().isEmpty())
                extra += QStringLiteral(", fontsize=%1").arg(pyFontPointSize(font, g), 0, 'g', 4);
            else
                extra += QStringLiteral(", prop={\"family\": %1, \"size\": %2}")
                             .arg(pyStringLiteral(font.family()))
                             .arg(pyFontPointSize(font, g), 0, 'g', 4);
            // FrameWidget::drawFrame(): square corners; "None" still fills the background, only the
            // border goes; the border's width/line style are set on the frame patch after creation.
            const bool bgFilled = lw->backgroundColor().alpha() > 0;
            extra += QStringLiteral(", fancybox=False, frameon=%1")
                         .arg(framed || bgFilled ? QStringLiteral("True") : QStringLiteral("False"));
            if (framed || bgFilled)
                extra += QStringLiteral(", facecolor=%1, edgecolor=%2")
                             .arg(bgFilled ? pyColorRGBA(lw->backgroundColor()) : QStringLiteral("\"none\""),
                                  framed ? pyColorRGBA(lw->frameColor()) : QStringLiteral("\"none\""));

            // One entry per text line; "\l(n)"/"\l(L.n)" gives the sample, a line without one keeps an
            // empty handle. The caption is the user's text, not the column name.
            static const QRegularExpression sampleRe(QStringLiteral("\\\\l\\(\\s*([\\d.\\s]+)\\)"),
                                                     QRegularExpression::CaseInsensitiveOption);
            const QString noHandle = QStringLiteral("mlines.Line2D([], [], linestyle=\"None\", marker=\"None\")");
            QStringList handles, labels;
            for (const QString &line : lw->text().split(QLatin1Char('\n')))
            {
                QString handle;
                const QRegularExpressionMatch m = sampleRe.match(line);
                if (m.hasMatch())
                {
                    Graph *owner = nullptr;
                    int index = -1;
                    if (PlotCurve *ref = legendCurveRef(g, m.captured(1), &owner, &index))
                    {
                        // Own layer: the proxy written next to the curve (absent = curve not exported).
                        // Another layer: an inline proxy, since that layer's variables may not exist yet.
                        if (owner != g)
                            handle = pyLegendProxy(ref, owner);
                        else if (curveHandle.contains(index + 1))
                            handle = curveHandle.value(index + 1);
                    }
                }
                QString caption = line;
                caption.remove(sampleRe);
                caption = resolveLegendPlaceholders(caption, g).trimmed();
                if (handle.isEmpty() && caption.isEmpty())
                    continue;
                handles << (handle.isEmpty() ? noHandle : handle);
                labels << pyStringLiteral(pyRichText(caption));
            }

            // Numbers first, user text last: every later .arg() would substitute into a caption
            // that itself contains "%1".."%9".
            if (!handles.isEmpty())
            {
                out << QStringLiteral("_leg = %3.legend([%4], [%5], loc=\"upper left\", bbox_to_anchor=(%1, %2)%6)\n")
                           .arg(fracX, 0, 'f', 4)
                           .arg(fracY, 0, 'f', 4)
                           .arg(annotVar, handles.join(QStringLiteral(", ")), labels.join(QStringLiteral(", ")), extra);
                if (framed)
                    out << QStringLiteral("_leg.get_frame().set_linewidth(%1)\n_leg.get_frame().set_linestyle(%2)\n")
                               .arg(pxToPt(lw->frameWidth() > 0 ? lw->frameWidth() : 1.0, g), 0, 'f', 2)
                               .arg(pyStringLiteral(pyLineStyle(lw->frameLineStyle())));
                // add_artist(): a second legend() call on the same axes would otherwise replace this one.
                out << QStringLiteral("%1.add_artist(_leg)\n").arg(annotVar);
                if (lw->frameStyle() == FrameWidget::Shadow)
                    out << QStringLiteral("%1.add_artist(QtisasShadow(_leg, %2))\n")
                               .arg(annotVar)
                               .arg(pxToPt(frameShadowPx, g), 0, 'g', 4);
            }
            continue;
        }

        const QString text = resolveLegendPlaceholders(lw->text(), g);
        if (text.trimmed().isEmpty())
            continue;

        QString extra;
        if (lw->textColor().isValid())
            extra += QStringLiteral(", color=%1").arg(pyStringLiteral(lw->textColor().name(QColor::HexRgb)));
        extra += QStringLiteral(", fontsize=%1").arg(pyFontPointSize(font, g), 0, 'g', 4);
        if (font.bold())
            extra += QStringLiteral(", fontweight=\"bold\"");
        if (font.italic())
            extra += QStringLiteral(", fontstyle=\"italic\"");
        if (lw->angle() != 0)
            extra += QStringLiteral(", rotation=%1").arg(lw->angle());

        // Box as FrameWidget::drawFrame() draws it: "None" keeps the background fill without a border.
        QString bboxArg;
        const bool bgFilled = lw->backgroundColor().alpha() > 0;
        if (framed || bgFilled)
            bboxArg = QStringLiteral(", bbox=dict(boxstyle=\"square,pad=0.3\", facecolor=%1, %2)")
                          .arg(bgFilled ? pyColorRGBA(lw->backgroundColor()) : QStringLiteral("\"none\""),
                               framed ? pyFrameEdgeArgs(lw, g) : QStringLiteral("edgecolor=\"none\", linewidth=0"));

        out << QStringLiteral("_txt = %1.text(%2, %3, %4, transform=%1.transAxes, ha=\"left\", va=\"top\"%5%6)\n")
                   .arg(annotVar)
                   .arg(fracX, 0, 'f', 4)
                   .arg(fracY, 0, 'f', 4)
                   .arg(pyStringLiteral(pyRichText(text)), extra, bboxArg);
        if (lw->frameStyle() == FrameWidget::Shadow)
            out << QStringLiteral("%1.add_artist(QtisasShadow(_txt, %2))\n")
                       .arg(annotVar)
                       .arg(pxToPt(frameShadowPx, g), 0, 'g', 4);
    }
    // Arrows/lines (Graph::arrowsList()) -- kept in data coordinates, which is how ArrowMarker stores
    // its endpoints, so they track the curves the same way they do in QtiSAS.
    foreach (ArrowMarker *arrow, g->arrowsList())
    {
        if (!arrow || !arrow->isVisible())
            continue;
        const QPointF from = arrow->startPointCoord();
        const QPointF to = arrow->endPointCoord();
        const QPen ap = arrow->linePen();
        QString style = QStringLiteral("-");
        if (arrow->hasEndArrow())
            style = arrow->hasStartArrow() ? QStringLiteral("<|-|>") : QStringLiteral("-|>");
        else if (arrow->hasStartArrow())
            style = QStringLiteral("<|-");
        // Drawn on the top axes but placed by the primary's data coordinates -- ArrowMarker lives on
        // xBottom/yLeft -- so it stays above a twin's curves yet tracks the data it points at.
        out << QStringLiteral("%1.annotate(\"\", xy=(%2, %3), xytext=(%4, %5), xycoords=%10.transData, "
                              "textcoords=%10.transData, arrowprops=dict(arrowstyle=%6, "
                              "color=%7, linewidth=%8, linestyle=%9))\n")
                   .arg(annotVar, pyNumber(to.x()), pyNumber(to.y()), pyNumber(from.x()), pyNumber(from.y()),
                        pyStringLiteral(style), pyStringLiteral(ap.color().name(QColor::HexRgb)))
                   .arg(pxToPt(ap.widthF() > 0 ? ap.widthF() : 1.0, g), 0, 'f', 2)
                   .arg(pyStringLiteral(pyLineStyle(ap.style())))
                   .arg(axVar);
    }

    // Rectangles and ellipses; other enrichments (images, TeX) are named in a script comment.
    foreach (FrameWidget *fw, g->enrichmentsList())
    {
        if (!fw || !fw->isVisible() || qobject_cast<LegendWidget *>(fw))
            continue;

        const bool isEllipse = fw->inherits("EllipseWidget");
        const bool isRect = fw->inherits("RectangleWidget");
        if (!isEllipse && !isRect)
        {
            out << QStringLiteral("# enrichment %1 not reproduced (no matplotlib equivalent generated)\n")
                       .arg(pyStringLiteral(fw->metaObject()->className()));
            continue;
        }

        // Placed like the text boxes above: these widgets live in the multi-layer canvas' pixel
        // space, so convert to this axes' fraction and draw in axes coordinates.
        const double x0 = canvasW > 0 ? double(fw->pos().x() - canvasOrigin.x()) / canvasW : 0.0;
        const double yTop = canvasH > 0 ? double(fw->pos().y() - canvasOrigin.y()) / canvasH : 0.0;
        const double w = canvasW > 0 ? double(fw->width()) / canvasW : 0.0;
        const double h = canvasH > 0 ? double(fw->height()) / canvasH : 0.0;
        const double y0 = 1.0 - yTop - h; // matplotlib anchors a patch at its BOTTOM-left corner

        // Layered as drawFrame(): background, brush, border. Rectangle "Shadow" shrinks the box by the
        // strip width so box + strip fill the widget; an ellipse only outlines with "Line", never shadows.
        const int frameStyle = fw->frameStyle();
        const bool shadow = isRect && frameStyle == FrameWidget::Shadow;
        const bool outlined = isEllipse ? frameStyle == FrameWidget::Line : frameStyle != FrameWidget::None;
        double bw = w;
        double bh = h;
        double by0 = y0;
        if (shadow && canvasW > 0 && canvasH > 0)
        {
            bw -= frameShadowPx / canvasW;
            bh -= frameShadowPx / canvasH;
            by0 += frameShadowPx / canvasH;
        }
        auto addPatch = [&](const QString &args) {
            const QString shape = isEllipse ? QStringLiteral("mpatches.Ellipse((%1, %2), %3, %4")
                                                  .arg(x0 + bw / 2.0, 0, 'f', 4)
                                                  .arg(by0 + bh / 2.0, 0, 'f', 4)
                                                  .arg(bw, 0, 'f', 4)
                                                  .arg(bh, 0, 'f', 4)
                                            : QStringLiteral("mpatches.Rectangle((%1, %2), %3, %4")
                                                  .arg(x0, 0, 'f', 4)
                                                  .arg(by0, 0, 'f', 4)
                                                  .arg(bw, 0, 'f', 4)
                                                  .arg(bh, 0, 'f', 4);
            out << QStringLiteral("_fw = %1.add_patch(%2, %3, transform=%1.transAxes, clip_on=False))\n")
                       .arg(annotVar, shape, args);
        };
        const QColor bg = fw->backgroundColor();
        if (bg.isValid() && bg.alpha() > 0)
            addPatch(QStringLiteral("facecolor=%1, edgecolor=\"none\", linewidth=0").arg(pyColorRGBA(bg)));
        if (fw->brush().style() != Qt::NoBrush)
            addPatch(pyFillLayerArgs(fw->brush(), g));
        if (outlined)
            addPatch(QStringLiteral("facecolor=\"none\", ") + pyFrameEdgeArgs(fw, g));
        if (shadow)
            out << QStringLiteral("%1.add_artist(QtisasShadow(_fw, %2))\n")
                       .arg(annotVar)
                       .arg(pxToPt(frameShadowPx, g), 0, 'g', 4);
    }

    out << "\n";
}
} // namespace MatplotlibExport

#endif
