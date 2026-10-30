/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: EDF (ESRF Data Format) parser
 ******************************************************************************/

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

#include <QByteArray>
#include <QFile>
#include <QMap>
#include <QPair>
#include <QtEndian>

#include "parser-edf.h"

namespace
{
struct EdfHeaderInfo
{
    int W = 0;
    int H = 0;
    int bytesPerPixel = 4;
    bool isFloat = false;
    bool isDouble = false;
    bool isSigned = false;
    bool bigEndian = false;
    qint64 dataOffset = 0;
};

//+++ read the '{' ... '}' header block into an ordered key/value list, locating the data offset
bool readEdfHeaderPairs(QFile &file, EdfHeaderInfo &info, QList<QPair<QString, QString>> &pairs)
{
    QByteArray rawHeader;
    char c;
    bool started = false;
    while (file.getChar(&c))
    {
        if (!started)
        {
            if (c == '{')
            {
                started = true;
                rawHeader.append(c);
            }
            continue;
        }
        rawHeader.append(c);
        if (c == '}')
            break;
    }
    if (!started)
        return false;

    info.dataOffset = ((file.pos() + 511) / 512) * 512;

    for (const QByteArray &token : rawHeader.split(';'))
    {
        qsizetype eq = token.indexOf('=');
        if (eq < 0)
            continue;
        QString key = QString::fromLatin1(token.left(eq)).simplified();
        QString val = QString::fromLatin1(token.mid(eq + 1)).simplified();
        // '{' sits on its own line before the first real entry, and '}' can trail the last one —
        // splitting by ';' (not by line) glues either onto whichever token they're adjacent to, so
        // both key and value need stripping, not just the value.
        key = key.remove('{').remove('}').trimmed();
        val = val.remove('{').remove('}').trimmed();
        if (key.isEmpty())
            continue;

        pairs << qMakePair(key, val);

        if (key.compare("Dim_1", Qt::CaseInsensitive) == 0)
            info.W = val.toInt();
        else if (key.compare("Dim_2", Qt::CaseInsensitive) == 0)
            info.H = val.toInt();
        else if (key.compare("DataType", Qt::CaseInsensitive) == 0)
        {
            if (val.compare("UnsignedShort", Qt::CaseInsensitive) == 0)
                info.bytesPerPixel = 2;
            else if (val.compare("UnsignedInteger", Qt::CaseInsensitive) == 0 ||
                     val.compare("UnsignedLong", Qt::CaseInsensitive) == 0)
                info.bytesPerPixel = 4;
            else if (val.compare("Float", Qt::CaseInsensitive) == 0 ||
                     val.compare("FloatValue", Qt::CaseInsensitive) == 0)
            {
                info.bytesPerPixel = 4;
                info.isFloat = true;
            }
            else if (val.compare("Double", Qt::CaseInsensitive) == 0)
            {
                info.bytesPerPixel = 8;
                info.isDouble = true;
            }
            else if (val.compare("Integer", Qt::CaseInsensitive) == 0 ||
                     val.compare("Long", Qt::CaseInsensitive) == 0 ||
                     val.compare("SignedInteger", Qt::CaseInsensitive) == 0 ||
                     val.compare("SignedLong", Qt::CaseInsensitive) == 0)
            {
                info.bytesPerPixel = 4;
                info.isSigned = true;
            }
            else if (val.compare("Short", Qt::CaseInsensitive) == 0)
            {
                info.bytesPerPixel = 2;
                info.isSigned = true;
            }
            else if (val.compare("UnsignedByte", Qt::CaseInsensitive) == 0 ||
                     val.compare("Byte", Qt::CaseInsensitive) == 0)
                info.bytesPerPixel = 1;
            else if (val.compare("SignedByte", Qt::CaseInsensitive) == 0)
            {
                info.bytesPerPixel = 1;
                info.isSigned = true;
            }
        }
        else if (key.compare("ByteOrder", Qt::CaseInsensitive) == 0)
            info.bigEndian = val.contains("High", Qt::CaseInsensitive);
    }

    return info.W > 0 && info.H > 0;
}

//+++ read just the geometry/encoding summary, discarding the key/value list
bool readEdfHeader(QFile &file, EdfHeaderInfo &info)
{
    QList<QPair<QString, QString>> pairs;
    return readEdfHeaderPairs(file, info, pairs);
}

//+++ decode one raw pixel to a double
//! Applies the masked/negative -> 0 convention every EDF read in this codebase uses (high bit set
//! on an unsigned 32-bit pixel marks an ESRF mask sentinel).
double decodePixel(const char *p, const EdfHeaderInfo &info)
{
    if (info.isDouble)
    {
        quint64 bits;
        memcpy(&bits, p, 8);
        bits = info.bigEndian ? qFromBigEndian(bits) : qFromLittleEndian(bits);
        double v;
        memcpy(&v, &bits, 8);
        return v < 0 ? 0.0 : v;
    }
    if (info.isFloat)
    {
        uint32_t bits;
        memcpy(&bits, p, 4);
        bits = info.bigEndian ? qFromBigEndian(bits) : qFromLittleEndian(bits);
        float v;
        memcpy(&v, &bits, 4);
        return v < 0.0f ? 0.0 : double(v);
    }
    if (info.bytesPerPixel == 4 && info.isSigned)
    {
        uint32_t bits;
        memcpy(&bits, p, 4);
        bits = info.bigEndian ? qFromBigEndian(bits) : qFromLittleEndian(bits);
        int32_t v;
        memcpy(&v, &bits, 4);
        return v < 0 ? 0.0 : double(v);
    }
    if (info.bytesPerPixel == 4)
    {
        uint32_t bits;
        memcpy(&bits, p, 4);
        bits = info.bigEndian ? qFromBigEndian(bits) : qFromLittleEndian(bits);
        return (bits & 0x80000000u) ? 0.0 : double(bits); // high bit set → masked pixel sentinel
    }
    if (info.bytesPerPixel == 2 && info.isSigned)
    {
        uint16_t bits;
        memcpy(&bits, p, 2);
        bits = info.bigEndian ? qFromBigEndian(bits) : qFromLittleEndian(bits);
        int16_t v;
        memcpy(&v, &bits, 2);
        return v < 0 ? 0.0 : double(v);
    }
    if (info.bytesPerPixel == 2)
    {
        uint16_t bits;
        memcpy(&bits, p, 2);
        return double(info.bigEndian ? qFromBigEndian(bits) : qFromLittleEndian(bits));
    }
    auto v = (uint8_t)*p;
    return info.isSigned ? (((int8_t)v < 0) ? 0.0 : double((int8_t)v)) : double(v);
}

//+++ encode one summed pixel back into its original on-disk type/endianness
//! Negative sums are clamped to 0, matching decodePixel()'s own masked/negative convention.
void encodePixel(double value, const EdfHeaderInfo &info, char *out)
{
    if (value < 0)
        value = 0;

    if (info.isDouble)
    {
        double v = value;
        quint64 bits;
        memcpy(&bits, &v, 8);
        bits = info.bigEndian ? qToBigEndian(bits) : qToLittleEndian(bits);
        memcpy(out, &bits, 8);
    }
    else if (info.isFloat)
    {
        auto v = float(value);
        uint32_t bits;
        memcpy(&bits, &v, 4);
        bits = info.bigEndian ? qToBigEndian(bits) : qToLittleEndian(bits);
        memcpy(out, &bits, 4);
    }
    else if (info.bytesPerPixel == 4)
    {
        auto bits = uint32_t(qint64(std::llround(value)) & 0xffffffffLL);
        bits = info.bigEndian ? qToBigEndian(bits) : qToLittleEndian(bits);
        memcpy(out, &bits, 4);
    }
    else if (info.bytesPerPixel == 2)
    {
        auto bits = uint16_t(qint64(std::llround(value)) & 0xffffLL);
        bits = info.bigEndian ? qToBigEndian(bits) : qToLittleEndian(bits);
        memcpy(out, &bits, 2);
    }
    else
        *out = char(uint8_t(qint64(std::llround(value)) & 0xffLL));
}
} // namespace

