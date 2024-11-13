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
    explicit Anova(ApplicationWindow *parent, bool twoWay = false, double level = 0.05);

		void showAnovaTwoWayInteractions(bool show = true){d_show_interactions = show;};
		void setAnovaTwoWayModel(int type){d_anova_type = (gsl_anova_twoway_types)type;};
		bool addSample(const QString& colName, int aLevel = 1, int bLevel = 1);
    bool run() override;

    QString logInfo() override;

		//! \name One-Way ANOVA results
		//@{
    double statistic() override
    {
        return d_at.F;
    }
    double pValue() override
    {
        return d_at.p;
    }
    [[nodiscard]] double fStat() const
    {
        return d_at.F;
    }
    [[nodiscard]] double ssm() const
    {
        return d_at.SSTr;
    }
		//@}

		//! \name Two-Way ANOVA results
		//@{
    [[nodiscard]] double fStatA() const
    {
        return d_att.FA;
    }
    [[nodiscard]] double fStatB() const
    {
        return d_att.FB;
    }
    [[nodiscard]] double fStatAB() const
    {
        return d_att.FAB;
    }
    [[nodiscard]] double pValueA() const
    {
        return d_att.pA;
    }
    [[nodiscard]] double pValueB() const
    {
        return d_att.pB;
    }
    [[nodiscard]] double pValueAB() const
    {
        return d_att.pAB;
    }
    [[nodiscard]] double ssa() const
    {
        return d_att.SSA;
    }
    [[nodiscard]] double ssb() const
    {
        return d_att.SSB;
    }
    [[nodiscard]] double ssab() const
    {
        return d_att.SSAB;
    }
    [[nodiscard]] double msa() const
    {
        return d_att.MSA;
    }
    [[nodiscard]] double msb() const
    {
        return d_att.MSB;
    }
    [[nodiscard]] double msab() const
    {
        return d_att.MSAB;
    }
		//@}

    [[nodiscard]] double sse() const
    {
        if (d_two_way)
            return d_att.SSE;
        return d_at.SSE;
    }
    [[nodiscard]] double sst() const
    {
        if (d_two_way)
            return d_att.SST;
        return d_at.SST;
    }
    [[nodiscard]] double mse() const
    {
        if (d_two_way)
            return d_att.MSE;
        return d_at.MSE;
    }

		//! Returns a pointer to the table created to display the results
    Table *resultTable(const QString &name) override;
    void outputResultsTo(Table *) override;

	protected:
		bool twoWayANOVA();
		bool oneWayANOVA();
    void freeMemory() override;
    static QString levelName(int level, bool b = false);

		bool d_two_way;
		bool d_show_interactions;
		gsl_anova_twoway_types d_anova_type;
		QList<Statistics *> d_data_samples;
		QList<int> d_factorA_levels;
		QList<int> d_factorB_levels;

    tamu_anova_table d_at{};
    tamu_anova_table_twoway d_att{};
};

#endif
