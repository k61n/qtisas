/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Box curve
 ******************************************************************************/

#ifndef BOXCURVE_H
#define BOXCURVE_H

#include <qwt/qwt_plot.h>
#include <qwt/qwt_series_data.h>
#include <qwt/qwt_symbol.h>

#include "PlotCurve.h"

//! Box curve
class BoxCurve: public DataCurve
{
public:
	enum BoxStyle{NoBox, Rect, Diamond, WindBox, Notch};
	enum Range{None, SD, SE, r25_75, r10_90, r5_95, r1_99, MinMax, UserDef};
	enum LabelsDisplayPolicy{Percentage, Value, PercentageValue, ValuePercentage};

	BoxCurve(Table *t, const QString& name, int startRow = 0, int endRow = -1);

	void copy(BoxCurve *b);

    [[nodiscard]] QRectF boundingRect() const override;

	QwtSymbol::Style minStyle(){return min_style;};
	void setMinStyle(QwtSymbol::Style s){min_style = s;};

	QwtSymbol::Style maxStyle(){return max_style;};
	void setMaxStyle(QwtSymbol::Style s){max_style = s;};

	void setMeanStyle(QwtSymbol::Style s){mean_style = s;};
	QwtSymbol::Style meanStyle(){return mean_style;};

	void setP99Style(QwtSymbol::Style s){p99_style = s;};
	QwtSymbol::Style p99Style(){return p99_style;};

	void setP1Style(QwtSymbol::Style s){p1_style = s;};
	QwtSymbol::Style p1Style(){return p1_style;};

    [[nodiscard]] int boxStyle() const
    {
        return b_style;
    }
	void setBoxStyle(int style);

    [[nodiscard]] int boxWidth() const
    {
        return b_width;
    }
	void setBoxWidth(int width){b_width=width;};

    [[nodiscard]] double boxRange() const
    {
        return b_coeff;
    }
    [[nodiscard]] int boxRangeType() const
    {
        return b_range;
    }
	void setBoxRange(int type, double coeff);

    [[nodiscard]] double whiskersRange() const
    {
        return w_coeff;
    }
    [[nodiscard]] int whiskersRangeType() const
    {
        return w_range;
    }
	void setWhiskersRange(int type, double coeff = 0.0);

	LabelsDisplayPolicy labelsDisplayPolicy(){return d_labels_display;};
	void setLabelsDisplayPolicy(const LabelsDisplayPolicy& policy);
    [[nodiscard]] bool hasBoxLabels() const
    {
        return d_box_labels;
    }
	void showBoxLabels(bool on = true);

    [[nodiscard]] bool hasWhiskerLabels() const
    {
        return d_whiskers_labels;
    }
	void showWhiskerLabels(bool on = true);

    void loadData() override;

	QString statistics();

	double median(){return quantile(0.5);};
	double quantile(double f);

private:
    void draw(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap, int from, int to) const override;
	void drawBox(QPainter *painter, const QwtScaleMap &xMap,
				const QwtScaleMap &yMap, double *dat, int size) const;
	void drawSymbols(QPainter *painter, const QwtScaleMap &xMap,
				const QwtScaleMap &yMap, double *dat, int size) const;

	double* statisticValues();
	QString labelText(int index, double val);
    [[nodiscard]] QString labelPercentage(int index) const;
	void createLabel(double val);
    void loadLabels() override;
	void updateLabels(bool updateText = true);
    void updateLabelsPosition() override
    {
        updateLabels(false);
    }

	QwtSymbol::Style min_style;
	QwtSymbol::Style max_style;
	QwtSymbol::Style mean_style;
	QwtSymbol::Style p99_style;
	QwtSymbol::Style p1_style;
	double b_coeff;
	double w_coeff;
	int b_range;
	int w_range;
	int b_style;
	int b_width;
	bool d_box_labels;
	bool d_whiskers_labels;
	LabelsDisplayPolicy d_labels_display;
};


/**
 * @brief Series data where all points share a common X coordinate.
 *
 * Extension of QwtSeriesData<QPointF> for representing vertical data sets
 * (e.g., box plots) where multiple Y values are associated with a single X position.
 */
class QwtSingleArrayData : public QwtSeriesData<QPointF>
{
public:
    explicit QwtSingleArrayData(const QVector<QPointF> &data) : d_data(data)
    {
    }

    QwtSingleArrayData(const double x, QVector<double> y, size_t)
    {
        for (const auto &value : y)
            d_data.append(QPointF(x, value));
    }

    QPointF sample(const size_t i) const override
    {
        return d_data[static_cast<qsizetype>(i)];
    }

    size_t size() const override
    {
        return d_data.size();
    }

    QRectF boundingRect() const override
    {
        if (d_data.isEmpty())
            return {1.0, 1.0, -2.0, -2.0}; // invalid rect

        double xVal = d_data[0].x();
        double minY = d_data[0].y();
        double maxY = minY;
        for (int i = 1; i < d_data.size(); ++i)
        {
            double yVal = d_data[i].y();
            if (yVal < minY)
                minY = yVal;
            if (yVal > maxY)
                maxY = yVal;
        }
        return {xVal, minY, 0.0, maxY - minY};
    }

    double x(size_t) const
    {
        return d_data.isEmpty() ? 0.0 : d_data[0].x();
    }

    double y(const size_t i) const
    {
        return d_data[static_cast<qsizetype>(i)].y();
    }

private:
    QVector<QPointF> d_data;
};

#endif
