/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Matrix undo/redo commands
 ******************************************************************************/

#ifndef MATRIXCOMMAND_H
#define MATRIXCOMMAND_H

#include <QUndoCommand>

#include "Matrix.h"
#include "MatrixModel.h"

//! Matrix commands used by the undo/redo framework
class MatrixEditCellCommand: public QUndoCommand
{
public:
    MatrixEditCellCommand(MatrixModel *model, const QModelIndex & index, double valBefore,
                            double valAfter, const QString & text);
    virtual void redo();
    virtual void undo();

private:
    MatrixModel *d_model;
    QModelIndex d_index;
    double d_val_before;
    double d_val_after;
};

class MatrixSetFormulaCommand: public QUndoCommand
{
public:
    MatrixSetFormulaCommand(Matrix *m, const QString& oldFormula, const QString& newFormula, const QString & text);
    virtual void redo();
    virtual void undo();

private:
    Matrix *d_matrix;
    QString d_old_formula;
    QString d_new_formula;
};

class MatrixSetViewCommand: public QUndoCommand
{
public:
    MatrixSetViewCommand(Matrix *m, Matrix::ViewType oldView, Matrix::ViewType newView, const QString & text);
    virtual void redo();
    virtual void undo();

private:
    Matrix *d_matrix;
    Matrix::ViewType d_old_view;
    Matrix::ViewType d_new_view;
};

class MatrixSetHeaderViewCommand: public QUndoCommand
{
public:
    MatrixSetHeaderViewCommand(Matrix *m, Matrix::HeaderViewType oldView,
                            Matrix::HeaderViewType newView, const QString & text);
    virtual void redo();
    virtual void undo();

private:
    Matrix *d_matrix;
    Matrix::HeaderViewType d_old_view;
    Matrix::HeaderViewType d_new_view;
};

class MatrixSetColWidthCommand: public QUndoCommand
{
public:
    MatrixSetColWidthCommand(Matrix *m, int oldWidth, int newWidth, const QString & text);
    virtual void redo();
    virtual void undo();

private:
    Matrix *d_matrix;
    int d_old_width;
    int d_new_width;
};

class MatrixSetPrecisionCommand: public QUndoCommand
{
public:
    MatrixSetPrecisionCommand(Matrix *m, const QChar& oldFormat, const QChar& newFormat,
                    int oldPrec, int newPrec, const QString & text);
    virtual void redo();
    virtual void undo();

private:
    Matrix *d_matrix;
    QChar d_old_format;
    QChar d_new_format;
    int d_old_prec;
    int d_new_prec;
};

class MatrixSetCoordinatesCommand: public QUndoCommand
{
public:
    MatrixSetCoordinatesCommand(Matrix *, double, double, double, double,
                    double, double, double, double, const QString &);
    virtual void redo();
    virtual void undo();

private:
    Matrix *d_matrix;
    double d_old_xs, d_old_xe, d_old_ys, d_old_ye;
    double d_new_xs, d_new_xe, d_new_ys, d_new_ye;
};

class MatrixSetColorMapCommand: public QUndoCommand
{
public:
    MatrixSetColorMapCommand(Matrix *m, Matrix::ColorMapType d_map_type_before,
					const LinearColorMap& d_map_before, Matrix::ColorMapType d_map_type_after,
					const LinearColorMap& d_map_after, const QString& text);
    virtual void redo();
    virtual void undo();

private:
    Matrix *d_matrix;
    Matrix::ColorMapType d_map_type_before, d_map_type_after;
	LinearColorMap d_map_before, d_map_after;
};

class MatrixDeleteRowsCommand: public QUndoCommand
{
public:
    MatrixDeleteRowsCommand(MatrixModel *model, int startRow, int count, double* data, const QString& text);
    ~MatrixDeleteRowsCommand(){free(d_data);};
    virtual void redo();
    virtual void undo();

private:
    MatrixModel *d_model;
    int d_start_row, d_count;
	double* d_data;
};

class MatrixInsertRowCommand: public QUndoCommand
{
public:
    MatrixInsertRowCommand(MatrixModel *model, int startRow, const QString& text);
    virtual void redo();
    virtual void undo();

private:
    MatrixModel *d_model;
    int d_start_row;
};

