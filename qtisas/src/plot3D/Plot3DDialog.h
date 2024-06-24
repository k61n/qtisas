/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2008 rnt <rnt@5a6a7de5-fb12-0410-b871-c33778c25c60>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Surface plot options dialog
 ******************************************************************************/

#ifndef PLOT3DDIALOG_H
#define PLOT3DDIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStringList>
#include <QTabWidget>
#include <QTextEdit>
#include <QWidget>

#include "ColorButton.h"
#include "ColorMapEditor.h"
#include "DoubleSpinBox.h"
#include "Graph3D.h"
#include "TextFormatButtons.h"

using namespace Qwt3D;

//! Surface plot options dialog
class Plot3DDialog : public QDialog
{
    Q_OBJECT

public:
    explicit Plot3DDialog(QWidget *parent, Qt::WindowFlags fl = Qt::WindowFlags());
	void setPlot(Graph3D *);

	void showTitleTab();
	void showAxisTab();
	void showGeneralTab();

private slots:
	void accept();
	bool updatePlot();

	void pickTitleFont();
	void viewAxisOptions(int axis);
	QFont axisFont(int axis);
	void pickAxisLabelFont();
	void pickNumbersFont();

	void viewScaleLimits(int axis);
	void disableMeshOptions();

	void worksheet();

	void initPointsOptionsStack();
	void changeZoom(int);
	void changeTransparency(int val);
    void pickDataColorMap();
	void updateColorMapFileGroupBox(bool);
	void updateLinearColorMapGroupBox(bool);
	void enableMajorGrids(bool on);
	void enableMinorGrids(bool on);

private:
	void initConnections();
    void initScalesPage();
	void initAxesPage();
	void initTitlePage();
	void initGridPage();
	void initColorsPage();
	void initGeneralPage();
	void initPrintPage();
	void setColorMapPreview(const QString& fileName);

	void showBarsTab();
    void showPointsTab(double rad, bool smooth);
	void showConesTab(double rad, int quality);
	void showCrossHairTab(double rad, double linewidth, bool smooth, bool boxed);

    Graph3D *d_plot;
	QGroupBox *linearColorMapGroupBox, *colorMapFileGroupBox;
	QLabel *colorMapPreviewLabel;
	QFont titleFont, xAxisFont,yAxisFont,zAxisFont, numbersFont;
	QStringList labels;
    QDoubleSpinBox *boxMeshLineWidth;
    QPushButton* buttonApply;
    QPushButton* buttonOk;
    QPushButton* buttonCancel;
	QPushButton *btnTitleFont, *btnLabelFont;
    QPushButton *btnNumbersFont, *btnTable, *btnColorMap;
	ColorButton *btnBackground, *btnMesh, *btnAxes, *btnTitleColor, *btnLabels, *btnNumbers, *btnGrid, *btnGridMinor;
	ColorMapEditor *d_color_map_editor;
    QTabWidget* generalDialog;
	QWidget *scale, *colors, *general, *axes, *title, *bars, *points, *gridPage, *printPage;
	DoubleSpinBox *boxFrom, *boxTo;
	QTextEdit *boxTitle, *boxLabel;
	QSpinBox *boxMajors, *boxMinors;
	QGroupBox *TicksGroupBox, *AxesColorGroupBox;
	QSpinBox *boxResolution, *boxDistance, *boxTransparency;
	QSlider *transparencySlider;
	QCheckBox *boxLegend, *boxSmooth, *boxBoxed, *boxCrossSmooth, *boxOrthogonal;
	QListWidget *axesList, *axesList2;
	QComboBox *boxType, *boxPointStyle;
	DoubleSpinBox *boxMajorLength, *boxMinorLength, *boxConesRad;
	QSpinBox *boxZoom, *boxXScale, *boxYScale, *boxZScale, *boxQuality, *boxPrecision;
	DoubleSpinBox *boxSize, *boxBarsRad, *boxCrossRad, *boxCrossLinewidth;
	QStackedWidget *optionStack;
	QWidget *dotsPage, *conesPage, *crossPage;
	QCheckBox *boxBarLines, *boxFilledBars, *boxScaleOnPrint, *boxPrintCropmarks;
	TextFormatButtons *titleFormatButtons, *axisTitleFormatButtons;
    double zoom, xScale, yScale, zScale;
	QString d_color_map_file;
	DoubleSpinBox *boxMajorGridWidth, *boxMinorGridWidth;
	QComboBox *boxMajorGridStyle, *boxMinorGridStyle, *boxTickLabelsFormat;
	QCheckBox *boxMajorGrids, *boxMinorGrids;
	DoubleSpinBox *boxXRotation, *boxYRotation, *boxZRotation;
};

#endif