//+++ read EDF (ESRF Data Format) into gsl_matrix
bool ParserEDF::readMatrixEdf(const QString &fileName, int DD, gsl_matrix *&matrix, int offsetX, int offsetY)
{
    if (!matrix)
        return false;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    EdfHeaderInfo info;
    if (!readEdfHeader(file, info))
        return false;
    if (!file.seek(info.dataOffset))
        return false;

    QByteArray data = file.readAll();
    if (data.size() < qint64(info.W) * qint64(info.H) * qint64(info.bytesPerPixel))
        return false;

    // ROI logic (same as readMatrixTiff)
    int cx = info.W / 2 - offsetX;
    int cy = info.H / 2 - offsetY;
    int roiStartX = cx - DD / 2;
    int roiStartY = cy - DD / 2;
    int copyStartX = std::max(roiStartX, 0);
    int copyStartY = std::max(roiStartY, 0);
    int copyEndX = std::min(roiStartX + DD, info.W);
    int copyEndY = std::min(roiStartY + DD, info.H);

    gsl_matrix_set_zero(matrix);

    const char *raw = data.constData();

    for (int y = copyStartY; y < copyEndY; ++y)
    {
        for (int x = copyStartX; x < copyEndX; ++x)
        {
            int outX = x - roiStartX;
            int outY = y - roiStartY;
            if (outX < 0 || outX >= DD || outY < 0 || outY >= DD)
                continue;

            gsl_matrix_set(matrix, outY, outX, decodePixel(raw + (qint64(y) * info.W + x) * info.bytesPerPixel, info));
        }
    }

    return true;
}

