/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
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
	ColorMapDialog(QWidget* parent=0, Qt::WindowFlags fl = 0);
	void setMatrix(Matrix *m);
	
protected slots:
	void apply();

private:
	QPushButton *applyBtn, *closeBtn;
	ColorMapEditor *editor;
	Matrix *d_matrix;
};
   
#endif
