/***************************************************************************
    File                 : Plot3DDialog.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2004-2007 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : Surface plot options dialog

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
#ifndef PLOT3DDIALOG_H
#define PLOT3DDIALOG_H

#include "Graph3D.h"
#include <QCheckBox>

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QTabWidget;
class QWidget;
class QStringList;
class QStackedWidget;
class QDoubleSpinBox;

using namespace Qwt3D;

//! Surface plot options dialog
class Plot3DDialog : public QDialog
{
    Q_OBJECT

public:
    Plot3DDialog( QWidget* parent, const char* name = 0, bool modal = false, Qt::WFlags fl = 0 );
	void setPlot(Graph3D *);

	void showTitleTab();
	void showAxisTab();
	void showGeneralTab();

private slots:
	void accept();
	bool updatePlot();

	QColor pickFromColor();
	QColor pickToColor();
	QColor pickMeshColor();
	QColor pickAxesColor();
	QColor pickTitleColor();
	QColor pickNumberColor();
	QColor pickBgColor();
	QColor pickLabelColor();
	QColor pickGridColor();

	void pickTitleFont();
	void viewAxisOptions(int axis);
	QFont axisFont(int axis);
	void pickAxisLabelFont();
	void pickNumbersFont();

	QStringList scaleOptions(int axis, double start, double end,
							const QString& majors, const QString& minors);
	void viewScaleLimits(int axis);
	void disableMeshOptions();
	void showBarsTab(double rad);
	void showPointsTab(double rad, bool smooth);
	void showConesTab(double rad, int quality);
	void showCrossHairTab(double rad, double linewidth, bool smooth, bool boxed);

	void worksheet();

	void initPointsOptionsStack();
	void changeZoom(int);
	void changeTransparency(int val);

	void showLowerGreek();
	void showUpperGreek();
	void addSymbol(const QString& letter);
    void pickDataColorMap();

private:
    void initScalesPage();
	void initAxesPage();
	void initTitlePage();
	void initColorsPage();
	void initGeneralPage();

    Graph3D *d_plot;
	QFont titleFont, xAxisFont,yAxisFont,zAxisFont, numbersFont;
	QStringList labels, scales, tickLengths;
	QColor titleColor,meshColor,bgColor, axesColor, numColor,labelColor, gridColor;
	QColor fromColor, toColor; //custom data colors
    QDoubleSpinBox *boxMeshLineWidth;
    QPushButton* buttonApply;
    QPushButton* buttonOk;
    QPushButton* buttonCancel, *buttonLowerGreek, *buttonUpperGreek;
	QPushButton* btnTitleColor, *btnTitleFont, *btnLabelFont, *btnGrid;
	QPushButton *btnBackground, *btnMesh, *btnAxes, *btnTitle, *btnLabels, *btnNumbers;
	QPushButton *btnNumbersFont, *btnFromColor, *btnToColor, *btnTable, *btnColorMap;
	QPushButton *buttonAxisLowerGreek, *buttonAxisUpperGreek;
    QTabWidget* generalDialog;
	QWidget *scale, *colors, *general, *axes, *title, *bars, *points;
	QLineEdit *boxTitle, *boxFrom, *boxTo, *boxLabel;
	QSpinBox *boxMajors, *boxMinors;
	QGroupBox *TicksGroupBox, *AxesColorGroupBox;
	QSpinBox *boxResolution, *boxDistance, *boxTransparency;
	QCheckBox *boxLegend, *boxSmooth, *boxBoxed, *boxCrossSmooth, *boxOrthogonal;
	QListWidget *axesList, *axesList2;
	QComboBox *boxType, *boxPointStyle;
	QLineEdit *boxMajorLength, *boxMinorLength, *boxConesRad;
	QSpinBox *boxZoom, *boxXScale, *boxYScale, *boxZScale, *boxQuality;
	QLineEdit *boxSize, *boxBarsRad, *boxCrossRad, *boxCrossLinewidth;
	QStackedWidget *optionStack;
	QWidget *dotsPage, *conesPage, *crossPage;
};

#endif
