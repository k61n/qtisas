/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Python interface for fittable18 — parameter access, function
             selection, fit/simulate triggers, and result retrieval
 ******************************************************************************/

#include <cmath>

#include <gsl/gsl_sf_gamma.h>

#include "fittable18.h"
#include "globals.h"

static int paraIndex(QTableWidget *tablePara, const QString &name)
{
    int p = tablePara->rowCount();
    for (int pp = 0; pp < p; pp++)
    {
        auto *item = tablePara->verticalHeaderItem(pp);
        if (item && item->text() == name)
            return pp;
    }
    return -1;
}

int fittable18::parameterCount() const
{
    return spinBoxPara->value();
}

QStringList fittable18::parameterNames() const
{
    QStringList names;
    int p = spinBoxPara->value();
    for (int pp = 0; pp < p; pp++)
    {
        auto *item = tablePara->verticalHeaderItem(pp);
        names << (item ? item->text() : QString::number(pp));
    }
    return names;
}

double fittable18::parameterValue(const QString &name, int dataset) const
{
    int pp = paraIndex(tablePara, name);
    if (pp < 0)
        return qQNaN();
    auto *item = tablePara->item(pp, 3 * (dataset - 1) + 2);
    return item ? uiToDouble(item->text()) : qQNaN();
}

bool fittable18::setParameterValue(const QString &name, double value, int dataset)
{
    int pp = paraIndex(tablePara, name);
    if (pp < 0)
        return false;
    const QString text = uiNumber(value, 'G', spinBoxSignDigits->value());
    const int first = (dataset == -1) ? 1 : dataset;
    const int last = (dataset == -1) ? datasetCount() : dataset;
    for (int d = first; d <= last; ++d)
    {
        auto *item = tablePara->item(pp, 3 * (d - 1) + 2);
        if (item)
            item->setText(text);
    }
    return true;
}

void fittable18::setParameterValueAll(const QVector<double> &values, int dataset)
{
    const int prec = spinBoxSignDigits->value();
    const int first = (dataset == -1) ? 1 : dataset;
    const int last = (dataset == -1) ? datasetCount() : dataset;
    const int count = qMin((int)values.size(), tablePara->rowCount());
    for (int pp = 0; pp < count; ++pp)
    {
        const QString text = QString::number(values[pp], 'G', prec);
        for (int d = first; d <= last; ++d)
        {
            auto *item = tablePara->item(pp, 3 * (d - 1) + 2);
            if (item)
                item->setText(text);
        }
    }
}

void fittable18::setParameterVariesAll(const QVector<bool> &values, int dataset)
{
    const int first = (dataset == -1) ? 1 : dataset;
    const int last = (dataset == -1) ? datasetCount() : dataset;
    const int count = qMin((int)values.size(), tablePara->rowCount());
    for (int pp = 0; pp < count; ++pp)
    {
        const Qt::CheckState state = values[pp] ? Qt::Checked : Qt::Unchecked;
        for (int d = first; d <= last; ++d)
        {
            auto *item = tablePara->item(pp, 3 * (d - 1) + 1);
            if (item)
                item->setCheckState(state);
        }
    }
}

bool fittable18::parameterVaries(const QString &name, int dataset) const
{
    int pp = paraIndex(tablePara, name);
    if (pp < 0)
        return false;
    auto *item = tablePara->item(pp, 3 * (dataset - 1) + 1);
    return item && item->checkState() == Qt::Checked;
}

bool fittable18::setParameterVaries(const QString &name, bool varies, int dataset)
{
    int pp = paraIndex(tablePara, name);
    if (pp < 0)
        return false;
    const int first = (dataset == -1) ? 1 : dataset;
    const int last = (dataset == -1) ? datasetCount() : dataset;
    for (int d = first; d <= last; ++d)
    {
        auto *item = tablePara->item(pp, 3 * (d - 1) + 1);
        if (item)
            item->setCheckState(varies ? Qt::Checked : Qt::Unchecked);
    }
    return true;
}

double fittable18::parameterError(const QString &name, int dataset) const
{
    int pp = paraIndex(tablePara, name);
    if (pp < 0)
        return qQNaN();
    auto *item = tablePara->item(pp, 3 * (dataset - 1) + 3);
    if (!item)
        return qQNaN();
    QString text = item->text();
    text.remove(QChar(177)); // remove ± symbol
    text = text.trimmed();
    if (text == "---" || text.isEmpty())
        return qQNaN();
    return uiToDouble(text);
}

bool fittable18::parameterShared(const QString &name) const
{
    int pp = paraIndex(tablePara, name);
    if (pp < 0)
        return false;
    auto *item = tablePara->item(pp, 0);
    return item && item->checkState() == Qt::Checked;
}

void fittable18::setParameterShared(const QString &name, bool shared)
{
    int pp = paraIndex(tablePara, name);
    if (pp < 0)
        return;
    auto *item = tablePara->item(pp, 0);
    if (item)
        item->setCheckState(shared ? Qt::Checked : Qt::Unchecked);
}

static QString formatLimit(double v, bool isLeft, int prec)
{
    if (std::isinf(v) && v < 0)
        return "-inf";
    if (std::isinf(v) && v > 0)
        return "inf";
    return QString::number(v, 'G', prec);
}

