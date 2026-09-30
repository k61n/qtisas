/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Python interface for compile18 — function/group discovery,
             FIF management, code generation, and build triggers
 ******************************************************************************/

#include "compile18.h"

// ---------- state getters ----------

QString compile18::functionName() const
{
    return lineEditFunctionName->text();
}

void compile18::setFunctionName(const QString &name)
{
    lineEditFunctionName->setText(name);
}

QString compile18::yName() const
{
    return lineEditY->text();
}

void compile18::setYName(const QString &name)
{
    lineEditY->setText(name);
    changedFXYinfo();
}

QString compile18::xName() const
{
    return lineEditXXX->text();
}

void compile18::setXName(const QString &name)
{
    lineEditXXX->setText(name);
    changedFXYinfo();
}

int compile18::parameterCount() const
{
    return spinBoxP->value();
}

QString compile18::code() const
{
    return textEditCode->toPlainText();
}

void compile18::setCode(const QString &code)
{
    textEditCode->setPlainText(code);
}

QString compile18::info() const
{
    return textEditDescription->toNormalizedHtml();
}

void compile18::setInfo(const QString &text) const
{
    textEditDescription->clear();
    textEditDescription->setNormalizedHtml(text);
}

QString compile18::hHeaders() const
{
    return textEditHFiles->toPlainText();
}

void compile18::setHHeaders(const QString &text)
{
    textEditHFiles->setPlainText(text);
}

QString compile18::includedFunctions() const
{
    return textEditFunctions->toPlainText();
}

void compile18::setIncludedFunctions(const QString &text)
{
    textEditFunctions->setPlainText(text);
}

QString compile18::group() const
{
    return lineEditGroupName->text();
}

void compile18::setGroup(const QString &name)
{
    lineEditGroupName->setText(name);
}

QStringList compile18::groups() const
{
    const QStringList all = categories();
    QStringList result;
    for (const QString &s : all)
        if (s != "ALL" && !s.endsWith('/'))
            result << s;
    return result;
}

QString compile18::category() const
{
    const QModelIndexList sel = listViewGroup->selectionModel()->selectedRows();
    if (sel.isEmpty())
        return {};
    return sel[0].data().toString();
}

QStringList compile18::categories() const
{
    QStringList result;
    QAbstractItemModel *m = listViewGroup->model();
    if (!m)
        return result;
    for (int i = 0; i < m->rowCount(); i++)
        result << m->index(i, 0).data().toString();
    return result;
}

// ---------- category / function selection ----------

bool compile18::selectCategory(const QString &name)
{
    if (!listViewGroup->model())
        return false;

    const QModelIndexList idx =
        listViewGroup->model()->match(listViewGroup->model()->index(0, 0), Qt::DisplayRole, name, 1, Qt::MatchExactly);

    if (idx.isEmpty())
        return false;

    categoryChanged(name);
    return true;
}

QStringList compile18::functionNames(QString categoryName)
{
    if (categoryName.isEmpty())
    {
        const QModelIndexList sel = listViewGroup->selectionModel()->selectedRows();
        if (sel.isEmpty())
            return {};
        categoryName = sel[0].data().toString();
    }

    selectCategory(categoryName);
    return groupFunctions(categoryName);
}

bool compile18::openFunction(const QString &name)
{
    QString fifExt = radioButton2D->isChecked() ? ".2dfif" : ".fif";
    QString fn = name;
    if (!fn.endsWith(".fif") && !fn.endsWith(".2dfif"))
        fn += fifExt;
    const QString fullPath = fn.contains(pathFIF) ? fn : pathFIF + fn;
    if (!QFile::exists(fullPath))
        return false;
    openFIFfile(fn);
    return true;
}

// ---------- parameter table (tableParaNames) ----------

QStringList compile18::paramNames() const
{
    QStringList result;
    for (int i = 0; i < tableParaNames->rowCount(); i++)
        result << tableParaNames->item(i, 0)->text();
    return result;
}

QString compile18::paramName(int i) const
{
    if (i < 0 || i >= tableParaNames->rowCount())
        return {};
    return tableParaNames->item(i, 0)->text();
}

void compile18::setParamName(int i, const QString &name)
{
    if (i < 0 || i >= tableParaNames->rowCount())
        return;
    tableParaNames->item(i, 0)->setText(name);
}

