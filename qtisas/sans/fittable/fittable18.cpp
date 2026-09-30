/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Table(s) fitting interface
 ******************************************************************************/

#include <QDockWidget>
#include <QUrl>

#include "fittable18.h"
#include "Folder.h"
#include "globals.h"
#include "numericLocale.h"

fittable18::fittable18(QWidget *parent)
: QWidget(parent)
{
    setupUi(this);
    connectSlot();
    initFITTABLE();
    d_widgetLocale = locale();
}


fittable18::~fittable18()
{
    if (libName != "")
    {
        lib->unload();
        delete lib;
        QFile::remove(libTempName);
    }
}
//*******************************************
//+++ setPathExtern
//*******************************************
void fittable18::setPathExtern(QString path)
{
    libPath = std::move(path);
}
//*******************************************
//+++ app
//*******************************************
ApplicationWindow *fittable18::app()
{
    if (this->parentWidget()->parent())
        return (ApplicationWindow *)((QDockWidget *)this->parentWidget())->parent();
    return (ApplicationWindow *)this->parent();
}

//*******************************************
//+++ numbers in FitTable widgets
//*******************************************
QLocale fittable18::uiLocale() const
{
    return const_cast<fittable18 *>(this)->app()->locale();
}

double fittable18::uiToDouble(const QString &text, bool *ok) const
{
    double value = 0.0;
    const bool parsed = NumericLocale::toDouble(text, uiLocale(), value);
    if (ok)
        *ok = parsed;
    return value;
}

QString fittable18::uiNumber(double value, char format, int precision) const
{
    return uiLocale().toString(value, format, precision);
}

QString fittable18::uiFromLocale(const QString &text, const QLocale &source) const
{
    return NumericLocale::convert(text, source, uiLocale());
}

QString fittable18::uiToLocale(const QString &text, const QLocale &target) const
{
    return NumericLocale::convert(text, uiLocale(), target);
}

QString fittable18::uiToC(const QString &text) const
{
    return NumericLocale::convert(text, uiLocale(), QLocale::c());
}

QString fittable18::cToUi(const QString &text) const
{
    return NumericLocale::convert(text, QLocale::c(), uiLocale());
}

QString fittable18::rangeUiToC(const QString &text) const
{
    return NumericLocale::convertRange(text, uiLocale(), QLocale::c());
}

QString fittable18::rangeCToUi(const QString &text) const
{
    return NumericLocale::convertRange(text, QLocale::c(), uiLocale());
}

void fittable18::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() != QEvent::LocaleChange || locale() == d_widgetLocale)
        return;

    convertWidgetNumbers(d_widgetLocale, locale());
    d_widgetLocale = locale();
}

void fittable18::convertWidgetNumbers(const QLocale &from, const QLocale &to)
{
    const auto convertItem = [&from, &to](QTableWidgetItem *item, bool range) {
        if (item)
            item->setText(range ? NumericLocale::convertRange(item->text(), from, to)
                                : NumericLocale::convert(item->text(), from, to));
    };
    const bool blockPara = tablePara->blockSignals(true);
    const bool blockSimulate = tableParaSimulate->blockSignals(true);
    const bool blockControl = tableControl->blockSignals(true);
    const bool blockMultiFit = tableMultiFit->blockSignals(true);
    const bool blockCurves = tableCurves->blockSignals(true);

    // Parameter table: per dataset "vary" range, value, "±error"
    for (int row = 0; row < tablePara->rowCount(); row++)
        for (int col = 1; col < tablePara->columnCount(); col++)
            convertItem(tablePara->item(row, col), (col - 1) % 3 == 0);

    for (int row = 0; row < tableParaSimulate->rowCount(); row++)
        convertItem(tableParaSimulate->item(row, 0), false);

    for (int row = 0; row < tableControl->rowCount(); row++)
    {
        convertItem(tableControl->item(row, 0), false);
        convertItem(tableControl->item(row, 4), false);
    }

    // Batch table: parameter cells may carry a "|fixed" marker
    for (int row = 1; row < tableMultiFit->rowCount(); row++)
        for (int col = 0; col < tableMultiFit->columnCount(); col++)
        {
            QTableWidgetItem *item = tableMultiFit->item(row, col);
            if (!item)
                continue;
            const QString text = item->text();
            const qsizetype pipe = text.indexOf(QLatin1Char('|'));
            item->setText(pipe >= 0 ? NumericLocale::convert(text.left(pipe), from, to) + text.mid(pipe)
                                    : NumericLocale::convert(text, from, to));
        }

    textLabelChi->setText(NumericLocale::convert(textLabelChi->text(), from, to));
    textLabelR2->setText(NumericLocale::convert(textLabelR2->text(), from, to));

    // Q ranges of the datasets and of the simulation; N ranges are integers and stay unchanged
    for (int col = 1; col < tableCurves->columnCount(); col += 2)
    {
        convertItem(tableCurves->item(2, col), false);
        convertItem(tableCurves->item(3, col), false);
    }
    // ... and the Fit-Control fields (tolerances, derivative step, integration errors, GenMin rates)
    for (QLineEdit *edit :
         {textLabelRangeFirst, textLabelRangeLast, lineEditFromQsim, lineEditToQsim, lineEditImin, lineEditFromQ,
          lineEditToQ, lineEditTolerance, lineEditToleranceAbs, lineEditSTEP, lineEditAbsErr, lineEditRelErr,
          lineEditAbsErrPoly, lineEditRelErrPoly, lineEditSelectionRate, lineEditMutationRate})
    {
        const bool blocked = edit->blockSignals(true);
        edit->setText(NumericLocale::convert(edit->text(), from, to));
        edit->blockSignals(blocked);
    }
    for (QLabel *label :
         {textLabelRangeFirstLimit, textLabelRangeLastLimit, textLabelChi2Sim, textLabelChi2dofSim, textLabelR2sim})
        label->setText(NumericLocale::convert(label->text(), from, to));

    tablePara->blockSignals(blockPara);
    tableParaSimulate->blockSignals(blockSimulate);
    tableControl->blockSignals(blockControl);
    tableMultiFit->blockSignals(blockMultiFit);
    tableCurves->blockSignals(blockCurves);
}