static void parseLocalLimits(const QString &text, double &left, double &right, bool &bayesian)
{
    const double minf = -std::numeric_limits<double>::infinity();
    const double pinf = std::numeric_limits<double>::infinity();
    left = minf;
    right = pinf;
    bayesian = false;
    QString t = text.trimmed().remove(' ');
    if (t.isEmpty() || (!t.contains("..") && !t.contains("+-")))
        return;
    bayesian = t.contains("+-");
    QStringList parts = t.split(bayesian ? "+-" : "..");
    if (parts.size() < 2)
        return;
    left = (parts[0].isEmpty() || parts[0] == "-inf") ? minf : parts[0].toDouble();
    right = (parts[1].isEmpty() || parts[1] == "inf") ? pinf : parts[1].toDouble();
}

double fittable18::parameterLimitLeft(const QString &name, int dataset) const
{
    int pp = paraIndex(tablePara, name);
    if (pp < 0)
        return qQNaN();
    if (dataset >= 1)
    {
        auto *item = tablePara->item(pp, 3 * (dataset - 1) + 1);
        if (!item)
            return qQNaN();
        double l, r;
        bool b;
        parseLocalLimits(rangeUiToC(item->text()), l, r, b);
        return l;
    }
    auto *item = tableControl->item(pp, 0);
    if (!item)
        return qQNaN();
    QString text = item->text().trimmed();
    if (text == "-inf")
        return -std::numeric_limits<double>::infinity();
    return uiToDouble(text);
}

double fittable18::parameterLimitRight(const QString &name, int dataset) const
{
    int pp = paraIndex(tablePara, name);
    if (pp < 0)
        return qQNaN();
    if (dataset >= 1)
    {
        auto *item = tablePara->item(pp, 3 * (dataset - 1) + 1);
        if (!item)
            return qQNaN();
        double l, r;
        bool b;
        parseLocalLimits(rangeUiToC(item->text()), l, r, b);
        return r;
    }
    auto *item = tableControl->item(pp, 4);
    if (!item)
        return qQNaN();
    QString text = item->text().trimmed();
    if (text == "inf")
        return std::numeric_limits<double>::infinity();
    return uiToDouble(text);
}

bool fittable18::parameterBayesian(const QString &name, int dataset) const
{
    int pp = paraIndex(tablePara, name);
    if (pp < 0)
        return false;
    if (dataset >= 1)
    {
        auto *item = tablePara->item(pp, 3 * (dataset - 1) + 1);
        if (!item)
            return false;
        double l, r;
        bool b;
        parseLocalLimits(rangeUiToC(item->text()), l, r, b);
        return b;
    }
    auto *cb = qobject_cast<QComboBox *>(tableControl->cellWidget(pp, 1));
    return cb && cb->currentIndex() == 1;
}

void fittable18::setParameterLimits(const QString &name, double left, double right, bool bayesianYN, int dataset)
{
    int pp = paraIndex(tablePara, name);
    if (pp < 0)
        return;
    int prec = spinBoxSignDigits->value();
    if (dataset >= 1)
    {
        auto *item = tablePara->item(pp, 3 * (dataset - 1) + 1);
        if (!item)
            return;
        QString sep = bayesianYN ? "+-" : "..";
        item->setText(rangeCToUi(formatLimit(left, true, prec) + sep + formatLimit(right, false, prec)));
    }
    else
    {
        auto *itemL = tableControl->item(pp, 0);
        auto *itemR = tableControl->item(pp, 4);
        if (itemL)
            itemL->setText(cToUi(formatLimit(left, true, prec)));
        if (itemR)
            itemR->setText(cToUi(formatLimit(right, false, prec)));
        auto *cb = qobject_cast<QComboBox *>(tableControl->cellWidget(pp, 1));
        if (cb)
            cb->setCurrentIndex(bayesianYN ? 1 : 0);
    }
}

double fittable18::fit()
{
    scriptModeYN = true;
    int savedPlotAction = comboBoxPlotActions->currentIndex();
    comboBoxPlotActions->setCurrentIndex(3);
    bool ok = fitOrCalculate(false);
    lastFitRan = ok;
    comboBoxPlotActions->setCurrentIndex(savedPlotAction);
    scriptModeYN = false;
    tabWidgetFit->setCurrentIndex(2);
    // Previously returned a stale lastChi2 even when the fit never actually ran (e.g. unreadable data).
    // Confirmed live: silently returned a confident-looking result with no indication anything failed.
    return ok ? lastChi2 : std::numeric_limits<double>::quiet_NaN();
}

QString fittable18::fitReport() const
{
    return lastFitReport;
}

// ---------- fit control ----------

int fittable18::fitMethod() const
{
    return comboBoxFitMethod->currentIndex();
}

void fittable18::setFitMethod(int index)
{
    if (index < 0 || index >= comboBoxFitMethod->count())
    {
        app()->scriptPrint(
            QString("setFitMethod: value %1 out of range (0..%2).").arg(index).arg(comboBoxFitMethod->count() - 1));
        return;
    }
    comboBoxFitMethod->setCurrentIndex(index);
    algorithmSelected();
}

void fittable18::setFitMethod(const QString &name)
{
    // "Simplex"=0  "LM"=1  "GenMin"=2
    QString n = name.toLower().trimmed();
    int idx = -1;
    if (n == "simplex")
        idx = 0;
    else if (n == "lm" || n == "levenberg-marquardt" || n == "levenberg")
        idx = 1;
    else if (n == "genmin")
        idx = 2;
    if (idx < 0)
        app()->scriptPrint("setFitMethod: '" + name + "' not found. Use: Simplex, LM, GenMin");
    else
        setFitMethod(idx);
}