void compile18::setParamNamesAll(const QStringList &names)
{
    for (int i = 0; i < qMin(names.size(), tableParaNames->rowCount()); i++)
        tableParaNames->item(i, 0)->setText(names[i]);
}

double compile18::paramInit(int i) const
{
    if (i < 0 || i >= tableParaNames->rowCount())
        return 0.0;
    return uiToDouble(tableParaNames->item(i, 1)->text());
}

void compile18::setParamInit(int i, double value)
{
    if (i < 0 || i >= tableParaNames->rowCount())
        return;
    tableParaNames->item(i, 1)->setText(uiNumber(value));
}

void compile18::setParamInitsAll(const QVector<double> &values)
{
    for (int i = 0; i < qMin(values.size(), tableParaNames->rowCount()); i++)
        tableParaNames->item(i, 1)->setText(uiNumber(values[i]));
}

QString compile18::paramLimit(int i) const
{
    if (i < 0 || i >= tableParaNames->rowCount())
        return {};
    return rangeUiToC(tableParaNames->item(i, 2)->text());
}

void compile18::setParamLimit(int i, const QString &limit)
{
    if (i < 0 || i >= tableParaNames->rowCount())
        return;
    tableParaNames->item(i, 2)->setText(rangeCToUi(limit));
}

void compile18::setParamLimitsAll(const QStringList &limits)
{
    for (int i = 0; i < qMin(limits.size(), tableParaNames->rowCount()); i++)
        tableParaNames->item(i, 2)->setText(rangeCToUi(limits[i]));
}

bool compile18::paramVaries(int i) const
{
    if (i < 0 || i >= tableParaNames->rowCount())
        return false;
    return tableParaNames->item(i, 2)->checkState() == Qt::Checked;
}

void compile18::setParamVaries(int i, bool varies)
{
    if (i < 0 || i >= tableParaNames->rowCount())
        return;
    tableParaNames->item(i, 2)->setCheckState(varies ? Qt::Checked : Qt::Unchecked);
}

void compile18::setParamVariesAll(const QVector<bool> &varies)
{
    for (int i = 0; i < qMin(varies.size(), tableParaNames->rowCount()); i++)
        tableParaNames->item(i, 2)->setCheckState(varies[i] ? Qt::Checked : Qt::Unchecked);
}

QString compile18::paramDescr(int i) const
{
    if (i < 0 || i >= tableParaNames->rowCount())
        return {};
    return tableParaNames->item(i, 3)->text();
}

void compile18::setParamDescr(int i, const QString &descr)
{
    if (i < 0 || i >= tableParaNames->rowCount())
        return;
    tableParaNames->item(i, 3)->setText(descr);
}

void compile18::setParamDescrsAll(const QStringList &descrs)
{
    for (int i = 0; i < qMin(descrs.size(), tableParaNames->rowCount()); i++)
        tableParaNames->item(i, 3)->setText(descrs[i]);
}

// ---------- save ----------

QString compile18::saveFIF(const QString &name, const QString &customPath, bool askYN)
{
    const QString fifExt = radioButton2D->isChecked() ? ".2dfif" : ".fif";
    const QString base = customPath.isEmpty() ? pathFIF : customPath;

    if (name.contains('/'))
        QDir(base).mkpath(name.section('/', 0, -2));

    const QString stem = name.isEmpty() ? lineEditFunctionName->text().trimmed() : name;
    const QString fn = base + "/" + stem + fifExt;

    const QString text = save(fn, askYN);
    if (!text.isEmpty())
    {
        scanGroups();
        openFIFfile(stem + fifExt);
    }
    return text;
}

void compile18::deleteFIF(const QString &name, const QString &customPath)
{
    const QString fifExt = radioButton2D->isChecked() ? ".2dfif" : ".fif";
    const QString base = customPath.isEmpty() ? pathFIF : customPath;
    const QString stem = name.isEmpty() ? lineEditFunctionName->text().trimmed() : name;
    if (stem.isEmpty())
        return;

    QFile::remove(base + "/" + stem + fifExt);
    for (const QString &ext : {QString(".cpp"), QString(".dll"), QString(".so"), QString(".dylib")})
        QFile::remove(base + "/" + stem + ext);

    if (stem.contains('/'))
    {
        const QString sub = stem.section('/', 0, -2);
        QDir dir(base + "/" + sub);
        if (dir.exists() && dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty() &&
            !sub.contains("IncludedFunctions"))
            QDir(base).rmdir(sub);
    }

    scanGroups();
    newFIF();
}

