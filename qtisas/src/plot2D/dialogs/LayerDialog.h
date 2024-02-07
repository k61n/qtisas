/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Arrange layers dialog
 ******************************************************************************/

#ifndef LAYERDIALOG_H
#define LAYERDIALOG_H

#include <MultiLayer.h>

#include <QDialog>

class DoubleSpinBox;
class QGroupBox;
class QPushButton;
class QSpinBox;
class QCheckBox;
class QComboBox;

//! Arrange layers dialog
class LayerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LayerDialog(QWidget *parent, bool okMode = false, Qt::WindowFlags fl = Qt::WindowFlags());
	void setMultiLayer(MultiLayer *g);
	void setLayers(int layers);
	void setLayerCanvasSize(int w, int h, int unit);
	void setMargins(int, int, int, int);
	void setRows(int);
	void setColumns(int);
	void setSharedAxes(bool = true);

protected slots:
	void accept();
	void update();
	void enableLayoutOptions(bool ok);
	void swapLayers();
	void updateSizes(int unit);
	void adjustCanvasHeight(double width);
	void adjustCanvasWidth(double height);
	void showCommonAxesBox();

private:
	void closeEvent(QCloseEvent*);
	int convertToPixels(double w, FrameWidget::Unit unit, int dimension);
	double convertFromPixels(int w, FrameWidget::Unit unit, int dimension);

	MultiLayer *multi_layer;

	QPushButton* buttonOk;
	QPushButton* buttonCancel;
	QPushButton* buttonApply;
	QPushButton* buttonSwapLayers;
    QGroupBox *GroupCanvasSize, *GroupGrid;
    QSpinBox *boxX, *boxY, *boxColsGap, *boxRowsGap;
	QSpinBox *boxRightSpace, *boxLeftSpace, *boxTopSpace, *boxBottomSpace;
	DoubleSpinBox *boxCanvasWidth, *boxCanvasHeight;
	QSpinBox *layersBox;
	QSpinBox *boxLayerDest, *boxLayerSrc;
	QCheckBox *fitBox;
	QComboBox *alignHorBox, *alignVertBox;
	QComboBox *unitBox;
	QCheckBox *keepRatioBox;
	QComboBox *alignPolicyBox;
	QCheckBox *commonAxesBox;
	QCheckBox *fixedSizeBox;
	QCheckBox *linkXAxesBox;

	double aspect_ratio;
};

#endif