int fittable18::maxIterations() const
{
    return spinBoxMaxIter->value();
}
void fittable18::setMaxIterations(int n)
{
    spinBoxMaxIter->setValue(n);
}

double fittable18::tolerance() const
{
    return uiToDouble(lineEditTolerance->text());
}
void fittable18::setTolerance(double v)
{
    lineEditTolerance->setText(uiNumber(v));
}

double fittable18::toleranceAbs() const
{
    return uiToDouble(lineEditToleranceAbs->text());
}
void fittable18::setToleranceAbs(double v)
{
    lineEditToleranceAbs->setText(uiNumber(v));
}

double fittable18::derivStep() const
{
    return uiToDouble(lineEditSTEP->text());
}
void fittable18::setDerivStep(double v)
{
    lineEditSTEP->setText(uiNumber(v));
}

// Levenberg-Marquardt

int fittable18::lmControl() const
{
    return comboBoxLevenberg->currentIndex();
}

void fittable18::setLmControl(int index)
{
    if (index < 0 || index >= comboBoxLevenberg->count())
    {
        app()->scriptPrint(
            QString("setLmControl: value %1 out of range (0..%2).").arg(index).arg(comboBoxLevenberg->count() - 1));
        return;
    }
    comboBoxLevenberg->setCurrentIndex(index);
    levenbergSelected();
}

void fittable18::setLmControl(const QString &name)
{
    // "gradient-scaled"=0  "gradient-unscaled"=1  "delta-scaled"=2  "delta-unscaled"=3
    QString n = name.toLower().trimmed();
    int idx = -1;
    if (n == "gradient-scaled" || n == "gs")
        idx = 0;
    else if (n == "gradient-unscaled" || n == "gu")
        idx = 1;
    else if (n == "delta-scaled" || n == "ds")
        idx = 2;
    else if (n == "delta-unscaled" || n == "du")
        idx = 3;
    if (idx < 0)
        app()->scriptPrint("setLmControl: '" + name +
                           "' not found. Use: gradient-scaled, gradient-unscaled, delta-scaled, delta-unscaled");
    else
        setLmControl(idx);
}

bool fittable18::constChi2() const
{
    return checkBoxConstChi2->isChecked();
}
void fittable18::setConstChi2(bool on)
{
    checkBoxConstChi2->setChecked(on);
}

// Simplex

int fittable18::simplexVariant() const
{
    return comboBoxSimplex->currentIndex();
}

void fittable18::setSimplexVariant(int index)
{
    if (index < 0 || index >= comboBoxSimplex->count())
    {
        app()->scriptPrint(
            QString("setSimplexVariant: value %1 out of range (0..%2).").arg(index).arg(comboBoxSimplex->count() - 1));
        return;
    }
    comboBoxSimplex->setCurrentIndex(index);
}

void fittable18::setSimplexVariant(const QString &name)
{
    // "nmsimplex2"=0  "nmsimplex2rand"=1  "simplex"=2
    QString n = name.toLower().trimmed();
    int idx = -1;
    if (n == "nmsimplex2")
        idx = 0;
    else if (n == "nmsimplex2rand")
        idx = 1;
    else if (n == "simplex")
        idx = 2;
    if (idx < 0)
        app()->scriptPrint("setSimplexVariant: '" + name + "' not found. Use: nmsimplex2, nmsimplex2rand, simplex");
    else
        setSimplexVariant(idx);
}

int fittable18::simplexConvRate() const
{
    return comboBoxConvRate->currentIndex();
}

void fittable18::setSimplexConvRate(int index)
{
    if (index < 0 || index >= comboBoxConvRate->count())
    {
        app()->scriptPrint(QString("setSimplexConvRate: value %1 out of range (0..%2).")
                               .arg(index)
                               .arg(comboBoxConvRate->count() - 1));
        return;
    }
    comboBoxConvRate->setCurrentIndex(index);
}

bool fittable18::constChi2Simplex() const
{
    return checkBoxConstChi2Simplex->isChecked();
}
void fittable18::setConstChi2Simplex(bool on)
{
    checkBoxConstChi2Simplex->setChecked(on);
}

// GenMin

int fittable18::genMinPopulation() const
{
    return spinBoxGenomeCount->value();
}
void fittable18::setGenMinPopulation(int n)
{
    spinBoxGenomeCount->setValue(n);
}

int fittable18::genMinGenomeSize() const
{
    return spinBoxGenomeSize->value();
}
void fittable18::setGenMinGenomeSize(int n)
{
    spinBoxGenomeSize->setValue(n);
}

int fittable18::genMinGenerations() const
{
    return spinBoxMaxNumberGenerations->value();
}
void fittable18::setGenMinGenerations(int n)
{
    spinBoxMaxNumberGenerations->setValue(n);
}

double fittable18::genMinSelectionRate() const
{
    return uiToDouble(lineEditSelectionRate->text());
}
void fittable18::setGenMinSelectionRate(double v)
{
    lineEditSelectionRate->setText(uiNumber(v));
}

double fittable18::genMinMutationRate() const
{
    return uiToDouble(lineEditMutationRate->text());
}
void fittable18::setGenMinMutationRate(double v)
{
    lineEditMutationRate->setText(uiNumber(v));
}

