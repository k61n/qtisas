/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: HDF5 parser
 ******************************************************************************/

#include <QStringList>

#include <H5Cpp.h>

#include "compat.h"
#include "parser-hdf5.h"
#include <iostream>

using namespace H5;


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

    const H5std_string FILE_NAME(fileNameString.toLocal8Bit().constData());
    const H5std_string GROUP_NAME(code.toLocal8Bit().constData());

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
            QStringList lst = s.split(",", Qt::SkipEmptyParts);
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
                QStringList lst = s.split(",", Qt::SkipEmptyParts);
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
            QStringList lst = s.split(",", Qt::SkipEmptyParts);
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
                QStringList lst = s.split(",", Qt::SkipEmptyParts);
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

//
void oneDimMerging2x1(gsl_matrix *&matrix, int dim, bool xMerge)
{
    gsl_matrix *tmp = gsl_matrix_alloc(dim, dim);
    gsl_matrix_memcpy(tmp, matrix);
    gsl_matrix_set_all(matrix, 0);

    for (int i = dim / 4; i < 3 * dim / 4; i++)     // x
        for (int j = dim / 4; j < 3 * dim / 4; j++) // y
            if (xMerge)
                gsl_matrix_set(
                    matrix, j, i,
                    (gsl_matrix_get(tmp, j, (i - dim / 4) * 2) + gsl_matrix_get(tmp, j, (i - dim / 4) * 2 + 1)) / 2.0);
            else
                gsl_matrix_set(
                    matrix, j, i,
                    (gsl_matrix_get(tmp, (j - dim / 4) * 2, i) + gsl_matrix_get(tmp, (j - dim / 4) * 2 + 1, i)) / 2.0);
}

template <typename T> bool readHDF5Dataset(H5::DataSet dataset, std::vector<std::vector<std::vector<T>>> &data)
{
    try
    {
        // Get the dataspace
        H5::DataSpace dataspace = dataset.getSpace();

        // Get the dimensions of the dataset
        int rank = dataspace.getSimpleExtentNdims();
        std::vector<hsize_t> dims(rank);
        dataspace.getSimpleExtentDims(dims.data(), nullptr);

        // Get the chunk dimensions
        H5::DSetCreatPropList cparms = dataset.getCreatePlist();
        std::vector<hsize_t> chunk_dims(rank);
        cparms.getChunk(rank, chunk_dims.data());

        // Allocate memory for the entire dataset as a 3D vector
        if (rank == 3)
            data.resize(dims[0], std::vector<std::vector<T>>(dims[1], std::vector<T>(dims[2])));
        else if (rank == 2)
            data.resize(1, std::vector<std::vector<T>>(dims[0], std::vector<T>(dims[1])));

        // Create a memory dataspace for the chunk to be read
        H5::DataSpace memspace(rank, chunk_dims.data());

        // Buffer to hold chunk data temporarily
        int chunk_data_dim = static_cast<int>(chunk_dims[0] * chunk_dims[1]);
        if (rank == 3)
            chunk_data_dim *= static_cast<int>(chunk_dims[2]);
        std::vector<T> chunk_data(chunk_data_dim);

        // Iterate over chunks and read data
        hsize_t offset[3];
        hsize_t count[3];
        count[0] = chunk_dims[0];
        count[1] = chunk_dims[1];
        if (rank == 3)
            count[2] = chunk_dims[2];
        else
            count[2] = 1;

        int dims2 = 1;
        if (rank == 3)
            dims2 = static_cast<int>(dims[2]);

        for (hsize_t i = 0; i < dims[0]; i += count[0])
            for (hsize_t j = 0; j < dims[1]; j += count[1])
                for (hsize_t k = 0; k < dims2; k += count[2])
                {
                    offset[0] = i;
                    offset[1] = j;
                    offset[2] = k;
                    dataspace.selectHyperslab(H5S_SELECT_SET, count, offset);

                    if (std::is_same<T, int>::value)
                        dataset.read(chunk_data.data(), H5::PredType::NATIVE_INT, memspace, dataspace);
                    else if (std::is_same<T, double>::value)
                        dataset.read(chunk_data.data(), H5::PredType::NATIVE_DOUBLE, memspace, dataspace);

                    // Copy chunk data into the 3D vector
                    for (hsize_t x = 0; x < count[0]; ++x)
                        for (hsize_t y = 0; y < count[1]; ++y)
                            for (hsize_t z = 0; z < count[2]; ++z)
                                if ((i + x) < dims[0] && (j + y) < dims[1] && (k + z) < dims2)
                                {
                                    if (rank == 3)
                                        data[i + x][j + y][k + z] =
                                            chunk_data[x * count[1] * count[2] + y * count[2] + z];
                                    else
                                        data[k + z][i + x][j + y] =
                                            chunk_data[x * count[1] * count[2] + y * count[2] + z];
                                }
                }
    }
    catch (H5::FileIException &error)
    {
        std::cerr << "An error occurred: " << error.getCDetailMsg() << std::endl;
        return false;
    }
    catch (H5::DataSetIException &error)
    {
        std::cerr << "An error occurred: " << error.getCDetailMsg() << std::endl;
        return false;
    }
    catch (H5::DataSpaceIException &error)
    {
        std::cerr << "An error occurred: " << error.getCDetailMsg() << std::endl;
        return false;
    }
    return true;
}