//*******************************************
//*** Log-output
//*******************************************
void fittable18::toResLog(QString text)
{
    QString info =text;
    if (app()->dockWidgetArea(app()->logWindow) == Qt::RightDockWidgetArea )
    {
        app()->current_folder->appendLogInfo(text);
        app()->results->setText(app()->current_folder->logInfo());
    }
    else app()->showResults(text, true);
}
//+++ removeTables by pattern
void fittable18::removeTables(const QString &pattern)
{
    const auto tables = app()->tableList();

    const QRegularExpression rx = REGEXPS::wildcardToRegex(pattern);

    foreach (MdiSubWindow *w, tables)
    {
        if (!rx.match(w->name()).hasMatch())
            continue;

        w->askOnCloseEvent(false);
        app()->closeWindow(w, false);
    }

    Graph *plot = nullptr;
    if (app()->findActiveGraph(plot))
        plot->replot();
}
//*******************************************
//+++  makeNote
//*******************************************
void fittable18::makeNote(QString info, QString name, QString label)
{
    QList<MdiSubWindow *> windows = app()->windowsList();

    foreach (MdiSubWindow *w, windows)
    {
        if (QString(w->metaObject()->className()) == "Note" && w->name()==name)
        {
            ((Note *)w)->currentEditor()->setText(info);
            return;
        }
    }
    
    Note *nn=app()->newNote(name);
    nn->setWindowLabel(label);
    app()->setListViewLabel(nn->name(), label);
    app()->hideWindow(nn);

    //+++
    nn->currentEditor()->setText(info);
}


void fittable18::multiNSEfit(QString tableName, QString fitFunction, int from,int to)
{
    if (tableName=="") return;
    if (fitFunction=="") return;
    if (from>to) return;
    int M=to-from+1;
    
    if ( widgetStackFit->currentIndex()>0 ) slotStackFitPrev();
    if ( widgetStackFit->currentIndex()>0 ) slotStackFitPrev();

    const QModelIndexList indexes = listViewFunctions->model()->match(
        listViewFunctions->model()->index(0, 0), Qt::DisplayRole, fitFunction, 1, Qt::MatchExactly);

    if (indexes.size() <1) return;
    listViewFunctions->setCurrentIndex(indexes.at(0));
    
    checkBoxSANSsupport->setChecked ( false );
    checkBoxMultiData->setChecked ( true );
    spinBoxNumberCurvesToFit->setValue ( M );
    
    slotStackFitNext();
    
    int rawQ=F_paraListF.indexOf("Q");
    if (rawQ<0) rawQ=F_paraListF.indexOf("q");
    qDebug()<<rawQ<<" "<<F_paraListF[0];
    QList<MdiSubWindow *> tableList0=app()->tableList();
    //+++
    foreach (MdiSubWindow *w, tableList0)
    {
        if (w->name()==tableName)
        {
            Table *t=(Table *)w;
            for ( int j=0; j<M; j++ )
            {
                //tableCurves->setText ( 0,2*j,t->text ( from+j-1,3 ) );
                tableCurves->item (0,2*j)->setText(t->text ( from+j-1,19)+"**" );
                tableCurvechanged ( 0,2*j );
                tableCurves->item (0,2*j)->setText("" );
                if (rawQ >= 0)
                    tablePara->item(rawQ, 3 * j + 2)->setText(uiFromLocale(t->text(from + j - 1, 4), t->locale()));
                tableCurvechanged ( 0,2*j+1 );
            }
            plotSwitcher();
        }
    }
}