int fittable18::genMinRandomSeed() const
{
    return spinBoxRandomSeed->value();
}
void fittable18::setGenMinRandomSeed(int n)
{
    spinBoxRandomSeed->setValue(n);
}

int fittable18::genMinLocalSearch() const
{
    return comboBoxLocalSearch->currentIndex();
}

void fittable18::setGenMinLocalSearch(int index)
{
    if (index < 0 || index >= comboBoxLocalSearch->count())
    {
        app()->scriptPrint(QString("setGenMinLocalSearch: value %1 out of range (0..%2).")
                               .arg(index)
                               .arg(comboBoxLocalSearch->count() - 1));
        return;
    }
    comboBoxLocalSearch->setCurrentIndex(index);
}

void fittable18::setGenMinLocalSearch(const QString &name)
{
    // "genmin"=0  "levenberg"=1  "genmin+levenberg"=2
    QString n = name.toLower().trimmed();
    int idx = -1;
    if (n == "genmin")
        idx = 0;
    else if (n == "levenberg" || n == "lm")
        idx = 1;
    else if (n == "genmin+levenberg" || n == "both")
        idx = 2;
    if (idx < 0)
        app()->scriptPrint("setGenMinLocalSearch: '" + name + "' not found. Use: genmin, levenberg, genmin+levenberg");
    else
        setGenMinLocalSearch(idx);
}

// ---------- weighting ----------

int fittable18::weightingMethod() const
{
    return comboBoxWeightingMethod->currentIndex();
}

void fittable18::setWeightingMethod(int method)
{
    if (method < 0 || method > 6)
    {
        app()->scriptPrint(QString("setWeightingMethod: value %1 out of range (0..6).").arg(method));
        return;
    }
    comboBoxWeightingMethod->setCurrentIndex(method);
    weightChanged();
}

double fittable18::weightingParamA() const
{
    return spinBoxWA->value();
}
void fittable18::setWeightingParamA(double v)
{
    spinBoxWA->setValue(v);
}
double fittable18::weightingParamB() const
{
    return spinBoxWB->value();
}
void fittable18::setWeightingParamB(double v)
{
    spinBoxWB->setValue(v);
}
double fittable18::weightingParamC() const
{
    return spinBoxWC->value();
}
void fittable18::setWeightingParamC(double v)
{
    spinBoxWC->setValue(v);
}
double fittable18::weightingParamXmax() const
{
    return spinBoxWXMAX->value();
}
void fittable18::setWeightingParamXmax(double v)
{
    spinBoxWXMAX->setValue(v);
}

bool fittable18::weightingEnabled(int m) const
{
    m = m - 1;
    auto *item = tableCurves->item(4, 2 * m);
    return item && item->checkState() == Qt::Checked;
}

void fittable18::setWeightingEnabled(int m, bool enabled)
{
    m = m - 1;
    auto *item = tableCurves->item(4, 2 * m);
    if (item)
        item->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
}

QString fittable18::weightingDataset(int m) const
{
    m = m - 1;
    auto *cb = qobject_cast<QComboBoxInTable *>(tableCurves->cellWidget(4, 2 * m + 1));
    return cb ? cb->currentText() : QString();
}

bool fittable18::setWeightingDataset(int m, const QString &name)
{
    m = m - 1;
    auto *cb = qobject_cast<QComboBoxInTable *>(tableCurves->cellWidget(4, 2 * m + 1));
    if (!cb)
        return false;
    int idx = cb->findText(name);
    if (idx < 0)
        return false;
    cb->setCurrentIndex(idx);
    return true;
}

// ---------- dataset selection ----------

QStringList fittable18::availableDatasets()
{
    QStringList lst = app()->columnsList(Table::Y);
    QStringList result;
    for (const QString &s : lst)
        if (!(s.length() > 14 && s.right(9) == "_residues" &&
              (s.left(14) == "simulatedCurve" || s.left(8) == "fitCurve")))
            result << s;
    return result;
}

QString fittable18::selectedDataset(int m) const
{
    m = m - 1;
    auto *cb = (QComboBox *)tableCurves->cellWidget(0, 2 * m + 1);
    return cb ? cb->currentText() : QString();
}

QStringList fittable18::selectedDatasets() const
{
    QStringList result;
    int M = spinBoxNumberCurvesToFit->value();
    for (int m = 1; m <= M; m++)
        result << selectedDataset(m);
    return result;
}

int fittable18::datasetCount() const
{
    return spinBoxNumberCurvesToFit->value();
}

int fittable18::fittingStage() const
{
    if (widgetStackFit->currentIndex() == 0)
        return 0; // no function loaded
    if (lastFitReport.isEmpty())
        return 1; // function set, not yet fitted
    return 2;     // fitted
}

void fittable18::setDatasetCount(int n)
{
    if (n < 1)
        n = 1;
    checkBoxMultiData->setChecked(n > 1);
    spinBoxNumberCurvesToFit->setValue(n);
    // updateDatasets() is intentionally NOT called here — tableCurves is not
    // built until setFunction() → slotStackFitNext() runs. Call selectDataset()
    // after setFunction() to assign data.
}