//+++ accumulate (sum) the full detector image across several EDF files, in place
bool ParserEDF::accumulateDatasets(const QString &fileName, const QStringList &files)
{
    if (fileName.isEmpty() || files.isEmpty() || files[0].isEmpty())
        return false;

    // fileName is already a copy of files[0] (same calling convention as
    // ParserHDF5::accumulateDatasets()) — read its own header for both the dimensions every file
    // must match and the encoding the summed data must be written back in; the header text itself
    // is left untouched.
    QFile outFile(fileName);
    if (!outFile.open(QIODevice::ReadOnly))
        return false;
    EdfHeaderInfo info;
    bool headerOk = readEdfHeader(outFile, info);
    outFile.close();
    if (!headerOk)
        return false;

    std::vector<double> sum(size_t(info.W) * size_t(info.H), 0.0);

    for (const QString &f : files)
    {
        if (f.isEmpty())
            continue;

        QFile in(f);
        if (!in.open(QIODevice::ReadOnly))
            return false;
        EdfHeaderInfo hi;
        if (!readEdfHeader(in, hi) || hi.W != info.W || hi.H != info.H || !in.seek(hi.dataOffset))
            return false;

        QByteArray data = in.readAll();
        in.close();
        if (data.size() < qint64(hi.W) * qint64(hi.H) * qint64(hi.bytesPerPixel))
            return false;

        const char *raw = data.constData();
        for (int y = 0; y < hi.H; ++y)
            for (int x = 0; x < hi.W; ++x)
                sum[size_t(y) * info.W + x] += decodePixel(raw + (qint64(y) * hi.W + x) * hi.bytesPerPixel, hi);
    }

    QByteArray outData(qint64(info.W) * info.H * info.bytesPerPixel, Qt::Uninitialized);
    char *outRaw = outData.data();
    for (int y = 0; y < info.H; ++y)
        for (int x = 0; x < info.W; ++x)
            encodePixel(sum[size_t(y) * info.W + x], info, outRaw + (qint64(y) * info.W + x) * info.bytesPerPixel);

    QFile rw(fileName);
    if (!rw.open(QIODevice::ReadWrite) || !rw.seek(info.dataOffset))
        return false;
    bool ok = rw.write(outData) == outData.size();
    rw.close();
    return ok;
}

