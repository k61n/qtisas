/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A QwtLinearColorMap editor widget
 ******************************************************************************/

#ifndef COLORMAPEDITOR_H
#define COLORMAPEDITOR_H

#include <QCheckBox>
#include <QComboBox>
#include <QLocale>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

#include "DoubleSpinBox.h"
#include "LinearColorMap.h"
#include "Matrix.h"

//! A complex widget allowing to customize a QwtLinearColorMap.
/**
 * It uses a QTableWidget to display the values in one column and their corresponding colors in a second column.
 * A click on a table color pops-up a QColorDialog allowing to customize it.

	\image html images/color_map_editor.png
 */
class ColorMapEditor: public QWidget
{
    Q_OBJECT

public:
	//! Constructor.
	/**
	* \param parent parent widget (only affects placement of the widget)
	*/
	ColorMapEditor(QStringList mapLst, int initCurrentMap, bool initCurrentLog, QString initMapPath, const QLocale& locale = QLocale::system(), int precision = 6, QWidget* parent = 0, Matrix *m0 = 0);
	//! Returns the customized color map.
    LinearColorMap colorMap(){updateColorMap(); return color_map;};
	//! Use this function to initialize the color map to be edited.
	void setColorMap(const LinearColorMap& map);
	//! Use this function to initialize the values range.
	void setRange(double min, double max);

    int currentMap;
    QString mapPath;
    void colorMapToTable(QList<int> lR, QList<int> lG, QList<int> lB);
    void insertLevel(int row, double val, QColor c);
    void deleteLevel(int row);
    //+++
    QComboBox *colorMaps;
    QCheckBox *scaleColorsBoxLog;
    Matrix *m;

    void setScaledColorsLog(bool scale, bool fromCheckbox);
    void setScaledColorsLog(bool scale, bool fromCheckbox, bool lowLimit, bool upperLimit);
    //---

signals:
	void scalingChanged();

protected slots:
	void updateLowerRangeLimit(double);
	void updateUpperRangeLimit(double);
    void updateColorMap();
	void enableButtons(int row);
	void showColorDialog(int row, int col);
	void insertLevel();
	void deleteLevel();
//+++
    void colorMapsSelected(int);
//---
    
//+++
    void updateScale();
//---
	void setScaledColors(bool scale = true);
//+++
        void setScaledColorsLog(bool scale = false);
//---
	void spinBoxActivated(DoubleSpinBox *);

	bool eventFilter(QObject *object, QEvent *e);

private:
	//! Table displaying the values ranges in the first column and their corresponding colors in the second column
	QTableWidget *table;
	QPushButton *insertBtn, *deleteBtn;
	QCheckBox *scaleColorsBox;
//+++
    QPushButton *updateBtn;
//---
	//! Color map object
	LinearColorMap color_map;
	//! Levels range
	double min_val, max_val;
	//! Locale settings used to display level values
	QLocale d_locale;
	//! Precision used to display level values
	int d_precision;
};

#endif