bool fittable18::setDataset(int m, const QString &name)
{
    // tableCurves is only valid after setFunction() has transitioned to page 1
    if (widgetStackFit->currentIndex() != 1)
    {
        app()->scriptPrint("setDataset: call setFunction() before selecting a dataset.");
        return false;
    }
    updateDatasets();
    QStringList available = availableDatasets();
    if (!available.contains(name))
    {
        app()->scriptPrint("setDataset: dataset '" + name + "' not found.\nAvailable datasets:\n  " +
                           available.join("\n  "));
        return false;
    }
    return selectData(m, name);
}

void fittable18::setRange(int m, double min, double max, bool byIndex)
{
    selectRange(m, byIndex ? "N" : "Q", min, max);
}

void fittable18::setRangeAll(double min, double max, bool byIndex)
{
    int M = spinBoxNumberCurvesToFit->value();
    for (int mm = 1; mm <= M; mm++)
        selectRange(mm, byIndex ? "N" : "Q", min, max);
}

// ---------- instrument / SANS support ----------

bool fittable18::instrumentalFit() const
{
    return checkBoxSANSsupport->isChecked();
}

bool fittable18::setInstrumentalFit(bool on)
{
    if (widgetStackFit->currentIndex() > 2)
        return false;
    checkBoxSANSsupport->setChecked(on);
    SANSsupportYN();
    if (widgetStackFit->currentIndex() >= 1)
        updateInstrumentalWidgets();
    return true;
}

QString fittable18::instrument() const
{
    return comboBoxInstrument->currentText();
}

bool fittable18::setInstrument(const QString &name)
{
    if (widgetStackFit->currentIndex() > 2)
        return false;
    int idx = comboBoxInstrument->findText(name, Qt::MatchContains);
    if (idx < 0)
        return false;
    comboBoxInstrument->setCurrentIndex(idx);
    SANSsupportYN();
    if (widgetStackFit->currentIndex() >= 1)
        updateInstrumentalWidgets();
    return true;
}

// resolution — global

int fittable18::resoFunction() const
{
    return comboBoxResoFunction->currentIndex();
}

void fittable18::setResoFunction(int index)
{
    if (index < 0 || index >= comboBoxResoFunction->count())
    {
        app()->scriptPrint(QString("setResoFunction: value %1 out of range (0..%2).")
                               .arg(index)
                               .arg(comboBoxResoFunction->count() - 1));
        return;
    }
    comboBoxResoFunction->setCurrentIndex(index);
    speedControlReso();
}

void fittable18::setResoFunction(const QString &name)
{
    // "Gauss-SANS"=0  "Triangular"=1  "Bessel-SANS"=2  "Gauss"=3  "InFunction"=4
    QString n = name.toLower().trimmed();
    int idx = -1;
    if (n == "gauss-sans")
        idx = 0;
    else if (n == "triangular")
        idx = 1;
    else if (n == "bessel-sans")
        idx = 2;
    else if (n == "gauss")
        idx = 3;
    else if (n == "infunction")
        idx = 4;
    if (idx < 0)
        app()->scriptPrint("setResoFunction: '" + name +
                           "' not found. Use: Gauss-SANS, Triangular, Bessel-SANS, Gauss, InFunction");
    else
        setResoFunction(idx);
}

int fittable18::resoSpeedControl() const
{
    return comboBoxSpeedControlReso->currentIndex();
}

void fittable18::setResoSpeedControl(int index)
{
    if (index < 0 || index >= comboBoxSpeedControlReso->count())
    {
        app()->scriptPrint(QString("setResoSpeedControl: value %1 out of range (0..%2).")
                               .arg(index)
                               .arg(comboBoxSpeedControlReso->count() - 1));
        return;
    }
    comboBoxSpeedControlReso->setCurrentIndex(index);
    speedControlReso();
}

int fittable18::resoWorkspace() const
{
    return spinBoxIntWorkspase->value();
}

void fittable18::setResoWorkspace(int n)
{
    spinBoxIntWorkspase->setValue(n);
}

int fittable18::resoSigmaLimit() const
{
    return spinBoxIntLimits->value();
}

void fittable18::setResoSigmaLimit(int n)
{
    spinBoxIntLimits->setValue(n);
}

// resolution — per dataset (tableCurves row 5, m is 1-indexed)

bool fittable18::resoEnabled(int m) const
{
    m = m - 1;
    auto *item = tableCurves->item(5, 2 * m);
    return item && item->checkState() == Qt::Checked;
}

void fittable18::setResoEnabled(int m, bool enabled)
{
    m = m - 1;
    auto *item = tableCurves->item(5, 2 * m);
    if (item)
        item->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
}

QString fittable18::resoDataset(int m) const
{
    m = m - 1;
    auto *cb = qobject_cast<QComboBoxInTable *>(tableCurves->cellWidget(5, 2 * m + 1));
    return cb ? cb->currentText() : QString();
}

bool fittable18::setResoDataset(int m, const QString &name)
{
    m = m - 1;
    auto *cb = qobject_cast<QComboBoxInTable *>(tableCurves->cellWidget(5, 2 * m + 1));
    if (!cb)
        return false;
    int idx = cb->findText(name, Qt::MatchContains);
    if (idx < 0)
        return false;
    cb->setCurrentIndex(idx);
    return true;
}

// polydispersity — global

int fittable18::polyFunction() const
{
    return comboBoxPolyFunction->currentIndex();
}

