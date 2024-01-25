/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Numerical FFT filtering of data sets
 ******************************************************************************/

#ifndef FFTFILTER_H
#define FFTFILTER_H

#include "Filter.h"

class FFTFilter : public Filter
{
Q_OBJECT

public:
	FFTFilter(ApplicationWindow *parent, QwtPlotCurve *c, int m = 1);
	FFTFilter(ApplicationWindow *parent, QwtPlotCurve *c, double start, double end, int m = 1);
	FFTFilter(ApplicationWindow *parent, Graph *g, const QString& curveTitle, int m = 1);
	FFTFilter(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end, int m = 1);
	FFTFilter(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int start = 0, int end = -1, int m = 1);

    enum FilterType{LowPass = 1, HighPass = 2, BandPass = 3, BandBlock = 4};

    void setFilterType(int type);

    //! Sets the cutoff frequency. To be used only for Low Pass and High Pass filters.
    void setCutoff(double f);

    //! Sets the cutoff frequencies. To be used only for the Band Pass and Band block filters.
    void setBand(double lowFreq, double highFreq);

    //! Enables/Disables the DC offset when applying a Band Pass/Band block filter.
    void enableOffset(bool offset = true){d_offset = offset;};

private:
    void init(int m);
    void calculateOutputData(double *x, double *y);

    //! The filter type.
    FilterType d_filter_type;

    //! Cutoff frequency for Low Pass and High Pass filters. Lower edge of the band for Band Pass and Band block filters.
    double d_low_freq;

    //! Upper edge of the band for Band Pass and Band block filters.
    double d_high_freq;

    //! Flag telling if the DC offset must be added/substracted when applying a Band Pass/Band block filter respectively.
    bool d_offset;
};

#endif
