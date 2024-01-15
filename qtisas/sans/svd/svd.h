/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2013 Vitaliy Pipich <v.pipich@gmail.com>
Description: Singular value decomposition interface (SANS)
 ******************************************************************************/

#ifndef SVD_H
#define SVD_H

#include "ui_svd.h"
#include "ApplicationWindow.h"
#include <gsl/gsl_vector.h>	
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>

class svd : public QWidget
{
Q_OBJECT

public:
	svd(QWidget *parent = 0);
   	~svd();
private slots:
	void on_spinBoxMsvd_valueChanged(int value);
	void on_spinBoxNsvd_valueChanged(int value);
	void readSettings();
	void writeSettings();

public slots:
	void setFont(QFont fonts);
	bool checkTableExistence( QString & tableName );
	void slotReadSvdData( QString tableName );
	void slotSVD1dataTransfer( QString Name, int Nsvd, QStringList TexT, gsl_matrix * SLDcomponent, int Msvd, QStringList Dcomposition, QStringList FileNames, gsl_vector * SLDsolvent, int Ksvd );
	void slotTableMsvdChanged( int raw, int col );
	void slotCheckRhoHD();
	void slotTabNameSVDcheck();
	void slotSVD1dataTransfer();
	void slotTableNsvdChanged( int row, int col );
	void slotReadDataFromTable();
	void openHelpOnline( const QUrl & link );
	void forceReadSettings();


private:
 Ui::svdui ui;
 };

#endif
