/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: HDF5 parser
 ******************************************************************************/

#ifndef PARSERHDF5_H
#define PARSERHDF5_H

#include <QString>

#include <gsl/gsl_matrix.h>

class ParserHDF5
{
  public:
    static QString readEntry(const QString &file, const QString &code, QString &action);
    static bool readMatrix(const QString &file, QString code, int numberFrames, int dimX, int DimY,
                           gsl_matrix *&matrix);
};

#endif
