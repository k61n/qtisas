/***************************************************************************
    File                 : parser-hdf5.cpp
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

#include "parser-hdf5.h"

QString ParserHDF5::readEntry(const QString &fileNameString, const QString &code, QString &action)
{
    if (code == "")
        return "";

    QString postAction = action;
    postAction = postAction.remove("[sum]").remove("[mean]");

    if (action != "[sum][sum]" && action != "[mean][mean]" && !(action.contains("[mean]") && action.contains("[sum]")))
    {
        if (action.left(1) == "[" && action.contains("]") && action.left(2) != "[s")
        {
            action = action.split("]")[0] + "]";
            postAction = postAction.remove(action);
        }
    }

    QString result = "";

    const H5std_string FILE_NAME(fileNameString.toLatin1().constData());
    const H5std_string GROUP_NAME(code.toLatin1().constData());

    H5File file(FILE_NAME, H5F_ACC_RDONLY);
    H5::Exception::dontPrint();

    DataSet dataset;
    try
    {
        dataset = file.openDataSet(GROUP_NAME);
    }
    catch (...)
    {
        // close the HDF5 file
        file.close();
        action = "";
        return "0";
    }

    DataSpace dataspace = dataset.getSpace();
    H5T_class_t type_class = dataset.getTypeClass();

    if (type_class == H5T_STRING)
    {
        StrType datatype = dataset.getStrType();
        std::string data;
        dataset.read(data, datatype, dataspace);
        result = QString::fromLocal8Bit(data.c_str());
    }
    else if (type_class == H5T_INTEGER)
    {
        int rank = dataspace.getSimpleExtentNdims();
        if (rank == 1)
        {
            hsize_t dim[1];
            int ndims = dataspace.getSimpleExtentDims(dim, nullptr);
            int int_value[dim[0]];
            dataset.read(int_value, PredType::NATIVE_INT, H5S_ALL, H5S_ALL);

            QString s = action;
            int index = s.remove("[").remove("]").toInt();
            if (index >= dim[0])
                index = 0;

            if (action == "[sum]")
            {
                int sum = 0;
                for (int i = 0; i < dim[0]; i++)
                    sum += int_value[i];
                result = QString::number(sum);
            }
            else if (action == "[mean]")
            {
                double sum = 0;
                for (int i = 0; i < dim[0]; i++)
                    sum += int_value[i];
                result = QString::number(sum / double(dim[0]));
            }
            else if (action == "[last]")
            {
                result = QString::number(int_value[dim[0] - 1]);
            }
            else if (action == "[first]")
            {
                result = QString::number(int_value[0]);
            }
            else
                result = QString::number(int_value[index]);
        }
        else if (rank == 2)
        {
            hsize_t dim[2];
            int ndims = dataspace.getSimpleExtentDims(dim, nullptr);
            int int_value[dim[0]][dim[1]];
            dataset.read(int_value, PredType::NATIVE_INT, H5S_ALL, H5S_ALL);

            QString s = action;
            s = s.remove("[").remove("]");
            QStringList lst = s.split(",", QString::SkipEmptyParts);
            int index0 = 0;
            int index1 = 0;

            if (lst.count() == 2)
            {
                index0 = lst[0].toInt();
                if (index0 >= dim[0])
                    index0 = 0;
                index1 = lst[1].toInt();
                if (index1 >= dim[1])
                    index1 = 0;
            }

            if (action == "[sum]" || action == "[sum][sum]" || (action.contains("[mean]") && action.contains("[sum]")))
            {
                int sum = 0;
                for (int i = 0; i < dim[0]; i++)
                    for (int j = 0; j < dim[1]; j++)
                        sum += int_value[i][j];
                result = QString::number(sum);
            }
            else if (action == "[mean]" || action == "[mean][mean]")
            {
                double sum = 0;
                for (int i = 0; i < dim[0]; i++)
                    for (int j = 0; j < dim[1]; j++)
                        sum += int_value[i][j];
                result = QString::number(sum / double(dim[0]) / double(dim[1]));
            }
            else if (action == "[first]")
                result = QString::number(int_value[0][0]);
            else if (action == "[last]")
                result = QString::number(int_value[dim[0] - 1][dim[1] - 1]);
            else
                result = QString::number(int_value[index0][index1]);
        }
        else if (rank == 3)
        {
            hsize_t dim[3];
            dataspace.getSimpleExtentDims(dim, nullptr);

            int int_value[dim[0]][dim[1]][dim[2]];
            dataset.read(int_value, PredType::NATIVE_INT, H5S_ALL, H5S_ALL);

            if (action == "[sum][sum]" || action == "[mean][mean]" ||
                (action.contains("[mean]") && action.contains("[sum]")))
            {
                if (action == "[sum][sum]")
                {
                    int sum = 0;
                    for (int k = 0; k < dim[0]; k++)
                        for (int i = 0; i < dim[1]; i++)
                            for (int j = 0; j < dim[2]; j++)
                                sum += int_value[k][i][j];
                    result = QString::number(sum);
                }
                else if (action == "[mean][mean]")
                {
                    double sum = 0;
                    for (int k = 0; k < dim[0]; k++)
                        for (int i = 0; i < dim[1]; i++)
                            for (int j = 0; j < dim[2]; j++)
                                sum += int_value[k][i][j];
                    result = QString::number(sum / double(dim[0]) / double(dim[1]) / double(dim[2]));
                }
                else if (action.contains("[mean]") && action.contains("[sum]"))
                {
                    double sum = 0;
                    for (int k = 0; k < dim[0]; k++)
                        for (int i = 0; i < dim[1]; i++)
                            for (int j = 0; j < dim[2]; j++)
                                sum += int_value[k][i][j];
                    result = QString::number(sum / double(dim[0]));
                }
                else
                    result = QString::number(int_value[0][0][0]);
            }
            else
            {
                QString s = action;
                s = s.remove("[").remove("]");
                QStringList lst = s.split(",", QString::SkipEmptyParts);
                int index0 = 0;
                int index1 = 0;

                if (lst.count() == 2)
                {
                    index0 = lst[0].toInt();
                    if (index0 >= dim[0])
                        index0 = 0;
                    index1 = lst[1].toInt();
                    if (index1 >= dim[1])
                        index1 = 0;
                }

                if (action == "[sum]")
                {
                    int sum = 0;
                    for (int i = 0; i < dim[1]; i++)
                        for (int j = 0; j < dim[2]; j++)
                            sum += int_value[0][i][j];
                    result = QString::number(sum);
                }
                else if (action == "[mean]")
                {
                    double sum = 0;
                    for (int i = 0; i < dim[1]; i++)
                        for (int j = 0; j < dim[2]; j++)
                            sum += int_value[0][i][j];
                    result = QString::number(sum / double(dim[1]) / double(dim[2]));
                }
                else if (action == "[first]")
                    result = QString::number(int_value[0][0][0]);
                else if (action == "[last]")
                    result = QString::number(int_value[0][dim[1] - 1][dim[2] - 1]);
                else
                    result = QString::number(int_value[0][index0][index1]);
            }
        }
    }
    else if (type_class == H5T_FLOAT)
    {
        int rank = dataspace.getSimpleExtentNdims();
        if (rank == 1)
        {
            hsize_t dim[1];
            int ndims = dataspace.getSimpleExtentDims(dim, nullptr);
            double double_value[dim[0]];
            dataset.read(double_value, PredType::NATIVE_DOUBLE, H5S_ALL, H5S_ALL);

            QString s = action;
            int index = s.remove("[").remove("]").toInt();
            if (index >= dim[0])
                index = 0;

            if (action == "[sum]" || action == "[meansum]")
            {
                double sum = 0;
                for (int i = 0; i < dim[0]; i++)
                    sum += double_value[i];
                result = QString::number(sum);
            }
            else if (action == "[mean]")
            {
                double sum = 0;
                for (int i = 0; i < dim[0]; i++)
                    sum += double_value[i];
                result = QString::number(sum / double(dim[0]));
            }
            else if (action == "[last]")
                result = QString::number(double_value[dim[0] - 1]);
            else if (action == "[first]")
                result = QString::number(double_value[0]);
            else
                result = QString::number(double_value[index]);
        }
        else if (rank == 2)
        {
            hsize_t dim[2];
            int ndims = dataspace.getSimpleExtentDims(dim, nullptr);
            double double_value[dim[0]][dim[1]];
            dataset.read(double_value, PredType::NATIVE_DOUBLE, H5S_ALL, H5S_ALL);

            QString s = action;
            s = s.remove("[").remove("]");
            QStringList lst = s.split(",", QString::SkipEmptyParts);
            int index0 = 0;
            int index1 = 0;

            if (lst.count() == 2)
            {
                index0 = lst[0].toInt();
                if (index0 >= dim[0])
                    index0 = 0;
                index1 = lst[1].toInt();
                if (index1 >= dim[1])
                    index1 = 0;
            }

            if (action == "[sum]" || action == "[sum][sum]" || (action.contains("[mean]") && action.contains("[sum]")))
            {
                double sum = 0;
                for (int i = 0; i < dim[0]; i++)
                    for (int j = 0; j < dim[1]; j++)
                        sum += double_value[i][j];
                result = QString::number(sum);
            }
            else if (action == "[mean]" || action == "[mean][mean]")
            {
                double sum = 0;
                for (int i = 0; i < dim[0]; i++)
                    for (int j = 0; j < dim[1]; j++)
                        sum += double_value[i][j];
                result = QString::number(sum / double(dim[0]) / double(dim[1]));
            }
            else if (action == "[first]")
                result = QString::number(double_value[0][0]);
            else if (action == "[last]")
                result = QString::number(double_value[dim[0] - 1][dim[1] - 1]);
            else
                result = QString::number(double_value[index0][index1]);
        }
        else if (rank == 3)
        {
            hsize_t dim[3];
            dataspace.getSimpleExtentDims(dim, nullptr);

            double double_value[dim[0]][dim[1]][dim[2]];
            dataset.read(double_value, PredType::NATIVE_DOUBLE, H5S_ALL, H5S_ALL);

            if (action == "[sum][sum]" || action == "[mean][mean]" ||
                (action.contains("[mean]") && action.contains("[sum]")))
            {
                if (action == "[sum][sum]")
                {
                    double sum = 0;
                    for (int k = 0; k < dim[0]; k++)
                        for (int i = 0; i < dim[1]; i++)
                            for (int j = 0; j < dim[2]; j++)
                                sum += double_value[k][i][j];
                    result = QString::number(sum);
                }
                else if (action == "[mean][mean]")
                {
                    double sum = 0;
                    for (int k = 0; k < dim[0]; k++)
                        for (int i = 0; i < dim[1]; i++)
                            for (int j = 0; j < dim[2]; j++)
                                sum += double_value[k][i][j];
                    result = QString::number(sum / double(dim[0]) / double(dim[1]) / double(dim[2]));
                }
                else if (action.contains("[mean]") && action.contains("[sum]"))
                {
                    double sum = 0;
                    for (int k = 0; k < dim[0]; k++)
                        for (int i = 0; i < dim[1]; i++)
                            for (int j = 0; j < dim[2]; j++)
                                sum += double_value[k][i][j];
                    result = QString::number(sum / double(dim[0]));
                }
                else
                    result = QString::number(double_value[0][0][0]);
            }
            else
            {
                QString s = action;
                s = s.remove("[").remove("]");
                QStringList lst = s.split(",", QString::SkipEmptyParts);
                int index0 = 0;
                int index1 = 0;

                if (lst.count() == 2)
                {
                    index0 = lst[0].toInt();
                    if (index0 >= dim[0])
                        index0 = 0;
                    index1 = lst[1].toInt();
                    if (index1 >= dim[1])
                        index1 = 0;
                }

                if (action == "[sum]")
                {
                    double sum = 0;
                    for (int i = 0; i < dim[1]; i++)
                        for (int j = 0; j < dim[2]; j++)
                            sum += double_value[0][i][j];
                    result = QString::number(sum);
                }
                else if (action == "[mean]")
                {
                    double sum = 0;
                    for (int i = 0; i < dim[1]; i++)
                        for (int j = 0; j < dim[2]; j++)
                            sum += double_value[0][i][j];
                    result = QString::number(sum / double(dim[1]) / double(dim[2]));
                }
                else if (action == "[first]")
                    result = QString::number(double_value[0][0][0]);
                else if (action == "[last]")
                    result = QString::number(double_value[0][dim[1] - 1][dim[2] - 1]);
                else
                    result = QString::number(double_value[0][index0][index1]);
            }
        }
    }
    else if (type_class == H5T_TIME)
        result = "TIME";
    else if (type_class == H5T_ARRAY)
        result = "ARRAY";
    else
        result = "???";

    // close datasetset
    dataset.close();

    // close the HDF5 file
    file.close();

    action = postAction;
    return result;
}
bool ParserHDF5::readMatrix(const QString &fileNameString, QString code, int numberFramesRequested, int dimX, int dimY,
                            gsl_matrix *&matrix)
{
    if (numberFramesRequested != 1) // todo numberFramesRequested >1
        return false;

    bool sumOfAllFrames = false;
    if (code.contains("[sum]"))
    {
        sumOfAllFrames = true;
        code = code.remove("[sum]");
    }

    bool meanOfAllFrames = false;
    if (code.contains("[mean]"))
    {
        meanOfAllFrames = true;
        sumOfAllFrames = true;
        code = code.remove("[mean]");
    }

    const H5std_string FILE_NAME(fileNameString.toLatin1().constData());
    const H5std_string GROUP_NAME(code.toLatin1().constData());

    H5File file(FILE_NAME, H5F_ACC_RDONLY);
    H5::Exception::dontPrint();

    DataSet dataset;
    try
    {
        dataset = file.openDataSet(GROUP_NAME);
    }
    catch (...)
    {
        // close the HDF5 file
        file.close();
        return false;
    }

    DataSpace dataspace = dataset.getSpace();
    H5T_class_t type_class = dataset.getTypeClass();

    hsize_t dim[3];
    int ndims = dataspace.getSimpleExtentDims(dim, nullptr);

    int numberFrames = 1;
    if (ndims > 2)
        numberFrames = int(dim[0]);

    if (numberFrames < numberFramesRequested)
    {
        dataset.close();
        file.close();
        return false;
    }

    if ((ndims == 2 && (dim[0] != dimX || dim[1] != dimY)) || (ndims > 2 && (dim[1] != dimX || dim[2] != dimY)))
    {
        dataset.close();
        file.close();
        return false;
    }

    if (type_class == H5T_INTEGER)
    {
        int int_value[numberFrames][dimX][dimY];
        dataset.read(int_value, PredType::NATIVE_INT, H5S_ALL, H5S_ALL);

        if (sumOfAllFrames)
        {
            for (int k = 1; k < numberFrames; k++)
                for (int i = 0; i < dimX; i++)
                    for (int j = 0; j < dimY; j++)
                        int_value[0][i][j] += int_value[k][i][j];
        }

        double numberAveraged = 1.0;
        if (meanOfAllFrames)
            numberAveraged = numberFrames;

        for (int i = 0; i < dimX; i++)
            for (int j = 0; j < dimY; j++)
                gsl_matrix_set(matrix, j, i, double(int_value[0][i][j] / numberAveraged));
    }
    else if (type_class == H5T_FLOAT)
    {

        double double_value[numberFrames][dimX][dimY];
        dataset.read(double_value, PredType::NATIVE_DOUBLE, H5S_ALL, H5S_ALL);

        if (sumOfAllFrames)
        {
            for (int k = 1; k < numberFrames; k++)
                for (int i = 0; i < dimX; i++)
                    for (int j = 0; j < dimY; j++)
                        double_value[0][i][j] += double_value[k][i][j];
        }

        double numberAveraged = 1.0;
        if (meanOfAllFrames)
            numberAveraged = numberFrames;

        for (int i = 0; i < dimX; i++)
            for (int j = 0; j < dimY; j++)
                gsl_matrix_set(matrix, j, i, double_value[0][i][j] / numberAveraged);
    }
    else
    {
        dataset.close();
        file.close();
        return false;
    }

    // close datasetset
    dataset.close();
    // close the HDF5 file
    file.close();

    return true;
}