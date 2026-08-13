/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: Image Matrix Parsers
 ******************************************************************************/

#pragma once

#include <QString>
#include <QStringList>

#include <gsl/gsl_matrix.h>

namespace ParserIMAGE
{
    bool readMatrixTiff(const QString &fn, int DD, gsl_matrix* &matrix, int offsetX = 0, int offsetY = 0);
    bool readMatrixImage(const QString &fn, int DD, gsl_matrix* &matrix, int offsetX = 0, int offsetY = 0);
    bool readMatrixBinary(const QString &fn, int DD, gsl_matrix* &matrix, int offsetX = 0, int offsetY = 0);
    bool readMatrixBinaryGZipped(const QString &fn, int DD, gsl_matrix* &matrix, int offsetX = 0, int offsetY = 0);
}