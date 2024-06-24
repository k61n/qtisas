/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: ANOVA
 ******************************************************************************/

#ifndef ANOVA_H
#define ANOVA_H

#include <tamuanova/tamu_anova.h>

#include "StatisticTest.h"

//! ANOVA
class Anova : public StatisticTest
{
	Q_OBJECT

	public:
		Anova(ApplicationWindow *parent, bool twoWay = false, double level = 0.05);

		void showAnovaTwoWayInteractions(bool show = true){d_show_interactions = show;};
		void setAnovaTwoWayModel(int type){d_anova_type = (gsl_anova_twoway_types)type;};
		bool addSample(const QString& colName, int aLevel = 1, int bLevel = 1);
		bool run();

		virtual QString logInfo();

		//! \name One-Way ANOVA results
		//@{
		double fStat(){return d_at.F;};
		double statistic(){return d_at.F;};
		double pValue(){return d_at.p;};
		double ssm(){return d_at.SSTr;};
		//@}

		//! \name Two-Way ANOVA results
		//@{
		double fStatA(){return d_att.FA;};
		double fStatB(){return d_att.FB;};
		double fStatAB(){return d_att.FAB;};

		double pValueA(){return d_att.pA;};
		double pValueB(){return d_att.pB;};
		double pValueAB(){return d_att.pAB;};

		double ssa(){return d_att.SSA;};
		double ssb(){return d_att.SSB;};
		double ssab(){return d_att.SSAB;};

		double msa(){return d_att.MSA;};
		double msb(){return d_att.MSB;};
		double msab(){return d_att.MSAB;};
		//@}

		double sse(){if (d_two_way) return d_att.SSE; return d_at.SSE;};
		double sst(){if (d_two_way) return d_att.SST; return d_at.SST;};
		double mse(){if (d_two_way) return d_att.MSE; return d_at.MSE;};

		//! Returns a pointer to the table created to display the results
		virtual Table *resultTable(const QString& name = QString());
		virtual void outputResultsTo(Table *);

	protected:
		bool twoWayANOVA();
		bool oneWayANOVA();
		void freeMemory();
		QString levelName(int level, bool b = false);

		bool d_two_way;
		bool d_show_interactions;
		gsl_anova_twoway_types d_anova_type;
		QList<Statistics *> d_data_samples;
		QList<int> d_factorA_levels;
		QList<int> d_factorB_levels;

		tamu_anova_table d_at;
		tamu_anova_table_twoway d_att;
};

#endif