void fittable18::setPolyFunction(int index)
{
    if (index < 0 || index >= comboBoxPolyFunction->count())
    {
        app()->scriptPrint(QString("setPolyFunction: value %1 out of range (0..%2).")
                               .arg(index)
                               .arg(comboBoxPolyFunction->count() - 1));
        return;
    }
    comboBoxPolyFunction->setCurrentIndex(index);
    SDchanged(index);
}

void fittable18::setPolyFunction(const QString &name)
{
    // "Gauss"=0  "Schultz-Zimm"=1  "Gamma"=2  "Log-Normal"=3  "Uniform"=4  "Triangular"=5
    QString n = name.toLower().trimmed();
    int idx = -1;
    if (n == "gauss")
        idx = 0;
    else if (n == "schultz-zimm" || n == "schultz")
        idx = 1;
    else if (n == "gamma")
        idx = 2;
    else if (n == "log-normal" || n == "lognormal")
        idx = 3;
    else if (n == "uniform")
        idx = 4;
    else if (n == "triangular")
        idx = 5;
    if (idx < 0)
        app()->scriptPrint("setPolyFunction: '" + name +
                           "' not found. Use: Gauss, Schultz-Zimm, Gamma, Log-Normal, Uniform, Triangular");
    else
        setPolyFunction(idx);
}

int fittable18::polySpeedControl() const
{
    return comboBoxSpeedControlPoly->currentIndex();
}

void fittable18::setPolySpeedControl(int index)
{
    if (index < 0 || index >= comboBoxSpeedControlPoly->count())
    {
        app()->scriptPrint(QString("setPolySpeedControl: value %1 out of range (0..%2).")
                               .arg(index)
                               .arg(comboBoxSpeedControlPoly->count() - 1));
        return;
    }
    comboBoxSpeedControlPoly->setCurrentIndex(index);
    speedControlPoly();
}

int fittable18::polyWorkspace() const
{
    return spinBoxIntWorkspasePoly->value();
}

void fittable18::setPolyWorkspace(int n)
{
    spinBoxIntWorkspasePoly->setValue(n);
}

int fittable18::polySigmaLimit() const
{
    return spinBoxIntLimitsPoly->value();
}

void fittable18::setPolySigmaLimit(int n)
{
    spinBoxIntLimitsPoly->setValue(n);
}

// polydispersity — per dataset (tableCurves row 6, m is 1-indexed)

bool fittable18::polyEnabled(int m) const
{
    m = m - 1;
    auto *item = tableCurves->item(6, 2 * m);
    return item && item->checkState() == Qt::Checked;
}

void fittable18::setPolyEnabled(int m, bool enabled)
{
    m = m - 1;
    auto *item = tableCurves->item(6, 2 * m);
    if (item)
        item->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
}

QString fittable18::polyDataset(int m) const
{
    m = m - 1;
    auto *cb = qobject_cast<QComboBoxInTable *>(tableCurves->cellWidget(6, 2 * m + 1));
    return cb ? cb->currentText() : QString();
}

bool fittable18::setPolyDataset(int m, const QString &name)
{
    m = m - 1;
    auto *cb = qobject_cast<QComboBoxInTable *>(tableCurves->cellWidget(6, 2 * m + 1));
    if (!cb)
        return false;
    int idx = cb->findText(name, Qt::MatchContains);
    if (idx < 0)
        return false;
    cb->setCurrentIndex(idx);
    return true;
}

// ---------- function selection ----------

QStringList fittable18::fitFunctions(const QString &pattern) const
{
    QStringList available = groupFunctions("ALL", false);
    available.sort();
    if (pattern.isEmpty())
        return available;
    QRegularExpression re(REGEXPS::wildcardToRegex(pattern).pattern(), QRegularExpression::CaseInsensitiveOption);
    QStringList filtered;
    for (const QString &f : available)
        if (re.match(f).hasMatch())
            filtered << f;
    return filtered;
}

QString fittable18::currentFunction() const
{
    return comboBoxFunction->currentText();
}

bool fittable18::setFunction(const QString &name)
{
    // refresh group list
    scanGroup();

    // select "ALL" group to get the full function list
    const QModelIndexList groupIdx =
        listViewGroup->model()->match(listViewGroup->model()->index(0, 0), Qt::DisplayRole, "ALL", 1, Qt::MatchExactly);

    if (groupIdx.isEmpty())
    {
        app()->scriptPrint("setFunction: no function library found at: " + libPath);
        return false;
    }

    groupChanged(groupIdx[0], groupIdx[0]);
    listViewGroup->setCurrentIndex(groupIdx[0]);

    // search for the requested function
    const QModelIndexList funcIdx = listViewFunctions->model()->match(listViewFunctions->model()->index(0, 0),
                                                                      Qt::DisplayRole, name, 1, Qt::MatchExactly);

    if (funcIdx.isEmpty())
    {
        QStringList available = groupFunctions("ALL", false);
        available.sort();
        app()->scriptPrint("setFunction: function '" + name + "' not found.\nAvailable functions:\n  " +
                           available.join("\n  "));
        return false;
    }

    // highlight in list — triggers openSharedLibraryBySelection synchronously:
    // openSharedLibrary loads the .so/.dylib, sets spinBoxPara, F_paraList etc.
    // openFIFfile sets textLabelFfunc and reads parameter metadata
    listViewFunctions->setCurrentIndex(funcIdx[0]);

    // ensure we are on page 0 so slotStackFitNext() performs the 0→1 transition
    widgetStackFit->setCurrentIndex(0);

    // delegate full fitting-session initialization to slotStackFitNext():
    // initLimits, initFitPage, comboBoxFunction setup, initMultiTable, saveUndo, page transition
    slotStackFitNext();
    // A listed function whose library does not open leaves the init page: confirmed live (local, benchmark LIST 2
    // item 3), configure("ai/sasviewmodels-parallelepiped-fuzzy", []) returned True, the stage stayed "init" with the
    // previous function still set, and selectStage() failed four times with a message about another call.
    if (widgetStackFit->currentIndex() == 0)
    {
        app()->scriptPrint("setFunction: '" + name +
                           "' is listed but could not be opened: its compiled library is missing or does not load.");
        return false;
    }
    return true;
}