// dimData: X or Y dimension of dataset in a file
// dim:  X or Y dimension of matix in the interface
// dimStart: first point in matrix to save dataset
// dimFinish: last point in matrix to save dataset
bool dimScale(int dimData, int dim, int &dimStart, int &dimFinish)
{
    if (dimData >= dim)
        return false;

    dimStart = (dim - dimData) / 2;
    dimFinish = dimStart + dimData;

    return true;
}

// N number of repetitions
// TOF number of TOF channels
// dataType O: dimX-dimY-N; 1: N-dimX-dimY; 2: N-dimX*dimY-TOF; 3: N-dimY*dimX-TOF
bool ParserHDF5::readSingleMatrix(const QString &fileNameString, QString code, gsl_matrix *&matrix, int dimXY,
                                  int roiXY, int N, int TOF, int dataType)
{
    if (N != 1) // todo numberFramesRequested >1
        return false;

    if (TOF != 1) // todo TOF >1
        return false;

    if (dataType < 0 || dataType > 3)
        return false;

    bool merge2x1 = false;
    if (code.contains("[merge2x1]") || code.contains("[merge1x2]"))
    {
        merge2x1 = true;
        code = code.remove("[merge2x1]").remove("[merge1x2]");
    }

    bool sumOfAllFrames = false;
    if (code.contains("[sum]"))
    {
        sumOfAllFrames = true;
        code = code.remove("[sum]");
    }

    bool sumOfAllTofFrames = false;
    if (code.contains("[sumTOF]"))
    {
        sumOfAllTofFrames = true;
        code = code.remove("[sumTOF]");
    }

    bool meanOfAllFrames = false;
    if (code.contains("[mean]"))
    {
        meanOfAllFrames = true;
        sumOfAllFrames = true;
        code = code.remove("[mean]");
    }

    bool meanOfAllTofFrames = false;
    if (code.contains("[meanTOF]"))
    {
        meanOfAllTofFrames = true;
        sumOfAllTofFrames = true;
        code = code.remove("[meanTOF]");
    }

    int frameNumber = 0;
    int frameTofNumber = 0;
    if (code.contains("[#"))
    {
        QStringList lst = code.split("[#");
        code = lst[0];
        if (lst.count() == 2 && lst[1].remove("]").toInt() > 0)
        {
            QStringList lstNTof = lst[1].remove("]").split(",");
            frameNumber = lstNTof[0].remove(" ").toInt();
            frameNumber--;
            if (frameNumber < 0)
                frameNumber = 0;

            if (lstNTof.count() > 1)
            {
                frameTofNumber = lstNTof[1].remove(" ").toInt();
                if (frameTofNumber < 0)
                    frameTofNumber = 0;
            }
        }
    }

    const H5std_string FILE_NAME(fileNameString.toLocal8Bit().constData());
    const H5std_string GROUP_NAME(code.toLocal8Bit().constData());

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
        std::cout << "readSingleMatrix(...)/file.openDataSet(GROUP_NAME): could not open\n";
        return false;
    }

    DataSpace dataspace = dataset.getSpace();
    H5T_class_t type_class = dataset.getTypeClass();
    if (!(type_class == H5T_INTEGER || type_class == H5T_FLOAT))
    {
        // close datasetset
        dataset.close();
        // close the HDF5 file
        file.close();
        std::cout << "type_class is not H5T_INTEGER or H5T_FLOAT\n";
        return false;
    }

    std::vector<std::vector<std::vector<double>>> data;
    if (!readHDF5Dataset(dataset, data))
    {
        // close datasetset
        dataset.close();
        // close the HDF5 file
        file.close();
        std::cout << "readSingleMatrix(...):  _FALSE_ from readHDF5Dataset(...)\n";
        return false;
    }

    int rank = dataspace.getSimpleExtentNdims();
    std::vector<hsize_t> dim(rank);
    int ndims = dataspace.getSimpleExtentDims(dim.data(), nullptr);

    int numberFrames = 1;

    int dimXdata;
    int dimYdata;

    int dimXstart = 0;
    int dimXfinish = dimXY;
    int dimYstart = 0;
    int dimYfinish = dimXY;

    double numberAveraged = 1.0;

    int numberTofFrames = 1;

    gsl_matrix_set_all(matrix, 0);

    int totalDim;

    switch (dataType)
    {

    // O: [dimX][dimY][N]
    case 0:
        if (ndims > 2)
            numberFrames = int(dim[2]);

        if (numberFrames < N)
        {
            dataset.close();
            file.close();
            return false;
        }

        dimXdata = static_cast<int>(dim[0]);
        dimYdata = static_cast<int>(dim[1]);

        dimScale(dimXdata, dimXY, dimXstart, dimXfinish);
        dimScale(dimYdata, dimXY, dimYstart, dimYfinish);

        if (sumOfAllFrames)
            for (int k = 1; k < numberFrames; k++)
                for (int i = 0; i < dimXdata; i++)
                    for (int j = 0; j < dimYdata; j++)
                        data[i][j][0] += data[i][j][k];

        if (meanOfAllFrames)
            numberAveraged = numberFrames;

        if (frameNumber > 0 && frameNumber >= numberFrames)
            frameNumber = 0;

        for (int i = 0; i < dimXdata; i++)
            for (int j = 0; j < dimYdata; j++)
                gsl_matrix_set(matrix, j + dimYstart, i + dimXstart, data[i][j][frameNumber] / numberAveraged);

        break;

    // 1: [N][dimX][dimY]
    case 1:

        if (ndims > 2)
            numberFrames = int(dim[0]);

        if (numberFrames < N)
        {
            dataset.close();
            file.close();
            return false;
        }

        dimXdata = static_cast<int>(dim[0]);
        dimYdata = static_cast<int>(dim[1]);

        if (ndims > 2)
        {
            dimXdata = static_cast<int>(dim[1]);
            dimYdata = static_cast<int>(dim[2]);
        }

        dimScale(dimXdata, dimXY, dimXstart, dimXfinish);
        dimScale(dimYdata, dimXY, dimYstart, dimYfinish);

        if (ndims > 2 && sumOfAllFrames)
            for (int k = 1; k < numberFrames; k++)
                for (int i = 0; i < dimXdata; i++)
                    for (int j = 0; j < dimYdata; j++)
                        data[0][i][j] += data[k][i][j];

        if (ndims > 2 && meanOfAllFrames)
            numberAveraged = numberFrames;

        if (frameNumber > 0 && frameNumber >= numberFrames)
            frameNumber = 0;

        for (int i = 0; i < dimXdata; i++)
            for (int j = 0; j < dimYdata; j++)
                gsl_matrix_set(matrix, j + dimYstart, i + dimXstart, data[frameNumber][i][j] / numberAveraged);

        break;

    // 2: [N][dimX*dimY][TOF]
    case 2:

        if (ndims > 1)
            numberFrames = int(dim[0]);

        if (numberFrames < N)
        {
            dataset.close();
            file.close();
            return false;
        }

        if (ndims > 2)
            numberTofFrames = int(dim[2]);

        if (numberTofFrames < TOF)
        {
            dataset.close();
            file.close();
            return false;
        }

        totalDim = static_cast<int>(dim[1]);
        dimXdata = roiXY;
        dimYdata = totalDim / roiXY;

        dimScale(dimXdata, dimXY, dimXstart, dimXfinish);
        dimScale(dimYdata, dimXY, dimYstart, dimYfinish);

        if (sumOfAllTofFrames)
            for (int k = 0; k < numberFrames; k++)
                for (int i = 0; i < totalDim; i++)
                    for (int t = 1; t < numberTofFrames; t++)
                        data[k][i][0] += data[k][i][t];

        if (meanOfAllTofFrames)
            numberAveraged = numberTofFrames;

        if (sumOfAllFrames)
            for (int k = 1; k < numberFrames; k++)
                for (int t = 0; t < numberTofFrames; t++)
                    for (int i = 0; i < totalDim; i++)
                        data[0][i][t] += data[k][i][t];

        if (meanOfAllFrames)
            numberAveraged *= numberFrames;

        if (frameNumber > 0 && frameNumber >= numberFrames)
            frameNumber = 0;

        if (frameTofNumber > 0 && frameTofNumber >= numberTofFrames)
            frameTofNumber = 0;

        for (int i = 0; i < dimXdata; i++)
            for (int j = 0; j < dimYdata; j++)
                gsl_matrix_set(matrix, j + dimYstart, i + dimXstart,
                               double(data[0][j * dimXdata + i][frameNumber] / numberAveraged));
        break;

    // 3: [N][dimY*dimX][TOF]
    case 3:

        if (ndims > 1)
            numberFrames = int(dim[0]);

        if (numberFrames < N)
        {
            dataset.close();
            file.close();
            return false;
        }

        if (ndims > 2)
            numberTofFrames = int(dim[2]);

        if (numberTofFrames < TOF)
        {
            dataset.close();
            file.close();
            return false;
        }

        totalDim = static_cast<int>(dim[1]);
        dimXdata = roiXY;
        dimYdata = totalDim / roiXY;

        dimScale(dimXdata, dimXY, dimXstart, dimXfinish);
        dimScale(dimYdata, dimXY, dimYstart, dimYfinish);

        if (sumOfAllTofFrames)
            for (int k = 0; k < numberFrames; k++)
                for (int i = 0; i < totalDim; i++)
                    for (int t = 1; t < numberTofFrames; t++)
                        data[k][i][0] += data[k][i][t];

        if (meanOfAllTofFrames)
            numberAveraged = numberTofFrames;

        if (sumOfAllFrames)
            for (int k = 1; k < numberFrames; k++)
                for (int t = 0; t < numberTofFrames; t++)
                    for (int i = 0; i < totalDim; i++)
                        data[0][i][t] += data[k][i][t];

        if (meanOfAllFrames)
            numberAveraged *= numberFrames;

        if (frameNumber > 0 && frameNumber >= numberFrames)
            frameNumber = 0;

        if (frameTofNumber > 0 && frameTofNumber >= numberTofFrames)
            frameTofNumber = 0;

        for (int i = 0; i < dimXdata; i++)
            for (int j = 0; j < dimYdata; j++)
                gsl_matrix_set(matrix, j + dimYstart, i + dimXstart,
                               double(data[0][j + i * dimYdata][frameNumber] / numberAveraged));
        break;
    default:
        break;
    }

    if (merge2x1 && dimXdata / dimYdata == 2)
        oneDimMerging2x1(matrix, dimXY, true);
    else if (merge2x1 && dimYdata / dimXdata == 2)
        oneDimMerging2x1(matrix, dimXY, false);

    // close datasetset
    dataset.close();
    // close the HDF5 file
    file.close();

    return true;
}