// ---------- Fit.Control: x-range ----------

bool compile18::customXrange() const
{
    return checkBoxCustomXrange->isChecked();
}

void compile18::setCustomXrange(bool on)
{
    checkBoxCustomXrange->setChecked(on);
}

double compile18::xMin() const
{
    return uiToDouble(lineEditMin->text());
}

void compile18::setXMin(double v)
{
    lineEditMin->setText(uiNumber(v));
}

double compile18::xMax() const
{
    return uiToDouble(lineEditMax->text());
}

void compile18::setXMax(double v)
{
    lineEditMax->setText(uiNumber(v));
}

int compile18::xPoints() const
{
    return lineEditPoints->text().toInt();
}

void compile18::setXPoints(int n)
{
    lineEditPoints->setText(QString::number(n));
}

double compile18::yMin() const
{
    return uiToDouble(lineEditYmin->text());
}

void compile18::setYMin(double v)
{
    lineEditYmin->setText(uiNumber(v));
}

bool compile18::logStep() const
{
    return checkBoxLogStep->isChecked();
}

void compile18::setLogStep(bool on)
{
    checkBoxLogStep->setChecked(on);
}

void compile18::setXRange(bool uniform, double xMin, double xMax, int nPoints, bool logStep, double yMin)
{
    checkBoxCustomXrange->setChecked(true);
    checkBoxUniformX->setChecked(uniform);
    lineEditMin->setText(uiNumber(xMin));
    lineEditMax->setText(uiNumber(xMax));
    lineEditPoints->setText(QString::number(nPoints));
    checkBoxLogStep->setChecked(logStep);
    lineEditYmin->setText(uiNumber(yMin));
}

// ---------- Fit.Control: fit options ----------

bool compile18::eFitEnabled() const
{
    return checkBoxEfit->isChecked();
}

void compile18::setEFitEnabled(bool on)
{
    checkBoxEfit->setChecked(on);
}

QString compile18::eFit() const
{
    return lineEditEFIT->text();
}

void compile18::setEFit(const QString &text)
{
    lineEditEFIT->setText(text);
}

bool compile18::weight() const
{
    return checkBoxWeight->isChecked();
}

void compile18::setWeight(bool on)
{
    checkBoxWeight->setChecked(on);
}

int compile18::weightMethod() const
{
    return comboBoxWeightingMethod->currentIndex();
}

void compile18::setWeightMethod(int idx)
{
    comboBoxWeightingMethod->setCurrentIndex(idx);
}

bool compile18::superpositional() const
{
    return checkBoxSuperpositionalFit->isChecked();
}

void compile18::setSuperpositional(bool on)
{
    checkBoxSuperpositionalFit->setChecked(on);
}

int compile18::subFitCount() const
{
    return spinBoxSubFitNumber->value();
}

void compile18::setSubFitCount(int n)
{
    spinBoxSubFitNumber->setValue(n);
}

bool compile18::useAlgorithm() const
{
    return checkBoxAlgorithm->isChecked();
}

void compile18::setUseAlgorithm(bool on)
{
    checkBoxAlgorithm->setChecked(on);
}

int compile18::fitMethod() const
{
    return comboBoxFitMethod->currentIndex();
}

void compile18::setFitMethod(int idx)
{
    comboBoxFitMethod->setCurrentIndex(idx);
}

QString compile18::fitMethodPara() const
{
    return algorithmUiToC(lineEditFitMethodPara->text());
}

void compile18::setFitMethodPara(const QString &text)
{
    lineEditFitMethodPara->setText(algorithmCToUi(text));
}

void compile18::categoryChanged(const QString &category)
{
    if (!listViewGroup->model())
        return;

    QModelIndex idx;
    if (category.isEmpty())
    {
        const QModelIndexList sel = listViewGroup->selectionModel()->selectedRows();
        if (sel.isEmpty())
            return;
        idx = sel[0];
    }
    else
    {
        const QModelIndexList found = listViewGroup->model()->match(listViewGroup->model()->index(0, 0),
                                                                    Qt::DisplayRole, category, 1, Qt::MatchExactly);
        if (found.isEmpty())
            return;
        idx = found[0];
    }

    listViewGroup->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);
}
