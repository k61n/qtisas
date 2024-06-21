/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A 2D plot legend widget
 ******************************************************************************/

#ifndef LEGENDWIDGET_H
#define LEGENDWIDGET_H

#include "FrameWidget.h"
#include "Graph.h"

#include <qwt/qwt_plot.h>
#include <qwt/qwt_text.h>

#include <QWidget>

class PlotCurve;

class LegendWidget: public FrameWidget
{
	Q_OBJECT

public:
    LegendWidget(Graph *);
	~LegendWidget();

	void clone(LegendWidget* t);

	QString text(){return d_text->text();};
	void setText(const QString& s);

	QColor textColor(){return d_text->color();};
	void setTextColor(const QColor& c);

	QFont font(){return d_text->font();};
	void setFont(const QFont& font);

	void showTextEditor();
    void print(QPainter *p, const QwtScaleMap map[QwtPlot::axisCnt]);

	QString saveToString();
	static void restore(Graph *g, const QStringList& lst);

	bool isAutoUpdateEnabled(){return d_auto_update;};
	void setAutoUpdate(bool on = true){d_auto_update = on;};

	int angle(){return d_angle;};
	void setAngle(int angle);

	QSize textSize(QPainter *p, const QwtText& text);

	bool hasTeXOutput(){return d_tex_output;};
	void setTeXOutput(bool on = true){d_tex_output = on;};

private:
	PlotCurve* getCurve(const QString& s, int &point);
	void drawVector(PlotCurve *c, QPainter *p, int x, int y, int l);
	void drawSymbol(PlotCurve *c, int point, QPainter *p, int x, int y, int l);
	void drawText(QPainter *, const QRect&, QwtArray<long>, int);

	QwtArray<long> itemsHeight(QPainter *p, int symbolLineLength, int frameWidth, int &width, int &height, int &textWidth, int &textHeight);
	int symbolsMaxWidth();
	QString parse(const QString& str);

	virtual void paintEvent(QPaintEvent *e);

	//! Pointer to the QwtText object
	QwtText* d_text;
    //! Rotation angle
    int d_angle;
	//! Distance between symbols and legend text
	int h_space;
	//! Distance between frame and content
	int left_margin, top_margin;
	//! Length of the symbol line
	int line_length;
	//! Flag telling if the legend should be updated each time a curve is added/removed from the parent plot
	bool d_auto_update;
	//! Flag telling if the legend contains TeX commands that mustn't be escaped when exporting to .tex
	bool d_tex_output;

signals:
	void enableEditor();
};

#endif
