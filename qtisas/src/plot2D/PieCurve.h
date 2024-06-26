/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Pie plot class
 ******************************************************************************/

#ifndef PIECURVE_H
#define PIECURVE_H

#include <qwt/qwt_plot.h>

#include "LegendWidget.h"
#include "PlotCurve.h"

class PieLabel;

//! Pie plot class
class PieCurve: public DataCurve
{
public:
	PieCurve(Table *t, const QString& name, int startRow, int endRow);
    void clone(PieCurve* c);

    double viewAngle(){return d_view_angle;};
    void setViewAngle(double a){d_view_angle = a;};

    double thickness(){return d_thickness;};
    void setThickness(double t){d_thickness = t;};

    double horizontalOffset(){return d_horizontal_offset;};
    void setHorizontalOffset(double d){d_horizontal_offset = d;};

	bool counterClockwise(){return d_counter_clockwise;};
	void setCounterClockwise(bool on){d_counter_clockwise = on;};

	double startAzimuth(){return d_start_azimuth;};
	void setStartAzimuth(double angle){d_start_azimuth = angle;};

    double labelsEdgeDistance(){return d_edge_dist;};
    void setLabelsEdgeDistance(double d){d_edge_dist = d;};

    bool labelsAutoFormat(){return d_auto_labeling;};
    void setLabelsAutoFormat(bool on){d_auto_labeling = on;};

    bool labelsValuesFormat(){return d_values;};
    void setLabelValuesFormat(bool on){d_values = on;};

    bool labelsPercentagesFormat(){return d_percentages;};
    void setLabelPercentagesFormat(bool on){d_percentages = on;};

	bool labelCategories(){return d_categories;};
    void setLabelCategories(bool on){d_categories = on;};

    bool fixedLabelsPosition(){return d_fixed_labels_pos;};
    void setFixedLabelsPosition(bool on){d_fixed_labels_pos = on;};

	QColor color(int i) const;

	int radius(){return d_pie_ray;};
	void setRadius(int size){d_pie_ray = size;};

	Qt::BrushStyle pattern(){return QwtPlotCurve::brush().style();};
	void setBrushStyle(const Qt::BrushStyle& style);

	void setFirstColor(int index){d_first_color = index;};
	int firstColor(){return d_first_color;};

	void loadData();
	void initLabels();
	void clearLabels();

	PieLabel* addLabel(PieLabel *l, bool clone = false);

	QList <PieLabel *> labelsList(){return d_texts_list;};

private:
	void draw(QPainter *painter,const QwtScaleMap &xMap,
		const QwtScaleMap &yMap, int from, int to) const;
	void drawSlices(QPainter *painter, const QwtScaleMap &xMap,
		const QwtScaleMap &yMap, int from, int to) const;
	void drawDisk(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap) const;

	int d_pie_ray;
	int d_first_color;
	double d_start_azimuth;
	double d_view_angle;
	double d_thickness;
	double d_horizontal_offset;
	double d_edge_dist;
	bool d_counter_clockwise;
	bool d_auto_labeling;
	bool d_values;
	bool d_percentages;
	bool d_categories;
	bool d_fixed_labels_pos;
	QList <PieLabel *> d_texts_list;
	//! Stores table row indices to be displayed in PieLabels if d_categories is true.
	QVarLengthArray<int> d_table_rows;
};

class PieLabel: public LegendWidget
{
	Q_OBJECT

public:
    PieLabel(Graph *, PieCurve *pie = 0);

	QString customText();
	void setCustomText(const QString& s){d_custom_text = s;};

	void setPieCurve(PieCurve *pie){d_pie_curve = pie;};

	QString saveToString();
	static void restore(Graph *g, const QStringList& lst);

private:
	void closeEvent(QCloseEvent* e);

	PieCurve *d_pie_curve;
	QString d_custom_text;
};

#endif
