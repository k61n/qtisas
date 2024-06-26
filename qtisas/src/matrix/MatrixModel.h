/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: QtiPlot's matrix model
 ******************************************************************************/

#ifndef MATRIXMODEL_H
#define MATRIXMODEL_H

#include <QLocale>
#include <QSize>
#include <QVector>

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_permutation.h>

#include <QAbstractTableModel>

class Matrix;

class MatrixModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    MatrixModel(int rows = 32, int cols = 32, QObject *parent = 0);
    MatrixModel(const QImage& image, QObject *parent);
    ~MatrixModel(){free(d_data);};

    Matrix *matrix(){return d_matrix;};

	Qt::ItemFlags flags( const QModelIndex & index ) const;

    bool canResize(int rows, int cols);
	void setDimensions(int rows, int cols);
	void resample(int rows, int cols, int method = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
	void setRowCount(int rows);

    int columnCount(const QModelIndex &parent = QModelIndex()) const;
	void setColumnCount(int cols);

	bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());
	bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex());

	bool removeColumns(int column, int count, const QModelIndex & parent = QModelIndex());
	bool insertColumns(int column, int count, const QModelIndex & parent = QModelIndex());

	double x(int col) const;
	double y(int row) const;

	double cell(int row, int col);
	void setCell(int row, int col, double val);

	QString text(int row, int col);
	void setText(int row, int col, const QString&);

	QImage renderImage();

	double data(int row, int col) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	bool setData(const QModelIndex & index, const QVariant & value, int role);

    double* dataVector(){return d_data;};
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	void setImage(const QImage& image);

	bool importASCII(const QString &fname, const QString &sep, int ignoredLines, bool stripSpaces,
					bool simplifySpaces, const QString& commentString, int importAs,
					const QLocale& locale, int endLineChar = 0, int maxRows = -1);

	void setLocale(const QLocale& locale){d_locale = locale;};
	void setNumericFormat(char f, int prec);

	bool initWorkspace();
	void invert();
	void transpose();
	void flipVertically();
	void flipHorizontally();
	void rotate90(bool clockwise);
	void fft(bool inverse);
	void clear(int startRow = 0, int endRow = -1, int startCol = 0, int endCol = -1);
	bool calculate(int startRow = 0, int endRow = -1, int startCol = 0, int endCol = -1);
	bool muParserCalculate(int startRow = 0, int endRow = -1, int startCol = 0, int endCol = -1);
	double* dataCopy(int startRow = 0, int endRow = -1, int startCol = 0, int endCol = -1);
	void pasteData(double *clipboardBuffer, int topRow, int leftCol, int rows, int cols);

private:
	void init();
    int d_rows, d_cols;
    double *d_data;
	Matrix *d_matrix;
	//! Format code for displaying numbers
	char d_txt_format;
	//! Number of significant digits
	int d_num_precision;
	//! Locale used to display data
	QLocale d_locale;

	//! Pointers to GSL matrices used during inversion operations
	gsl_matrix *d_direct_matrix, *d_inv_matrix;
	//! Pointer to a GSL permutation used during inversion operations
	gsl_permutation *d_inv_perm;
	QSize d_data_block_size;
};

#endif
