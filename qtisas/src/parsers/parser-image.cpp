/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: Image Matrix Parser(s)
 ******************************************************************************/

#include <algorithm>
#include <cmath>
#include <gsl/gsl_matrix.h>
#include <memory>
#include <tiffio.h>
#include <zlib.h>

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QImageReader>
#include <QPixmap>

#include "parser-image.h"

//+++ read TIFF image into gsl_matrix  (FULL SCIENTIFIC SUPPORT)
bool ParserIMAGE::readMatrixTiff(const QString &fileName, int DD, gsl_matrix *&matrix, int offsetX, int offsetY)
{
    if (!matrix)
        return false;

    // RAII helper structs
    struct TiffCloser
    {
        void operator()(TIFF *t) const
        {
            if (t)
                TIFFClose(t);
        }
    };
    struct TiffBufferDeleter
    {
        void operator()(void *b) const
        {
            if (b)
                _TIFFfree(b);
        }
    };

    TIFFErrorHandler oldhandler = TIFFSetWarningHandler(nullptr);
    std::unique_ptr<TIFF, TiffCloser> tif(TIFFOpen(fileName.toLocal8Bit().constData(), "r"));
    TIFFSetWarningHandler(oldhandler);

    if (!tif)
        return false;

    uint32_t W, H;
    TIFFGetField(tif.get(), TIFFTAG_IMAGEWIDTH, &W);
    TIFFGetField(tif.get(), TIFFTAG_IMAGELENGTH, &H);

    uint16_t spp = 1;  // samples per pixel
    uint16_t bps = 16; // bits per sample
    uint16_t fmt = SAMPLEFORMAT_UINT;

    TIFFGetFieldDefaulted(tif.get(), TIFFTAG_SAMPLESPERPIXEL, &spp);
    TIFFGetFieldDefaulted(tif.get(), TIFFTAG_BITSPERSAMPLE, &bps);
    TIFFGetFieldDefaulted(tif.get(), TIFFTAG_SAMPLEFORMAT, &fmt);

    const tsize_t scanline = TIFFScanlineSize(tif.get());
    std::unique_ptr<void, TiffBufferDeleter> buf(_TIFFmalloc(scanline));
    if (!buf)
        return false; // Automatic cleanup

    // ---------------------------------------------------------
    // ROI LOGIC (unchanged)
    // ---------------------------------------------------------
    int cx = static_cast<int>(W) / 2 - offsetX;
    int cy = static_cast<int>(H) / 2 - offsetY;
    int roiStartX = cx - DD / 2;
    int roiStartY = cy - DD / 2;
    int roiEndX = roiStartX + DD;
    int roiEndY = roiStartY + DD;
    int copyStartX = std::max(roiStartX, 0);
    int copyStartY = std::max(roiStartY, 0);
    int copyEndX = std::min(roiEndX, (int)W);
    int copyEndY = std::min(roiEndY, (int)H);

    gsl_matrix_set_zero(matrix);

    // ---------------------------------------------------------
    // Pixel reading helpers
    // ---------------------------------------------------------
    auto read_pixel_uint16 = [](uint16_t *row, int x) -> double { return row[x]; };
    auto read_pixel_uint8 = [](const uint8_t *row, int x) -> double { return row[x] * 257; }; // scale 0–255 → 0–65535
    auto read_pixel_uint32 = [](const uint32_t *row, int x) -> double {
        uint32_t v = row[x];
        return (v > 65535u ? 65535u : v);
    };
    auto read_pixel_float32 = [](const float *row, int x) -> double {
        float v = row[x];
        if (v <= 0)
            return 0;
        if (v >= 1e6)
            v = 1e6;
        return (v / 1e6) * 65535.0; // scale scientific float
    };
    auto read_pixel_float64 = [](const double *row, int x) -> double {
        double v = row[x];
        if (v <= 0)
            return 0;
        if (v >= 1e6)
            v = 1e6;
        return (v / 1e6) * 65535.0;
    };
    auto read_pixel_rgb = [](const uint16_t *row, int x) -> double {
        // RGB 16-bit → grayscale
        uint16_t r = row[x * 3 + 0];
        uint16_t g = row[x * 3 + 1];
        uint16_t b = row[x * 3 + 2];
        return 0.2126 * r + 0.7152 * g + 0.0722 * b;
    };

    // ---------------------------------------------------------
    // MAIN COPY LOOP
    // ---------------------------------------------------------
    for (int y = copyStartY; y < copyEndY; ++y)
    {
        if (TIFFReadScanline(tif.get(), buf.get(), y, 0) < 0)
            continue;

        for (int x = copyStartX; x < copyEndX; ++x)
        {
            int outX = x - roiStartX;
            int outY = y - roiStartY;

            if (outX < 0 || outX >= DD || outY < 0 || outY >= DD)
                continue;

            double value = 0.0;

            switch (fmt)
            {
            case SAMPLEFORMAT_UINT:
                if (bps == 8)
                    value = read_pixel_uint8((uint8_t *)buf.get(), x * spp);
                else if (bps == 16)
                    value = read_pixel_uint16((uint16_t *)buf.get(), x * spp);
                else if (bps == 32)
                    value = read_pixel_uint32((uint32_t *)buf.get(), x * spp);
                break;
            case SAMPLEFORMAT_INT:
                if (bps == 16)
                {
                    auto *row = (int16_t *)buf.get();
                    value = std::max(0, (int)row[x]); // no negatives
                }
                else if (bps == 32)
                {
                    auto *row = (int32_t *)buf.get();
                    value = std::max(0, (int)row[x]);
                }
                break;
            case SAMPLEFORMAT_IEEEFP:
                if (bps == 32)
                    value = read_pixel_float32((float *)buf.get(), x);
                else if (bps == 64)
                    value = read_pixel_float64((double *)buf.get(), x);
                break;
            default:
                // unsupported format → leave value = 0
                break;
            }

            // RGB special case (3 or 4 channels)
            if (spp >= 3 && fmt == SAMPLEFORMAT_UINT && bps == 16)
                value = read_pixel_rgb((uint16_t *)buf.get(), x);

            // clamp
            if (value < 0)
                value = 0;
            if (value > 65535)
                value = 65535;

            gsl_matrix_set(matrix, outY, outX, value);
        }
    }

    return true;
}
//+++ read IMAGE (PNG, JPEG, BMP, etc.) into gsl_matrix  (BASIC SUPPORT)
bool ParserIMAGE::readMatrixImage(const QString &fileName, int DD, gsl_matrix *&matrix, int offsetX, int offsetY)
{
    if (!matrix)
        return false;

    // Check if QImageReader can read this file
    QImageReader reader(fileName);
    if (!reader.canRead())
        return readMatrixBinary(fileName, DD, matrix, offsetX, offsetY);

    QImage image = reader.read();
    if (image.isNull())
        return false;

    int W = image.width();
    int H = image.height();

    // -------------------------------------------
    // Compute ROI center with offset
    // -------------------------------------------
    int cx = W / 2 - offsetX;
    int cy = H / 2 - offsetY;

    int roiStartX = cx - DD / 2;
    int roiStartY = cy - DD / 2;
    int roiEndX = roiStartX + DD;
    int roiEndY = roiStartY + DD;

    int copyStartX = std::max(roiStartX, 0);
    int copyStartY = std::max(roiStartY, 0);
    int copyEndX = std::min(roiEndX, W);
    int copyEndY = std::min(roiEndY, H);

    gsl_matrix_set_zero(matrix);

    // -------------------------------------------
    // Copy pixels to matrix (RGB → grayscale)
    // -------------------------------------------
    for (int y = copyStartY; y < copyEndY; ++y)
    {
        for (int x = copyStartX; x < copyEndX; ++x)
        {
            int outX = x - roiStartX;
            int outY = y - roiStartY;

            if (outX < 0 || outX >= DD || outY < 0 || outY >= DD)
                continue;

            QRgb pixel = image.pixel(x, y);
            int gray = qGray(pixel);     // 0–255
            double value = gray * 257.0; // scale to 0–65535

            gsl_matrix_set(matrix, outY, outX, value);
        }
    }

    return true;
}
//+++ read BINARY image into gsl_matrix  (BASIC SUPPORT)
bool ParserIMAGE::readMatrixBinary(const QString &fn, int DD, gsl_matrix *&matrix, int offsetX, int offsetY)
{
    if (!matrix)
        return false;

    QFile file(fn);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    qint64 fileSize = file.size();

    qint64 expectedInt = qint64(DD) * qint64(DD) * qint64(sizeof(int32_t));
    qint64 expectedDbl = qint64(DD) * qint64(DD) * qint64(sizeof(double));

    QByteArray data = file.readAll();

    // ---------------------------------------------------------
    // AUTO-DETECT FORMAT
    // ---------------------------------------------------------
    enum Format
    {
        INT32,
        DOUBLE64
    } fmt;

    if (fileSize == expectedInt)
        fmt = INT32;
    else if (fileSize == expectedDbl)
        fmt = DOUBLE64;
    else
        return false;

    // ---------------------------------------------------------
    // IMAGE DIMENSIONS (Binary file is DD×DD)
    // ---------------------------------------------------------
    const int W = DD;
    const int H = DD;

    // ---------------------------------------------------------
    // ROI LOGIC (same as TIFF)
    // ---------------------------------------------------------
    int cx = W / 2 - offsetX;
    int cy = H / 2 - offsetY;

    int roiStartX = cx - DD / 2;
    int roiStartY = cy - DD / 2;
    int roiEndX = roiStartX + DD;
    int roiEndY = roiStartY + DD;

    int copyStartX = std::max(roiStartX, 0);
    int copyStartY = std::max(roiStartY, 0);
    int copyEndX = std::min(roiEndX, W);
    int copyEndY = std::min(roiEndY, H);

    gsl_matrix_set_zero(matrix);

    // ---------------------------------------------------------
    // DATA POINTERS
    // ---------------------------------------------------------
    auto *arrI = reinterpret_cast<const int32_t *>(data.constData());
    auto *arrD = reinterpret_cast<const double *>(data.constData());

    // ---------------------------------------------------------
    // MAIN COPY LOOP
    // ---------------------------------------------------------
    for (int y = copyStartY; y < copyEndY; ++y)
    {
        for (int x = copyStartX; x < copyEndX; ++x)
        {
            int outX = x - roiStartX;
            int outY = y - roiStartY;

            if (outX < 0 || outX >= DD || outY < 0 || outY >= DD)
                continue;

            double value = 0.0;

            if (fmt == INT32)
            {
                int32_t v = arrI[y * DD + x];
                value = (v < 0 ? 0.0 : double(v));
            }
            else // DOUBLE64
            {
                double v = arrD[y * DD + x];
                value = (v < 0 ? 0.0 : v);
            }

            gsl_matrix_set(matrix, outY, outX, value);
        }
    }

    return true;
}
//+++ read GZIPPED BINARY image into gsl_matrix  (BASIC SUPPORT)
bool ParserIMAGE::readMatrixBinaryGZipped(const QString &fn, int DD, gsl_matrix *&matrix, int offsetX, int offsetY)
{
    if (!matrix)
        return false;

    gzFile fd = gzopen(fn.toLocal8Bit().constData(), "rb");
    if (!fd)
        return false;

    std::vector<char> buf;
    const int chunkSize = 16384;
    char tmp[chunkSize];
    int bytesRead = 0;
    int ret;
    while ((ret = gzread(fd, tmp, chunkSize)) > 0)
    {
        buf.insert(buf.end(), tmp, tmp + ret);
        bytesRead += ret;
    }
    gzclose(fd);

    if (bytesRead == 0)
        return false;

    // --- detect type and shape ---
    int nInt = bytesRead / 4;
    int nDouble = bytesRead / 8;

    int origRows = 0, origCols = 0;
    bool useDouble = false;

    int sqInt = int(std::round(std::sqrt(nInt)));
    int sqDouble = int(std::round(std::sqrt(nDouble)));

    if (sqInt * sqInt == nInt)
    {
        useDouble = false;
        origRows = origCols = sqInt;
    }
    else if (sqDouble * sqDouble == nDouble)
    {
        useDouble = true;
        origRows = origCols = sqDouble;
    }
    else
    {
        // pick closest to square
        double rInt = std::abs(std::sqrt(nInt) - sqInt);
        double rDouble = std::abs(std::sqrt(nDouble) - sqDouble);
        useDouble = (rDouble < rInt);
        origRows = origCols = useDouble ? sqDouble : sqInt;
    }

    gsl_matrix_set_zero(matrix);

    // --- ROI / offset ---
    int cx = origCols / 2 - offsetX;
    int cy = origRows / 2 - offsetY;
    int roiStartX = cx - DD / 2;
    int roiStartY = cy - DD / 2;
    int roiEndX = roiStartX + DD;
    int roiEndY = roiStartY + DD;

    int copyStartX = std::max(roiStartX, 0);
    int copyStartY = std::max(roiStartY, 0);
    int copyEndX = std::min(roiEndX, origCols);
    int copyEndY = std::min(roiEndY, origRows);

    // --- fill matrix ---
    if (useDouble)
    {
        auto *p = (double *)buf.data();
        for (int y = copyStartY; y < copyEndY; ++y)
            for (int x = copyStartX; x < copyEndX; ++x)
                gsl_matrix_set(matrix, y - roiStartY, x - roiStartX, p[y * origCols + x]);
    }
    else
    {
        auto *p = (uint32_t *)buf.data();
        for (int y = copyStartY; y < copyEndY; ++y)
            for (int x = copyStartX; x < copyEndX; ++x)
                gsl_matrix_set(matrix, y - roiStartY, x - roiStartX, p[y * origCols + x]);
    }

    return true;
}
