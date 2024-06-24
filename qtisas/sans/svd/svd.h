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

#include <QInputDialog>

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

public slots:
    void setFont(QFont fonts);
    void slotReadSvdData(QString tableName);
    void slotSVD1dataTransfer(const QString &Name, int Nsvd, QStringList TexT, gsl_matrix *SLDcomponent, int Msvd,
                              QStringList Dcomposition, QStringList FileNames, gsl_vector *SLDsolvent, int Ksvd);
    void slotTableMsvdChanged(int raw, int col);
    void slotCheckRhoHD();
    void slotSVD1dataTransfer();
    void slotTableNsvdChanged(int row, int col);
    void slotReadDataFromTable();
    void findSvdTables();
};

#endif
