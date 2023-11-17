/***************************************************************************
    File                 : parser-hdf5.h
    Project              : QtiSAS
    --------------------------------------------------------------------
    Copyright            : (C) 2023 by Vitaliy Pipich
    Email (use @ for *)  : v.pipich*gmail.com
    Description          : HDF5 parser

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef PARSER_HDF5_H
#define PARSER_HDF5_H

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
