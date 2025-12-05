/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: ASCII parser(s)
 ******************************************************************************/

#pragma once

#include <QString>
#include <QStringList>
#include <QTextStream>

#include <gsl/gsl_matrix.h>

namespace ParserASCII
{
QString readEntryFlexy(const QString &file, QString &code, int &startingPosition, int maxLinesNumberInHeader);
QString readEntry(const QString &file, QString line, const QStringList &lst = QStringList());
QString stringParser(QString initText, QString &action);
QString findNumberInString(QString line, int digitNumber, QString &num);
QString findStringInString(const QString &initialString, int stringPosisionInString, const QString &separationSymbol,
                           QString &indexOfString);
void skipHeader(QTextStream &stream, int linesInHeader, bool flexiHeader, const QStringList &flexiStop);
bool readMatrixByName(const QString &fn, int DD, int pixelPerLine, gsl_matrix *&data, int linesInHeader,
                      bool flexiHeader, const QStringList &flexiStop);
bool readMatrixByNameOne(const QString &fn, int DD, gsl_matrix *&data, int linesInHeader, bool flexiHeader,
                         const QStringList &flexiStop);
bool readMatrixByNameGSL(const QString &fileName, gsl_matrix* &data);
bool readMatrixByNameGZipped(const QString &fn, int DD, int ROI, gsl_matrix* &matrix);
}