bool fittable18::configure(const QString &function, const QStringList &datasets)
{
    // Encapsulates the mandatory sequence: setDatasetCount → setFunction → selectDataset
    selectStage("init");
    int n = datasets.isEmpty() ? 1 : static_cast<int>(datasets.size());
    setDatasetCount(n);

    if (!setFunction(function))
        return false; // setFunction already prints the error + available list

    for (int i = 0; i < datasets.size(); i++)
    {
        if (!setDataset(i + 1, datasets[i]))
        {
            app()->scriptPrint("configure: dataset '" + datasets[i] + "' not found. Use datasets() to list available.");
            return false;
        }
    }
    return true;
}

// ---------- simulation controls (Simulate Curve tab) ----------

bool fittable18::simUniformX() const
{
    return radioButtonUniform_Q->isChecked();
}

void fittable18::setSimUniformX(bool uniform)
{
    if (uniform)
        radioButtonUniform_Q->setChecked(true);
    else
        radioButtonSameQrange->setChecked(true);
}

double fittable18::simFromX() const
{
    return uiToDouble(lineEditFromQsim->text());
}

void fittable18::setSimFromX(double v)
{
    lineEditFromQsim->setText(uiNumber(v, 'g', 6));
}

double fittable18::simToX() const
{
    return uiToDouble(lineEditToQsim->text());
}

void fittable18::setSimToX(double v)
{
    lineEditToQsim->setText(uiNumber(v, 'g', 6));
}

int fittable18::simNumPoints() const
{
    return lineEditNumPointsSim->text().toInt();
}

void fittable18::setSimNumPoints(int n)
{
    lineEditNumPointsSim->setText(QString::number(n));
}

bool fittable18::simLogStep() const
{
    return checkBoxLogStep->isChecked();
}

void fittable18::setSimLogStep(bool on)
{
    checkBoxLogStep->setChecked(on);
}

QString fittable18::simDataset() const
{
    return comboBoxDatasetSim->currentText();
}

bool fittable18::setSimDataset(const QString &name)
{
    int idx = comboBoxDatasetSim->findText(name, Qt::MatchContains);
    if (idx < 0)
    {
        QStringList lst;
        for (int i = 0; i < comboBoxDatasetSim->count(); i++)
            lst << comboBoxDatasetSim->itemText(i);
        app()->scriptPrint(QString("setSimDataset: '%1' not found. Available: %2").arg(name).arg(lst.join(", ")));
        return false;
    }
    comboBoxDatasetSim->setCurrentIndex(idx);
    datasetChangedSim(idx);
    return true;
}

int fittable18::curveColor() const
{
    return comboBoxColor->currentIndex();
}

bool fittable18::setCurveColor(int index)
{
    if (stage() == "init")
    {
        app()->scriptPrint("setCurveColor: not available in 'init' stage — call configure() first");
        return false;
    }
    int n = comboBoxColor->count();
    if (index < 0 || index >= n)
    {
        app()->scriptPrint(QString("setCurveColor: index %1 out of range 0..%2").arg(index).arg(n - 1));
        return false;
    }
    comboBoxColor->blockSignals(true);
    comboBoxColorOnly->blockSignals(true);
    comboBoxColor->setCurrentIndex(index);
    comboBoxColorOnly->setCurrentIndex(index);
    comboBoxColor->blockSignals(false);
    comboBoxColorOnly->blockSignals(false);
    return true;
}

bool fittable18::setCurveColor(const QString &name)
{
    if (stage() == "init")
    {
        app()->scriptPrint("setCurveColor: not available in 'init' stage — call configure() first");
        return false;
    }
    int idx = comboBoxColor->findText(name, Qt::MatchContains | Qt::MatchCaseSensitive);
    if (idx < 0)
        idx = comboBoxColor->findText(name, Qt::MatchContains);
    if (idx < 0)
    {
        QStringList lst;
        for (int i = 0; i < comboBoxColor->count(); i++)
            lst << comboBoxColor->itemText(i);
        app()->scriptPrint(QString("setCurveColor: '%1' not found. Available: %2").arg(name).arg(lst.join(", ")));
        return false;
    }
    comboBoxColor->blockSignals(true);
    comboBoxColorOnly->blockSignals(true);
    comboBoxColor->setCurrentIndex(idx);
    comboBoxColorOnly->setCurrentIndex(idx);
    comboBoxColor->blockSignals(false);
    comboBoxColorOnly->blockSignals(false);
    return true;
}