class MatrixDeleteColsCommand: public QUndoCommand
{
public:
    MatrixDeleteColsCommand(MatrixModel *model, int startCol, int count, double* data, const QString& text);
    ~MatrixDeleteColsCommand(){free(d_data);};
    virtual void redo();
    virtual void undo();

private:
    MatrixModel *d_model;
    int d_start_col, d_count;
	double* d_data;
};

class MatrixInsertColCommand: public QUndoCommand
{
public:
    MatrixInsertColCommand(MatrixModel *model, int startCol, const QString& text);
    virtual void redo();
    virtual void undo();

private:
    MatrixModel *d_model;
    int d_start_col;
};

class MatrixSetSizeCommand: public QUndoCommand
{
public:
    MatrixSetSizeCommand(MatrixModel *model, const QSize& oldSize, const QSize& newSize, double *data, const QString& text);
	~MatrixSetSizeCommand(){if (d_backup) free(d_backup);};
	virtual void redo();
    virtual void undo();

protected:
    MatrixModel *d_model;
    QSize d_old_size, d_new_size;
	double *d_backup;
};

class MatrixSmoothCommand: public QUndoCommand
{
public:
	MatrixSmoothCommand(MatrixModel *model, double *data, const QString& text);
	~MatrixSmoothCommand(){if (d_backup) free(d_backup);};
	virtual void redo();
	virtual void undo();

protected:
	MatrixModel *d_model;
	double *d_backup;
};

class MatrixResampleCommand: public MatrixSetSizeCommand
{
public:
	MatrixResampleCommand(MatrixModel *model, const QSize& oldSize, const QSize& newSize, int method, double *data, const QString& text);
	virtual void redo();

private:
	int d_method;
};

class MatrixUndoCommand: public QUndoCommand
{
public:
    MatrixUndoCommand(MatrixModel *model, Matrix::Operation op, int startRow, int endRow, int startCol, int endCol,
						double *data, const QString& text);
	~MatrixUndoCommand(){free(d_data);};
    virtual void redo();
    virtual void undo();

protected:
	MatrixModel *d_model;
	Matrix::Operation d_operation;
    int d_start_row, d_end_row, d_start_col, d_end_col;
    double* d_data;
};

class MatrixFftCommand: public MatrixUndoCommand
{
public:
    MatrixFftCommand(bool inverse, MatrixModel *model, int startRow, int endRow,
					int startCol, int endCol, double *data, const QString& text);
    virtual void redo();

private:
    bool d_inverse;
};

class MatrixSetImageCommand: public MatrixUndoCommand
{
public:
    MatrixSetImageCommand(MatrixModel *model, const QImage& image, Matrix::ViewType oldView,
						int startRow, int endRow, int startCol, int endCol, double *data, const QString& text);
    virtual void redo();
    virtual void undo();

private:
    QImage d_image;
	Matrix::ViewType d_old_view;
};

class MatrixImportAsciiCommand: public MatrixUndoCommand
{
public:
    MatrixImportAsciiCommand(const QString &fname, const QString &sep,
						int ignoredLines, bool stripSpaces, bool simplifySpaces,
						const QString& commentString, Matrix::ImportMode importAs, const QLocale& locale,
						int endLineChar, int maxRows, MatrixModel *model, int startRow, int endRow,
						int startCol, int endCol, double *data, const QString& text);
    virtual void redo();

private:
	QString d_path, d_sep, d_comment;
	int d_ignore_lines, d_end_line, d_max_rows;
	bool d_strip_spaces, d_simplify_spaces;
	Matrix::ImportMode d_mode;
	QLocale d_locale;
};

class MatrixSymmetryOperation: public QUndoCommand
{
public:
    MatrixSymmetryOperation(MatrixModel *model, Matrix::Operation op, const QString& text);
    virtual void redo();
    virtual void undo();

private:
    MatrixModel *d_model;
    Matrix::Operation d_operation;
};

class MatrixPasteCommand: public QUndoCommand
{
public:
    MatrixPasteCommand(MatrixModel *model, int startRow, int endRow, int startCol, int endCol,
					double *clipboardData, int rows, int cols, double *backupData,
					int oldRows, int oldCols, const QString& text);
	~MatrixPasteCommand(){free(d_clipboard_data); free(d_backup_data);};
    virtual void redo();
    virtual void undo();

private:
	MatrixModel *d_model;
    int d_start_row, d_end_row, d_start_col, d_end_col, d_rows, d_cols, d_old_rows, d_old_cols;
	double *d_clipboard_data, *d_backup_data;
};

#endif