//+++ replace the '{ ... }' header block, moving the binary data block if the padded size changes
bool ParserEDF::replaceHeader(const QString &fileName, const QStringList &headerLines)
{
    if (fileName.isEmpty())
        return false;

    QFile probe(fileName);
    if (!probe.open(QIODevice::ReadOnly))
        return false;
    EdfHeaderInfo info;
    QList<QPair<QString, QString>> originalPairs;
    bool headerOk = readEdfHeaderPairs(probe, info, originalPairs);
    probe.close();
    if (!headerOk)
        return false;

    int firstLine = 0;
    int lastLine = static_cast<int>(headerLines.size()) - 1;
    if (firstLine <= lastLine && headerLines[firstLine].trimmed().startsWith('{'))
        ++firstLine;
    if (lastLine >= firstLine && headerLines[lastLine].trimmed().startsWith('}'))
        --lastLine;

    QMap<QString, QPair<QString, QString>> newValues; // lowerKey -> (originalCaseKey, value)
    for (int i = firstLine; i <= lastLine; ++i)
    {
        for (const QString &token : headerLines[i].split(';'))
        {
            qsizetype eq = token.indexOf('=');
            if (eq < 0)
                continue;
            QString key = token.left(eq).simplified();
            QString val = token.mid(eq + 1).simplified();
            val.remove('{').remove('}');
            if (!key.isEmpty())
                newValues[key.toLower()] = qMakePair(key, val);
        }
    }

    QList<QPair<QString, QString>> merged = originalPairs;
    for (auto &kv : merged)
    {
        auto it = newValues.constFind(kv.first.toLower());
        if (it != newValues.constEnd())
            kv.second = it->second;
    }

    // the header block keeps at least its size (the data where it was; EDF_HeaderSize, when there, stays true): a
    // larger one is the next multiple of 512 bytes, and its EDF_HeaderSize says so
    auto block = [&merged]() {
        QByteArray b = "{\n";
        for (const auto &kv : merged)
            b += (kv.first + " = " + kv.second + " ;\n").toLatin1();
        return b;
    };
    QByteArray newHeaderBlock = block();
    qint64 padded = std::max(info.dataOffset, ((qint64(newHeaderBlock.size()) + 2 + 511) / 512) * 512);
    for (auto &kv : merged)
        if (kv.first.compare("EDF_HeaderSize", Qt::CaseInsensitive) == 0 && kv.second.toLongLong() != padded)
        {
            kv.second = QString::number(padded);
            newHeaderBlock = block();
            padded = std::max(padded, ((qint64(newHeaderBlock.size()) + 2 + 511) / 512) * 512);
            kv.second = QString::number(padded);
            newHeaderBlock = block();
        }
    newHeaderBlock += QByteArray(padded - newHeaderBlock.size() - 2, ' ');
    newHeaderBlock += "}\n";

    if (newHeaderBlock.size() == info.dataOffset)
    {
        // fast path: identical padded size, data offset unchanged — overwrite the header region only
        QFile rw(fileName);
        if (!rw.open(QIODevice::ReadWrite))
            return false;
        bool wrote = rw.write(newHeaderBlock) == newHeaderBlock.size();
        rw.close();
        return wrote;
    }

    QFile rd(fileName);
    if (!rd.open(QIODevice::ReadOnly) || !rd.seek(info.dataOffset))
        return false;
    QByteArray data = rd.readAll();
    rd.close();

    QFile wr(fileName);
    if (!wr.open(QIODevice::WriteOnly))
        return false;

    qint64 wroteHeader = wr.write(newHeaderBlock);
    qint64 wroteData = wr.write(data);
    bool wrote = (wroteHeader == newHeaderBlock.size()) && (wroteData == data.size());
    wr.close();
    return wrote;
}

//+++ copy files[0] to fileName, then sum every file in `files` into it
bool ParserEDF::accumulateFiles(const QString &fileName, const QStringList &files)
{
    if (fileName.isEmpty() || files.isEmpty() || files[0].isEmpty())
    {
        std::cerr << "! Check file-names" << std::endl;
        return false;
    }

    // QFile::copy() fails outright if the destination already exists — remove() must actually
    // succeed first, or copy() (and everything below) silently no-ops, leaving whatever stale file
    // was already at this path completely untouched.
    if (QFile::exists(fileName) && !QFile::remove(fileName))
    {
        std::cerr << "! Failed to remove existing file " << fileName.toStdString() << " before accumulating EDF files."
                  << std::endl;
        return false;
    }

    if (!QFile::copy(files[0], fileName))
    {
        std::cerr << "Failed to copy " << files[0].toStdString() << " to " << fileName.toStdString() << std::endl;
        return false;
    }

    if (!accumulateDatasets(fileName, files))
    {
        std::cerr << "! Failed to accumulate EDF detector images" << std::endl;
        return false;
    }
    return true;
}
