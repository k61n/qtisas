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
                      int Ksvd);
    // Python interface (svd-python.cpp)
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
    QString m_lastError;    // why the last decompose() made no table
    QStringList m_messages; // the warnings of the last decompose()
};

#endif
