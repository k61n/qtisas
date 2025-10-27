#ifndef TAMU_ANOVA_H_
#define TAMU_ANOVA_H_

#ifdef __cplusplus
#define BEGIN_DECLS
extern "C"
{
#define END_DECLS }
#else
#define BEGIN_DECLS
#define END_DECLS
#endif

    BEGIN_DECLS

    struct tamu_anova_table
    {
        long dfTr, dfE, dfT;
        double SSTr;
        double SSE;
        double SST;
        double MSTr;
        double MSE;
        double F;
        double p;
    };

    struct tamu_anova_table tamu_anova(double data[], long factor[], long I, long J);

    struct tamu_anova_table_twoway
    {
        long dfA, dfB, dfAB, dfE, dfT;
        double SSA, MSA, FA, pA, SSB, MSB, FB, pB, SSAB, MSAB, FAB, pAB, SSE, MSE, SST;
    };

    enum gsl_anova_twoway_types
    {
        anova_fixed,
        anova_random,
        anova_mixed
    };

    struct tamu_anova_table_twoway tamu_anova_twoway(double data[], long factor[][2], long I, long J[2],
                                                     enum gsl_anova_twoway_types type);

    void tamu_anova_printtable_twoway(struct tamu_anova_table_twoway t);

    void tamu_anova_printtable(struct tamu_anova_table t);

    END_DECLS

#endif /*TAMU_ANOVA_H_*/