QString fittable18::stage() const
{
    int page = widgetStackFit->currentIndex();
    if (page == 0)
        return "init";
    if (page == 1)
        return "fit";
    if (tabWidgetGenResults->currentIndex() == 0)
        return "simulate";
    if (tabWidgetGenResults->currentIndex() == 1)
        return "results";
    return "batch-fit";
}

int fittable18::stackPage() const
{
    return widgetStackFit->currentIndex();
}

bool fittable18::selectStage(const QString &s)
{
    int page = widgetStackFit->currentIndex();
    QString sl = s.toLower();

    if (sl == "init")
    {
        if (page == 2)
        {
            slotStackFitPrev();
            slotStackFitPrev();
        }
        else if (page == 1)
        {
            slotStackFitPrev();
        }
        return true;
    }
    if (sl == "fit")
    {
        if (page == 2)
            slotStackFitPrev();
        return true;
    }
    if (sl == "simulate" || sl == "results" || sl == "batch-fit")
    {
        if (page == 1)
        {
            if (!slotStackFitNext())
                return false;
        }
        if (sl == "simulate")
            tabWidgetGenResults->setCurrentIndex(0);
        if (sl == "results")
            tabWidgetGenResults->setCurrentIndex(1);
        if (sl == "batch-fit")
            tabWidgetGenResults->setCurrentIndex(2);
        return true;
    }
    return false;
}

double fittable18::simulate()
{
    int savedPlotAction = comboBoxPlotActions->currentIndex();
    comboBoxPlotActions->setCurrentIndex(3);
    plotSwitcher();
    comboBoxPlotActions->setCurrentIndex(savedPlotAction);
    tabWidgetFit->setCurrentIndex(2);
    return lastChi2;
}

double fittable18::r2Factor() const
{
    bool ok;
    double r2 = uiToDouble(textLabelR2->text(), &ok);
    return ok ? r2 : std::numeric_limits<double>::quiet_NaN();
}

double fittable18::qFactor() const
{
    if (lastDoF <= 0)
        return std::numeric_limits<double>::quiet_NaN();
    return gsl_sf_gamma_inc_Q(lastDoF / 2.0, lastChi2 * lastDoF / 2.0);
}

double fittable18::simulateCurve()
{
    int savedPlotAction = comboBoxPlotActions->currentIndex();
    comboBoxPlotActions->setCurrentIndex(3);
    tabWidgetGenResults->setCurrentIndex(0);
    simulateSwitcher();
    comboBoxPlotActions->setCurrentIndex(savedPlotAction);
    if (radioButtonUniform_Q->isChecked())
        return std::numeric_limits<double>::quiet_NaN();
    bool ok;
    double chi2 = uiToDouble(textLabelChi2dofSim->text(), &ok);
    return ok ? chi2 : std::numeric_limits<double>::quiet_NaN();
}

MultiLayer *fittable18::plotResults(const QString &graphName, const QString &graphTemplate)
{
    // Find existing MultiLayer by object name
    MultiLayer *ml = nullptr;
    if (!graphName.isEmpty())
    {
        foreach (MdiSubWindow *w, app()->windowsList())
        {
            if (w->name() == graphName && w->inherits("MultiLayer"))
            {
                ml = qobject_cast<MultiLayer *>(w);
                break;
            }
        }
    }

    int M = datasetCount();
    if (!ml)
    {
        if (!graphTemplate.isEmpty())
        {
            const QString tplPath = app()->templatesDir + graphTemplate + ".qpt";
            if (QFile::exists(tplPath))
            {
                MdiSubWindow *w = app()->openTemplate(tplPath);
                ml = qobject_cast<MultiLayer *>(w);
            }
        }
        if (!ml)
            ml = app()->newGraph();
        if (!ml)
            return nullptr;
        // interactive=false: no user present to dismiss the blocking rename-rejection dialog in scripted/AI runs —
        // confirmed live it looped once per iteration; report the rejection to script output instead.
        if (!graphName.isEmpty() && !app()->setWindowName(ml, graphName, false))
            app()->scriptPrint("plotResults: '" + graphName +
                               "' is not a valid window name (letters/digits only) — using '" + ml->name() +
                               "' instead.");
    }

    Graph *layer = ml->layer(1);
    Graph *residLayer = (ml->numLayers() >= 2) ? ml->layer(2) : nullptr;

    if (!layer)
        return nullptr;

    int tableType = (widgetStackFit->currentIndex() == 1) ? 1 : 2;

    for (int mm = 0; mm < M; mm++)
    {
        QString yCol = selectedDataset(mm + 1);
        if (!yCol.isEmpty())
            layer->insertCurveScatter(yCol, false);

        QString tableName;
        Table *ttt = nullptr;
        int np = 0;
        double chi2 = 0, TSS = 0;
        if (!generateSimulatedTable(true, tableType, mm, false, tableName, ttt, np, chi2, TSS))
            continue;

        int colorIndex = (checkBoxColorIndexing->isChecked() ? mm : 0) + comboBoxColor->currentIndex();
        addGeneralCurve(layer, tableName, colorIndex, ttt);
        if (residLayer && radioButtonSameQrange->isChecked())
            residLayer->insertCurveScatter(tableName + "_residues", false);
    }

    layer->setAutoScale();
    if (residLayer && radioButtonSameQrange->isChecked())
    {
        residLayer->setAutoScale();
        ml->linkXLayerAxes(true);
    }
    if (app()->isVisible())
    {
        ml->setMaximized();
        ml->setMaximized(ml);
    }
    return ml;
}
