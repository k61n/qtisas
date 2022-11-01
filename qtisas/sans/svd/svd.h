/***************************************************************************
File        : svd.h
Project     : QtiSAS
--------------------------------------------------------------------
Copyright: (C) 2013-2021 by Vitaliy Pipich
Email (use @ for *)  : v.pipich*gmail.com
Description  : Singular Value Decomposition Interface (SANS) 

 ***************************************************************************/

/***************************************************************************
 * *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or  *
 *  (at your option) any later version.*
 * *
 *  This program is distributed in the hope that it will be useful,*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  *
 *  GNU General Public License for more details.   *
 * *
 *   You should have received a copy of the GNU General Public License *
 *   along with this program; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,*
 *   Boston, MA  02110-1301  USA   *
 * *
 ***************************************************************************/
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
