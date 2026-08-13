/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: EDF (ESRF Data Format) parser
 ******************************************************************************/

#pragma once

#include <QString>
#include <QStringList>

#include <gsl/gsl_matrix.h>

namespace ParserEDF
{
    bool readMatrixEdf(const QString &fn, int DD, gsl_matrix* &matrix, int offsetX = 0, int offsetY = 0);
    bool accumulateDatasets(const QString &fileName, const QStringList &files);
    bool replaceHeader(const QString &fileName, const QStringList &headerLines);
    bool accumulateFiles(const QString &fileName, const QStringList &files);
}