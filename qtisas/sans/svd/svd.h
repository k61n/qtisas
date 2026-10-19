/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Singular value decomposition interface (SANS)
 ******************************************************************************/

#ifndef SVD_H
#define SVD_H

#include <QtNumeric>

#include <gsl/gsl_linalg.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>

#include "ui_svd.h"

#include "ApplicationWindow.h"

class svd : public QWidget, private Ui::svdui
{
Q_OBJECT

public:
    explicit svd(QWidget *parent = nullptr);
    ~svd() override;

private slots:
    void on_spinBoxMsvd_valueChanged(int value);
    void on_spinBoxNsvd_valueChanged(int value);
    void readSettings();
    void writeSettings();

  public:
    // the SVD of a contrast series to a new table; its name, or empty with lastError() when nothing could be made
    QString decompose(const QString &Name, int Nsvd, const QStringList &TexT, const gsl_matrix *SLDcomponent, int Msvd,
                      const QStringList &Dcomposition, const QStringList &FileNames, const gsl_vector *SLDsolvent,
                      int Ksvd, double rhoH, double rhoD);
    // singular values below this fraction of the largest are zero: their combinations of the S_ij are not determined
    static constexpr double singularThreshold = 1e-12;

    // Python interface (svd-python.cpp): the input set by calls, independent of the interface's widgets
    void reset();
    bool setSolvent(double sldH2Ocm2, double sldD2Ocm2);
    bool setComponents(const QStringList &names, const QVector<double> &sldH2Ocm2, const QVector<double> &sldD2Ocm2);
    // solventSldCm2: NaN for the SLD of the D2O fraction
    bool setContrasts(const QStringList &tables, const QVector<double> &fractions,
                      const QVector<double> &solventSldCm2);
    QString decompose(const QString &name, int qPoints);
    bool readTable(const QString &name);
    double value(const QString &svdTable, const QString &column, double q);
    QStringList componentNames() const;
    QVector<double> qValues(const QString &svdTable);
    QVector<double> contrastSolventSlds() const;
    QStringList contrastTables() const;
    QVector<double> contrastFractions() const;
    double matchPoint(double sldH2Ocm2, double sldD2Ocm2);
    int rank(const QVector<double> &fractions);
    QVector<double> plan(const QVector<double> &fractions, const QVector<double> &partials, double relError,
                         double background);
    QString lastError() const;
    QStringList lastMessages() const;

  public slots:
    void setFont(QFont fonts);
    void slotReadSvdData(QString tableName);
    void slotTableMsvdChanged(int raw, int col);
    void slotCheckRhoHD();
    void slotSVD1dataTransfer();
    void slotTableNsvdChanged(int row, int col);
    void findSvdTables();

  protected:
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

  private:
    double uiToDouble(const QString &text) const;
    QString uiNumber(double value) const;
    QString uiToC(const QString &text) const;
    QString cToUi(const QString &text) const;
    // A number of a table cell, in the interface's format
    QString tableToUi(Table *t, int row, int col) const;
    void updateTableNameColors();
    QLocale d_widgetLocale;
    QString m_lastError;    // why the last decompose() (or Python call) failed
    QStringList m_messages; // the warnings of the last decompose() (or Python call)
    // the Python interface's input
    double m_sldH2O = qQNaN(), m_sldD2O = qQNaN();
    QStringList m_componentNames, m_contrastTables;
    QVector<double> m_componentH, m_componentD, m_fractions, m_solventSld;
    ApplicationWindow *appWindow();
    bool checkSld(double sld, const QString &what);
    QString missingInput(bool contrasts) const;
    bool contrastMatrix(const QVector<double> &fractions, QVector<QVector<double>> &rows);
};

#endif
