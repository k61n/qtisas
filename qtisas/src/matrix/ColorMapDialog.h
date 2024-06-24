/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A QwtLinearColorMap editor dialog
 ******************************************************************************/

#ifndef COLORMAPDIALOG_H
#define COLORMAPDIALOG_H

#include <QDialog>

class QPushButton;
class ColorMapEditor;
class Matrix;
	
class ColorMapDialog: public QDialog
{
    Q_OBJECT

public:
    explicit ColorMapDialog(QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
	void setMatrix(Matrix *m);
	
protected slots:
	void apply();

private:
	QPushButton *applyBtn, *closeBtn;
	ColorMapEditor *editor;
	Matrix *d_matrix;
};

#endif
