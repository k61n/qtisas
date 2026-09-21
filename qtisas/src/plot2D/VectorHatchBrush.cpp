/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Vector (line/dot based) replacement for Qt's builtin hatch brush
             styles during print/export, where QSvgGenerator/QPrinter render
             them at the wrong tile density compared to on-screen painting.
 ******************************************************************************/

#include <algorithm>
#include <cmath>

#include <zlib.h>

#include <QByteArray>
#include <QColor>
#include <QLineF>
#include <QPainter>
#include <QPainterPath>
#include <QPair>
#include <QPdfWriter>
#include <QRegularExpression>
#include <QSvgGenerator>
#include <QVector>
#include <QWidget>

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

// Set by resetPatternRegistry() at the start of a whole export: SVG/PDF export devices always report
// devicePixelRatioF() == 1 themselves (real screens don't), so nativeTilePitch() can't tell
// "exporting" from "genuinely on a DPR-1 screen" from the device alone -- this is the actual
// triggering screen's DPR, needed so export density matches what that same screen's live graph shows.
double g_exportReferenceDpr = 0.0;

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

QByteArray zlibInflate(const QByteArray &in)
{
    uLongf destLen = static_cast<uLongf>(in.size()) * 8 + 1024;
    for (int attempt = 0; attempt < 6; ++attempt)
    {
        QByteArray out(static_cast<int>(destLen), Qt::Uninitialized);
        uLongf actual = destLen;
        const int rc = uncompress(reinterpret_cast<Bytef *>(out.data()), &actual,
                                  reinterpret_cast<const Bytef *>(in.constData()), static_cast<uLong>(in.size()));
        if (rc == Z_OK)
        {
            out.resize(static_cast<int>(actual));
            return out;
        }
        if (rc != Z_BUF_ERROR)
            return {};
        destLen *= 4;
    }
    return {};
}

QByteArray zlibDeflate(const QByteArray &in)
{
    uLongf destLen = compressBound(static_cast<uLong>(in.size()));
    QByteArray out(static_cast<int>(destLen), Qt::Uninitialized);
    if (compress2(reinterpret_cast<Bytef *>(out.data()), &destLen, reinterpret_cast<const Bytef *>(in.constData()),
                  static_cast<uLong>(in.size()), Z_BEST_COMPRESSION) != Z_OK)
        return {};
    out.resize(static_cast<int>(destLen));
    return out;
}

// Byte offset right after "<objNum> 0 obj", or -1. Rejects a match that's actually a longer number's
// suffix (e.g. "11 0 obj" matching inside "111 0 obj").
int findObjectBody(const QByteArray &pdf, int objNum)
{
    const QByteArray needle = QByteArray::number(objNum) + " 0 obj";
    int pos = 0;
    while (true)
    {
        pos = static_cast<int>(pdf.indexOf(needle, pos));
        if (pos < 0)
            return -1;
        if (pos == 0 || pdf.at(pos - 1) < '0' || pdf.at(pos - 1) > '9')
            return pos + static_cast<int>(needle.size());
        pos += 1;
    }
}

double luminance(const QColor &c)
{
    return 0.299 * c.redF() + 0.587 * c.greenF() + 0.114 * c.blueF();
}

// One registry entry's pattern tile content stream: merged rects for the "on" cells (same row-band +
// run-length technique as patternTileRectsSvg()), color baked directly in via a leading rg. A
// PaintType 2 (uncoloured) pattern -- color supplied externally via scn -- would need only one
// shared object per Dense level, but was confirmed to print wrong (color and opacity both ignored,
// falling back to black) on at least one real printer's PDF/PostScript pipeline: PaintType 1
// (coloured) patterns are a far more basic, universally-supported feature, at the cost of one
// pattern object per distinct (color, level) combination instead of one per level.
QByteArray patternTileContentPdf(int level, const QColor &color, bool grayscale)
{
    QByteArray s;
    if (grayscale)
    {
        const QByteArray g = QByteArray::number(luminance(color), 'f', 6);
        s = g + " " + g + " " + g + " rg\n";
    }
    else
        s = QByteArray::number(color.redF(), 'f', 6) + " " + QByteArray::number(color.greenF(), 'f', 6) + " " +
            QByteArray::number(color.blueF(), 'f', 6) + " rg\n";
    for (int row = 0; row < 8;)
    {
        const quint8 mask = g_denseBits[level][row];
        int rowSpan = 1;
        while (row + rowSpan < 8 && g_denseBits[level][row + rowSpan] == mask)
            ++rowSpan;
        if (mask)
        {
            const int y = 8 - (row + rowSpan); // PDF pattern space y grows upward; bitmap row 0 is the top.
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
                s += QByteArray::number(start) + " " + QByteArray::number(y) + " " + QByteArray::number(col - start) +
                     " " + QByteArray::number(rowSpan) + " re\n";
            }
        }
        row += rowSpan;
    }
    s += "f\n";
    return s;
}
} // namespace

