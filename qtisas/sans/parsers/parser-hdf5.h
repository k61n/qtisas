/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: HDF5 parser
 ******************************************************************************/

#ifndef PARSER_HDF5_H
#define PARSER_HDF5_H

#include "compat.h"

#include <QString>
#include <QStringList>

#include <iostream>

#include <H5Cpp.h>
using namespace H5;
using namespace std;

#include <gsl/gsl_matrix.h>

class ParserHDF5
{
  public:
    static QString readEntry(const QString &file, const QString &code, QString &action);
    static bool readMatrix(const QString &file, QString code, int numberFrames, int dimX, int DimY,
                           gsl_matrix *&matrix);
};
#endif
