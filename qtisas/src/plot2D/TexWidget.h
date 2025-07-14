/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A widget displaying Tex content as image in 2D plots
 ******************************************************************************/

#ifndef TEXWIDGET_H
#define TEXWIDGET_H

#include <QWidget>

#include "FrameWidget.h"

class TexWidget: public FrameWidget
{
	Q_OBJECT

public:
    explicit TexWidget(Graph *, const QString &s = QString(), const QPixmap &pix = QPixmap());

	//! Return the pixmap to be drawn.
	QPixmap pixmap() const {return d_pix;};
	void setPixmap(const QPixmap&);
	
	QString formula(){return d_formula;};
	void setFormula(const QString& s){d_formula = s;};
	
    void print(QPainter *p, const QwtScaleMap map[QwtPlot::axisCnt], double curveLineScalingFactor = 1.0) override;
    QString saveToString() override;
	
	void clone(TexWidget* t);
	static void restore(Graph *g, const QStringList& lst);
	
	void setBestSize();

private:
    void paintEvent(QPaintEvent *e) override;
	QPixmap d_pix;
	QString d_formula;
	int d_margin;
};

#endif
