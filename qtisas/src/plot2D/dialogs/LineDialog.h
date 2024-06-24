/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Line options dialog
 ******************************************************************************/

#ifndef LINEDIALOG_H
#define LINEDIALOG_H

#include <qdialog.h>

class QCheckBox;
class QComboBox;
class QPushButton;
class QTabWidget;
class QWidget;
class QSpinBox;
class DoubleSpinBox;
class ColorButton;
class ArrowMarker;
class DoubleSpinBox;
class PenStyleBox;

//! Line options dialog
class LineDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LineDialog(ArrowMarker *line, QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

    enum Unit{ScaleCoordinates, Pixels};

	void initGeometryTab();
	void enableHeadTab();
	void setCoordinates(int unit);

public slots:
    void enableButtonDefault(int);
	void setDefaultValues();
	void displayCoordinates(int unit);
	void accept();
	void apply();

private:
	ArrowMarker *lm;

    ColorButton* colorBox;
    PenStyleBox* styleBox;
    DoubleSpinBox* widthBox;
	QComboBox* unitBox;
    QPushButton* btnOk;
    QPushButton* btnApply;
	QPushButton* buttonDefault;
    QCheckBox* endBox;
    QCheckBox* startBox,  *filledBox;
	QTabWidget* tw;
	QWidget *options, *geometry, *head;
	DoubleSpinBox *xStartBox, *yStartBox, *xEndBox, *yEndBox;
	QSpinBox *xStartPixelBox, *yStartPixelBox, *xEndPixelBox, *yEndPixelBox;
	QSpinBox *boxHeadAngle, *boxHeadLength;
	QComboBox* attachToBox;
};

#endif