double nativeTilePitch(QPainter *p)
{
    // A real on-screen widget reports its own actual DPR; an export device (QSvgGenerator/
    // QPdfWriter) always reports 1 regardless of the triggering screen, so it falls back to the
    // reference DPR resetPatternRegistry() was given for this export, if any.
    const bool isExportDevice = p && p->device() && dynamic_cast<QWidget *>(p->device()) == nullptr;
    const qreal dpr = (isExportDevice && g_exportReferenceDpr > 0.0) ? g_exportReferenceDpr
                      : (p && p->device())                           ? p->device()->devicePixelRatioF()
                                                                     : 1.0;
    return 8.0 / (dpr > 0.0 ? dpr : 1.0);
}

void resetPatternRegistry(double exportReferenceDpr)
{
    patternRegistry().clear();
    g_exportReferenceDpr = exportReferenceDpr;
}

QByteArray applyPatternRegistry(const QByteArray &svg)
{
    const QVector<PatternEntry> &registry = patternRegistry();
    if (registry.isEmpty())
        return svg;

    // Explicit length, not QString::fromUtf8(svg) -- see applyPatternRegistryPdf()'s matching comment.
    QString s = QString::fromUtf8(svg.constData(), static_cast<int>(svg.size()));
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

QByteArray applyPatternRegistryPdf(const QByteArray &pdf, bool grayscale)
{
    const QVector<PatternEntry> &registry = patternRegistry();
    if (registry.isEmpty() && !grayscale)
    {
        return pdf;
    }

    // Our exports are always single-page, so the first /Contents and /Resources references belong
    // to that one page. Explicit length, not QString::fromLatin1(pdf) -- that overload resolves to
    // the (const char *, int size = -1) form via QByteArray's implicit conversion, defaulting to
    // strlen() and truncating at the first embedded null byte in the compressed stream data (which
    // can appear anywhere in the file, including before the page's own /Contents reference).
    const QString pdfText = QString::fromLatin1(pdf.constData(), static_cast<int>(pdf.size()));
    const QRegularExpressionMatch contentsMatch =
        QRegularExpression(QStringLiteral("/Contents\\s+(\\d+)\\s+0\\s+R")).match(pdfText);
    const QRegularExpressionMatch resourcesMatch =
        QRegularExpression(QStringLiteral("/Resources\\s+(\\d+)\\s+0\\s+R")).match(pdfText);
    if (!contentsMatch.hasMatch() || (!registry.isEmpty() && !resourcesMatch.hasMatch()))
    {
        return pdf;
    }
    const int contentsObjNum = contentsMatch.captured(1).toInt();
    const int resourcesObjNum = resourcesMatch.hasMatch() ? resourcesMatch.captured(1).toInt() : -1;

    // --- Content stream: locate, decompress, rewrite marker fills as pattern references ---
    const int contentsBodyStart = findObjectBody(pdf, contentsObjNum);
    if (contentsBodyStart < 0)
    {
        return pdf;
    }
    const int contentsObjEnd = static_cast<int>(pdf.indexOf("endobj", contentsBodyStart));
    if (contentsObjEnd < 0)
    {
        return pdf;
    }
    const QByteArray contentsDict = pdf.mid(contentsBodyStart, contentsObjEnd - contentsBodyStart);
    const bool compressed = contentsDict.contains("/FlateDecode");
    const int streamKw = static_cast<int>(contentsDict.indexOf("stream"));
    if (streamKw < 0)
    {
        return pdf;
    }
    int dataStart = contentsBodyStart + streamKw + 6; // past "stream"
    if (pdf.mid(dataStart, 2) == "\r\n")
        dataStart += 2;
    else if (dataStart < pdf.size() && pdf.at(dataStart) == '\n')
        dataStart += 1;
    const int dataEnd = static_cast<int>(pdf.indexOf("endstream", dataStart));
    if (dataEnd < 0)
    {
        return pdf;
    }
    const QByteArray rawStream = pdf.mid(dataStart, dataEnd - dataStart);
    const QByteArray streamText = compressed ? zlibInflate(rawStream) : rawStream;
    if (streamText.isEmpty() && !rawStream.isEmpty())
    {
        return pdf;
    }

    // --- Resources dict: no stream, safe to edit as plain text -- only needed when adding pattern
    // objects; a grayscale-only pass with no hatch patterns doesn't touch it. ---
    QByteArray resDict;
    if (!registry.isEmpty())
    {
        const int resBodyStart = findObjectBody(pdf, resourcesObjNum);
        if (resBodyStart < 0)
        {
            return pdf;
        }
        const int resObjEnd = static_cast<int>(pdf.indexOf("endobj", resBodyStart));
        if (resObjEnd < 0)
        {
            return pdf;
        }
        resDict = pdf.mid(resBodyStart, resObjEnd - resBodyStart);
    }

    const QRegularExpressionMatch sizeMatch = QRegularExpression(QStringLiteral("/Size\\s+(\\d+)")).match(pdfText);
    const QRegularExpressionMatch rootMatch =
        QRegularExpression(QStringLiteral("/Root\\s+(\\d+\\s+0\\s+R)")).match(pdfText);
    if (!sizeMatch.hasMatch() || !rootMatch.hasMatch())
    {
        return pdf;
    }
    int nextObjNum = sizeMatch.captured(1).toInt();
    const QByteArray rootRef = rootMatch.captured(1).toLatin1();

    QRegularExpressionMatchIterator startxrefIt =
        QRegularExpression(QStringLiteral("startxref\\s+(\\d+)")).globalMatch(pdfText);
    QRegularExpressionMatch lastStartxref;
    while (startxrefIt.hasNext())
        lastStartxref = startxrefIt.next();
    if (!lastStartxref.hasMatch())
    {
        return pdf;
    }
    const QByteArray prevXrefOffset = lastStartxref.captured(1).toLatin1();

    // One pattern object per registry entry (color, style, spacing) -- see patternTileContentPdf()
    // for why this isn't deduplicated down to one shared object per Dense level.
    QVector<int> patternObjNum(registry.size());
    for (int i = 0; i < registry.size(); ++i)
        patternObjNum[i] = nextObjNum++;

    // --- Rewrite the content stream: marker "R G B scn" -> pattern colorspace + this entry's own
    // pattern name, restoring the plain colorspace right after that fill's own fill operator so
    // later, non-hatched fills (a bare color triple, no pattern name) aren't left pointing at
    // /Pattern. ---
    const QString text = QString::fromLatin1(streamText.constData(), static_cast<int>(streamText.size()));
    QRegularExpression scnRe(QStringLiteral("([0-9.eE+-]+)\\s+([0-9.eE+-]+)\\s+([0-9.eE+-]+)\\s+scn"));
    QRegularExpression fillOpRe(QStringLiteral("(?<=\\s)(f\\*|f|B\\*|B|b\\*|b)(?=\\s)"));

    // A pattern's own Matrix always maps pattern space into the page's *default* (pre-content-stream)
    // coordinate system, ignoring any `cm` the content stream itself applies -- but Qt emits one fixed
    // `cm` (a y-flip + offset, to match its own top-down coordinate convention) right at the top of
    // every page, and all our fill geometry is expressed *after* that `cm`. Left uncorrected, the
    // pattern grid and the shapes it fills live in different coordinate frames, which is exactly what
    // produced the moire/seam artifacts reported after the density/size fix: fold that same `cm` into
    // each pattern's Matrix so pattern space and content space agree again.
    double cmA = 1, cmB = 0, cmC = 0, cmD = 1, cmE = 0, cmF = 0;
    const QRegularExpressionMatch cmMatch =
        QRegularExpression(QStringLiteral("([0-9.eE+-]+)\\s+([0-9.eE+-]+)\\s+([0-9.eE+-]+)\\s+([0-9.eE+-]+)\\s+"
                                          "([0-9.eE+-]+)\\s+([0-9.eE+-]+)\\s+cm"))
            .match(text);
    if (cmMatch.hasMatch())
    {
        cmA = cmMatch.captured(1).toDouble();
        cmB = cmMatch.captured(2).toDouble();
        cmC = cmMatch.captured(3).toDouble();
        cmD = cmMatch.captured(4).toDouble();
        cmE = cmMatch.captured(5).toDouble();
        cmF = cmMatch.captured(6).toDouble();
    }

    QString out;
    out.reserve(text.size() + 4096);
    int pos = 0;
    while (true)
    {
        const QRegularExpressionMatch m = scnRe.match(text, pos);
        if (!m.hasMatch())
        {
            out += text.mid(pos);
            break;
        }
        out += text.mid(pos, m.capturedStart() - pos);

        const double rF = m.captured(1).toDouble();
        const double gF = m.captured(2).toDouble();
        const double bF = m.captured(3).toDouble();
        const int gi = qRound(gF * 255.0);
        const int bi = qRound(bF * 255.0);
        const int index = (gi << 8) | bi;
        const bool isMarker =
            qAbs(rF * 255.0 - 1.0) < 0.01 && gi >= 0 && gi <= 255 && bi >= 0 && bi <= 255 && index < registry.size();

        if (!isMarker)
        {
            out += m.captured(0);
            pos = static_cast<int>(m.capturedEnd());
            continue;
        }

        // Colored (PaintType 1) pattern: no color operands on scn, just its own pattern name -- the
        // real color is already baked into pattern object index's own content stream.
        out += QStringLiteral("/Pattern cs\n/Pat%1 scn").arg(index);

        const QRegularExpressionMatch fillMatch = fillOpRe.match(text, m.capturedEnd());
        if (!fillMatch.hasMatch())
        {
            return pdf;
        }
        out += text.mid(m.capturedEnd(), fillMatch.capturedEnd() - m.capturedEnd());
        out += QStringLiteral("\n/CSp cs");
        pos = static_cast<int>(fillMatch.capturedEnd());
    }

    // Desaturate every remaining real color operand (fill and stroke) in place, keeping the same
    // DeviceRGB colorspace/operators (R=G=B=luminance) rather than switching to /CSpg -- sidesteps
    // any assumption about how often Qt actually re-emits cs/CS. Pattern references inserted just
    // above have no numeric operands, so they're untouched; each pattern's own tile content is
    // grayscale-converted separately, in patternTileContentPdf() below.
    if (grayscale)
    {
        QRegularExpression colorOpRe(
            QStringLiteral("([0-9.eE+-]+)\\s+([0-9.eE+-]+)\\s+([0-9.eE+-]+)\\s+(rg|RG|scn|SCN)"));
        QString grayOut;
        grayOut.reserve(out.size());
        int gpos = 0;
        while (true)
        {
            const QRegularExpressionMatch gm = colorOpRe.match(out, gpos);
            if (!gm.hasMatch())
            {
                grayOut += out.mid(gpos);
                break;
            }
            grayOut += out.mid(gpos, gm.capturedStart() - gpos);
            const double gray = 0.299 * gm.captured(1).toDouble() + 0.587 * gm.captured(2).toDouble() +
                                0.114 * gm.captured(3).toDouble();
            const QString grayStr = QString::number(gray, 'f', 6);
            grayOut += grayStr + QStringLiteral(" ") + grayStr + QStringLiteral(" ") + grayStr + QStringLiteral(" ") +
                       gm.captured(4);
            gpos = static_cast<int>(gm.capturedEnd());
        }
        out = grayOut;
    }

    const QByteArray newStreamRaw = out.toLatin1();
    const QByteArray newStreamData = compressed ? zlibDeflate(newStreamRaw) : newStreamRaw;
    if (newStreamData.isEmpty() && !newStreamRaw.isEmpty())
    {
        return pdf;
    }

    // --- Serialize the appended incremental-update objects, tracking byte offsets for the new xref ---
    QByteArray append;
    QVector<QPair<int, int>> newObjOffsets; // (objNum, offsetFromFileStart)
    const int baseOffset = static_cast<int>(pdf.size());

    auto appendObj = [&](int objNum, const QByteArray &body) {
        newObjOffsets.append({objNum, baseOffset + static_cast<int>(append.size())});
        append += QByteArray::number(objNum) + " 0 obj\n" + body + "\nendobj\n";
    };

    const QByteArray newContentsBody = "<<\n/Length " + QByteArray::number(newStreamData.size()) +
                                       (compressed ? QByteArray("\n/Filter /FlateDecode") : QByteArray()) +
                                       "\n>>\nstream\n" + newStreamData + "\nendstream";
    appendObj(contentsObjNum, newContentsBody);

    if (!registry.isEmpty())
    {
        // --- Resources: extend (or create) the /Pattern subdictionary with our new entries ---
        QByteArray patternEntries;
        for (int i = 0; i < registry.size(); ++i)
            patternEntries += "/Pat" + QByteArray::number(i) + " " + QByteArray::number(patternObjNum[i]) + " 0 R\n";

        QByteArray newResDict = resDict;
        const int patDictPos = static_cast<int>(newResDict.indexOf("/Pattern"));
        if (patDictPos >= 0)
        {
            const int openPos = static_cast<int>(newResDict.indexOf("<<", patDictPos));
            if (openPos < 0)
            {
                return pdf;
            }
            newResDict.insert(openPos + 2, patternEntries);
        }
        else
        {
            const int insertPos = static_cast<int>(newResDict.lastIndexOf(">>"));
            if (insertPos < 0)
            {
                return pdf;
            }
            newResDict.insert(insertPos, "/Pattern <<\n" + patternEntries + ">>\n");
        }
        appendObj(resourcesObjNum, newResDict);
    }

    for (int i = 0; i < registry.size(); ++i)
    {
        const PatternEntry &e = registry[i];
        const QByteArray tileContent = patternTileContentPdf(e.style - Qt::Dense1Pattern, e.color, grayscale);
        const double s = e.spacing / 8.0;
        // Scale (tile pitch) composed with the page's own leading cm (see above), not a bare scale --
        // pattern space maps straight into the page's default space, so the page's own y-flip/offset
        // has to be baked in here to land in the same frame as the (post-cm) fill geometry.
        const QByteArray matrix = QByteArray::number(s * cmA, 'f', 6) + " " + QByteArray::number(s * cmB, 'f', 6) +
                                  " " + QByteArray::number(s * cmC, 'f', 6) + " " +
                                  QByteArray::number(s * cmD, 'f', 6) + " " + QByteArray::number(cmE, 'f', 6) + " " +
                                  QByteArray::number(cmF, 'f', 6);
        const QByteArray patBody = "<<\n/Type /Pattern\n/PatternType 1\n/PaintType 1\n/TilingType 1\n"
                                   "/BBox [0 0 8 8]\n/XStep 8\n/YStep 8\n/Resources <<\n>>\n/Matrix [" +
                                   matrix + "]\n/Length " + QByteArray::number(tileContent.size()) + "\n>>\nstream\n" +
                                   tileContent + "endstream";
        appendObj(patternObjNum[i], patBody);
    }

    // --- New xref section (classic table, entries for exactly the objects we appended) + trailer,
    // chained to the original via /Prev -- a standard PDF incremental update. ---
    QVector<QPair<int, int>> sortedOffsets = newObjOffsets;
    std::sort(sortedOffsets.begin(), sortedOffsets.end(),
              [](const QPair<int, int> &a, const QPair<int, int> &b) { return a.first < b.first; });

    const int xrefOffset = baseOffset + static_cast<int>(append.size());
    QByteArray xref = "xref\n";
    int i = 0;
    while (i < sortedOffsets.size())
    {
        int j = i;
        while (j + 1 < sortedOffsets.size() && sortedOffsets[j + 1].first == sortedOffsets[j].first + 1)
            ++j;
        xref += QByteArray::number(sortedOffsets[i].first) + " " + QByteArray::number(j - i + 1) + "\n";
        for (int k = i; k <= j; ++k)
            xref += QString(QStringLiteral("%1")).arg(sortedOffsets[k].second, 10, 10, QChar('0')).toLatin1() +
                    " 00000 n \n";
        i = j + 1;
    }

    const QByteArray trailer = "trailer\n<<\n/Size " + QByteArray::number(nextObjNum) + "\n/Root " + rootRef +
                               "\n/Prev " + prevXrefOffset + "\n>>\nstartxref\n" + QByteArray::number(xrefOffset) +
                               "\n%%EOF\n";

    return pdf + append + xref + trailer;
}

void fillPath(QPainter *p, const QPainterPath &path, const QColor &color, Qt::BrushStyle style, double spacing)
{
    if (spacing <= 0.0)
        spacing = nativeTilePitch(p);

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
            // it to a compact native <pattern> after generation.
            const int index = registerPattern(color, style, spacing);
            p->fillPath(boundedPath, QBrush(markerColorForIndex(index)));
            p->restore();
            return;
        }
        if (dynamic_cast<QPdfWriter *>(p->device()) != nullptr)
        {
            // Same marker-and-resolve trick as the SVG branch above: fill with a reserved marker
            // color now, then applyPatternRegistryPdf() rewrites the finished PDF, replacing each
            // marker fill with a real PDF colored tiling-pattern reference (/Pattern cs + scn).
            // Requires the writer's own resolution pinned to 72 (see Graph::exportPdfNative()), or
            // the pattern's Matrix ends up scaled wrong.
            const int index = registerPattern(color, style, spacing);
            // Alpha isn't carried by scn's RGB operands -- it's a separate PDF graphics-state
            // property (an ExtGState's /ca) that Qt's PDF engine emits on its own based on the
            // brush's alpha and that post-processing never touches. Give the marker the real alpha
            // so that ExtGState (unlike the RGB, which stays fake until resolved) is already correct.
            QColor marker = markerColorForIndex(index);
            marker.setAlpha(color.alpha());
            p->fillPath(boundedPath, QBrush(marker));
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
