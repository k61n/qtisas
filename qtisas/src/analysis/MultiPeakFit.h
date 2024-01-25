/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: MultiPeakFit module with Lorentz and Gauss peak shapes
 ******************************************************************************/

#ifndef MULTIPEAKFIT_H
#define MULTIPEAKFIT_H

#include "Fit.h"

class MultiPeakFit : public Fit
{
	Q_OBJECT

	public:
		enum PeakProfile{Gauss, Lorentz};
		MultiPeakFit(ApplicationWindow *parent, QwtPlotCurve *c, PeakProfile profile = Gauss, int peaks = 1);
		MultiPeakFit(ApplicationWindow *parent, Graph *g = 0, PeakProfile profile = Gauss, int peaks = 1);
		MultiPeakFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol,
		  			 int startRow = 0, int endRow = -1, PeakProfile profile = Gauss, int peaks = 1);

		int peaks(){return d_peaks;};
		void setNumPeaks(int n);

		void enablePeakCurves(bool on){generate_peak_curves = on;};
		void setPeakCurvesColor(int colorIndex);
		void setPeakCurvesColor(const QColor& color){d_peaks_color = color;};

		static QString generateFormula(int order, PeakProfile profile);
		static QStringList generateParameterList(int order);
		static QStringList generateExplanationList(int order);

		//! Used by the GaussFit and LorentzFit derived classes to calculate initial values for the parameters
		void guessInitialValues();

		virtual double eval(double *par, double x);
		double evalPeak(double *par, double x, int peak);

		PeakProfile profile(){return d_profile;};

	private:
		void init(int);

		QString logFitInfo(int iterations, int status);
		void generateFitCurve();
		static QString peakFormula(int peakIndex, PeakProfile profile);
		//! Inserts a peak function curve into the plot
		void insertPeakFunctionCurve(int peak);
		void customizeFitResults();

		//! Number of peaks
		int d_peaks;

		//! Tells weather the peak curves should be displayed together with the best line fit.
		bool generate_peak_curves;

		//! Color for the peak curves
		QColor d_peaks_color;

		//! The peak profile
		PeakProfile d_profile;
};

class LorentzFit : public MultiPeakFit
{
	Q_OBJECT

	public:
		LorentzFit(ApplicationWindow *parent, QwtPlotCurve *);
		LorentzFit(ApplicationWindow *parent, QwtPlotCurve *, double, double);
		LorentzFit(ApplicationWindow *parent, Graph *g);
		LorentzFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle);
		LorentzFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end);
		LorentzFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow = 0, int endRow = -1);

	private:
		void init();
};

class GaussFit : public MultiPeakFit
{
	Q_OBJECT

	public:
		GaussFit(ApplicationWindow *parent, QwtPlotCurve *);
		GaussFit(ApplicationWindow *parent, QwtPlotCurve *, double, double);
		GaussFit(ApplicationWindow *parent, Graph *g);
		GaussFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle);
		GaussFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end);
		GaussFit( ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow = 0, int endRow = -1);

	private:
		void init();
};

class GaussAmpFit : public Fit
{
	Q_OBJECT

	public:
		GaussAmpFit(ApplicationWindow *parent, QwtPlotCurve *);
		GaussAmpFit(ApplicationWindow *parent, QwtPlotCurve *, double, double);
		GaussAmpFit(ApplicationWindow *parent, Graph *g);
		GaussAmpFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle);
		GaussAmpFit(ApplicationWindow *parent, Graph *g, const QString& curveTitle, double start, double end);
		GaussAmpFit(ApplicationWindow *parent, Table *t, const QString& xCol, const QString& yCol, int startRow = 0, int endRow = -1);

		void guessInitialValues();
        double eval(double *par, double x);

	private:
		void init();
		void calculateFitCurveData(double *X, double *Y);
};
#endif
