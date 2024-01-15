/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2013 Vitaliy Pipich <v.pipich@gmail.com>
Description: Singular value decomposition interface (SANS)
 ******************************************************************************/

#include "svd.h"

#include <QSettings>
#include <QString>
#include <QDockWidget>
#include <QDesktopServices>

svd::svd(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	readSettings();
	
	// connections
	connect(ui.spinBoxMsvd, SIGNAL(valueChanged(int)), this, SLOT(on_spinBoxMsvd_valueChanged(int)));
	connect(ui.spinBoxNsvd, SIGNAL(valueChanged(int)), this, SLOT(on_spinBoxNsvd_valueChanged(int)));
	
   	connect( ui.lineEditTabNameSVD, SIGNAL( textChanged(const QString&) ), this, SLOT( slotTabNameSVDcheck() ) );
    	connect( ui.tableNsvd, SIGNAL( cellChanged(int,int) ), this, SLOT( slotTableNsvdChanged(int,int) ) );
    	connect( ui.tableMsvd, SIGNAL( cellChanged(int,int) ), this, SLOT( slotTableMsvdChanged(int,int) ) );
    	connect( ui.pushButtonSVD1dataTransfer, SIGNAL( clicked() ), this, SLOT( slotSVD1dataTransfer() ) );
    	connect( ui.pushButtonSVD1dataTransferFrom, SIGNAL( clicked() ), this, SLOT( slotReadDataFromTable() ) );
    	connect( ui.lineEditRhoD, SIGNAL( editingFinished() ), this, SLOT( slotCheckRhoHD() ) );
    	connect( ui.lineEditRhoH, SIGNAL( editingFinished() ), this, SLOT( slotCheckRhoHD() ) );
//    	connect( ui.textBrowserHelp, SIGNAL( anchorClicked(const QUrl&) ), this, SLOT( openHelpOnline(const QUrl&) ) );


	// tables tuning
	ui.tableMsvd->setColumnWidth(0,125);
	ui.tableMsvd->setColumnWidth(1,125);
	    
	ui.tableNsvd->setColumnWidth(0,150);
	ui.tableNsvd->setColumnWidth(1,125);
	
	//read settings

};

svd::~svd()
{
    writeSettings();
}

void svd::writeSettings()
{

#ifdef Q_OS_MACOS // Mac
    QSettings settings(QSettings::IniFormat,QSettings::UserScope, "JCNS", "QtiSAS");
#else
    QSettings settings(QSettings::NativeFormat,QSettings::UserScope, "JCNS", "QtiSAS");
#endif
    
    QString ss;

    settings.beginGroup("/SVD");

    //SVD
    settings.setValue("/RhoH",  	ui.lineEditRhoH->text() );
    settings.setValue("/RhoD",  	ui.lineEditRhoD->text() );
    settings.setValue("/TabNameSVD",  	ui.lineEditTabNameSVD->text() );

    settings.setValue("/Msvd",  	ui.spinBoxMsvd->value());
    settings.setValue("/Nsvd",  	ui.spinBoxNsvd->value());
  
    int ii;
    QStringList TabList;

    for (ii=0; (ii< ui.spinBoxMsvd->value());ii++)
    {
	if (ui.tableMsvd->item(ii,0)) ss=ui.tableMsvd->item(ii,0)->text(); else ss=" ";
	TabList << ss; 
    }

    if (TabList.count()==ui.spinBoxMsvd->value()) 
    {
	settings.setValue("/svdSolventComp", TabList); 
	TabList.clear();
    }


    for (ii=0; (ii< ui.spinBoxMsvd->value());ii++) 
    {
	if (ui.tableMsvd->item(ii,1)) ss=ui.tableMsvd->item(ii,1)->text(); else ss=" ";
	TabList<< ss; 
    }
    if (TabList.count() >0 ) 
    {
	settings.setValue("/svdSolvSLD", TabList); 
	TabList.clear();
    }    

    for (ii=0; (ii< ui.spinBoxMsvd->value());ii++) 
    {
	if (ui.tableMsvd->item(ii,2)) ss=ui.tableMsvd->item(ii,2)->text(); else ss=" ";
	TabList<< ss; 
    }
    if (TabList.count()==ui.spinBoxMsvd->value()) 
    {
	settings.setValue("/svdTabNames", TabList); 
	TabList.clear();
    }    
    

    QStringList svdComments,svdSLDinH, svdSLDinD; 
    for (ii=0; (ii< ui.spinBoxNsvd->value());ii++) 
    {
	if (ui.tableNsvd->item(ii,0)) ss=ui.tableNsvd->item(ii,0)->text(); else ss=" ";
	svdComments<<ss; 
	if (ui.tableNsvd->item(ii,1)) ss=ui.tableNsvd->item(ii,1)->text(); else ss=" ";
	svdSLDinH<<ss; 
	if (ui.tableNsvd->item(ii,2)) ss=ui.tableNsvd->item(ii,2)->text(); else ss=" ";
	svdSLDinD<<ss; 
    }
    if (svdComments.count()>0) { settings.setValue("/svdComments", svdComments); svdComments.clear();}
    if (svdSLDinH.count()  >0) { settings.setValue("/svdSLDinH", svdSLDinH);    svdSLDinH.clear();}
    if (svdSLDinD.count()  >0) { settings.setValue("/svdSLDinD", svdSLDinD);    svdSLDinD.clear();}

    settings.endGroup();
}

void svd::readSettings()
{
#ifdef Q_OS_MACOS // Mac
	QSettings settings(QSettings::IniFormat,QSettings::UserScope, "JCNS", "QtiSAS");
#else
	QSettings settings(QSettings::NativeFormat,QSettings::UserScope, "JCNS", "QtiSAS");
#endif
    
    QString ss;
    
    settings.beginGroup("/SVD");
    ss=settings.value("/RhoH","-0.56e10").toString(); if (ss!="") ui.lineEditRhoH->setText(ss);
    ss=settings.value("/RhoD","+6.34e10").toString(); if (ss!="") ui.lineEditRhoD->setText(ss);
    ss=settings.value("/TabNameSVD","TEST1").toString(); if (ss!="") ui.lineEditTabNameSVD->setText(ss);

    int nn=settings.value("/Nsvd",2).toInt(); ui.spinBoxNsvd->setValue(nn);
    int mm=settings.value("/Msvd",2).toInt(); ui.spinBoxMsvd->setValue(mm);

    on_spinBoxMsvd_valueChanged(mm);
    on_spinBoxNsvd_valueChanged(nn);	

    QStringList svdTabNames = settings.value("/svdTabNames").toStringList();
    QStringList svdSolventComp = settings.value("/svdSolventComp").toStringList();
    QStringList svdSolvSLD = settings.value("/svdSolvSLD").toStringList();

    int ii;
 
    for (ii=0; (ii< mm);ii++) 
    {
	if (ii < svdTabNames.count())
	{
		ss=svdTabNames[ii];
		QTableWidgetItem *_item1=new QTableWidgetItem();
		_item1->setText ( ss );
		ui.tableMsvd->setItem(ii,2,_item1);
	}
//	tableMsvd->setText(ii,2,ss);
	if (ii < svdSolventComp.count())
	{	
		ss=svdSolventComp[ii];
		QTableWidgetItem *_item2=new QTableWidgetItem();
		_item2->setText ( ss );
		ui.tableMsvd->setItem(ii,0,_item2);
	}
//	tableMsvd->setText(ii,0,ss);
	if (ii < svdSolvSLD.count())
	{	
		ss=svdSolvSLD[ii];
		QTableWidgetItem *_item3=new QTableWidgetItem();
		_item3->setText ( ss );
		ui.tableMsvd->setItem(ii,1,_item3);
	}
//	tableMsvd->setText(ii,1,ss);
    }
    
    QStringList svdComments = settings.value("/svdComments").toStringList();
    QStringList svdSLDinH = settings.value("/svdSLDinH").toStringList();
    QStringList svdSLDinD = settings.value("/svdSLDinD").toStringList();

    for (ii=0; (ii< nn);ii++) 
    {
	if (ii < svdComments.count())
	{
		ss=svdComments[ii];
		QTableWidgetItem *_item1=new QTableWidgetItem();
		_item1->setText ( ss );
		ui.tableNsvd->setItem(ii,0,_item1);
	}
//	tableNsvd->setText(ii,0,ss);
	if (ii < svdSLDinH.count())
	{	
		ss=svdSLDinH[ii];
		QTableWidgetItem *_item2=new QTableWidgetItem();
		_item2->setText ( ss );
		ui.tableNsvd->setItem(ii,1,_item2);
	}
//	tableNsvd->setText(ii,1,ss);
	if (ii < svdSLDinD.count())
	{
		ss=svdSLDinD[ii];
		QTableWidgetItem *_item3=new QTableWidgetItem();
		_item3->setText ( ss );
		ui.tableNsvd->setItem(ii,2,_item3);
	}
//	tableNsvd->setText(ii,2,ss); 
    }

    settings.endGroup();    
    
    


}

ApplicationWindow *app(QWidget* widget)
{
	ApplicationWindow *appM;
	// to QtiPlot
    	QDockWidget *docWin=(QDockWidget*)widget->parent();
    
    	if ( docWin->isFloating() )
		appM = (ApplicationWindow *)docWin->parent()->parent();
    	else 
		appM = (ApplicationWindow *)docWin->parent();


	return appM;
}

// *******************************************
// +++ Text-To-Results Log-Window 
// *******************************************

void toResLogAll( QString interfaceName, QString text, QWidget* widget)
{
    QDateTime dt = QDateTime::currentDateTime();
    QString info	=dt.toString("dd.MM | hh:mm ->>" + interfaceName+">> ") ;
    info+=text;
    info+="\n";
    
    app(widget)->results->setText(app(widget)->results->toPlainText()+text);
}


void svd::on_spinBoxMsvd_valueChanged(int value)
{
ui.tableMsvd->setRowCount(value);

}

void svd::on_spinBoxNsvd_valueChanged(int value)
{
ui.tableNsvd->setRowCount(value);
}

bool svd::checkTableExistence(QString &tableName)
{
	int i;
	bool exist=false;

	QList<MdiSubWindow *> tableList=app(this)->tableList();

	for (i=0;i<(int)tableList.count();i++)  if (tableList.at(i) && tableList.at(i)->name()==tableName) exist=true;

	return exist;
}

// °°°°°°°°°°°°°°°°°°°°    READ-SVD-DATA-FROM-TABLE        °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
void svd::slotReadSvdData(QString tableName)
{
	Table *t;
	QString ss;
	int tt,ii;
	
	//°°°
	QList<MdiSubWindow *> tables=app(this)->tableList();
	
	//°°°
	for (tt=0; tt < (int)tables.count(); tt++)
	{
		ss=tables.at(tt)->name();
		//°°°
		if (ss==tableName)
		{
			t=(Table *)tables.at(tt);
			
			int nn=0;

			while(t->text(nn,0)!="") nn++;
			
			ui.spinBoxNsvd->setValue(nn);
			
			for(ii=0; ii<nn;ii++ )
			{
				QTableWidgetItem *_item1=new QTableWidgetItem();
				_item1->setText ( t->text(ii,0) );
				ui.tableNsvd->setItem(ii,0,_item1);
				//
				QTableWidgetItem *_item2=new QTableWidgetItem();
				_item2->setText ( t->text(ii,1) );
				ui.tableNsvd->setItem(ii,1,_item2);
				//
				QTableWidgetItem *_item3=new QTableWidgetItem();
				_item3->setText ( t->text(ii,2) );
				ui.tableNsvd->setItem(ii,2,_item3);

				// tableNsvd->setText(ii,0,t->text(ii,0));
				// tableNsvd->setText(ii,1,t->text(ii,1));
				// tableNsvd->setText(ii,2,t->text(ii,2));
			}
			
			nn=0;
			while(t->text(nn,3)!="") nn++;
			
			ui.spinBoxMsvd->setValue(nn);
			
			for(ii=0; ii<nn;ii++ )
			{
				QTableWidgetItem *_item1=new QTableWidgetItem();
				_item1->setText ( t->text(ii,4) );
				ui.tableMsvd->setItem(ii,0,_item1);
				//
				QTableWidgetItem *_item2=new QTableWidgetItem();
				_item2->setText ( t->text(ii,5) );
				ui.tableMsvd->setItem(ii,1,_item2);
				//
				QTableWidgetItem *_item3=new QTableWidgetItem();
				_item3->setText ( t->text(ii,3) );
				ui.tableMsvd->setItem(ii,2,_item3);

				// tableMsvd->setText(ii,0,t->text(ii,4));
				// tableMsvd->setText(ii,1,t->text(ii,5));
				// tableMsvd->setText(ii,2,t->text(ii,3));
			}
			
		}
		
	}
}


//°°°°°   Data Transfer                   °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
void svd::slotSVD1dataTransfer  (QString Name, int Nsvd, QStringList TexT, gsl_matrix* SLDcomponent, int Msvd,
								 QStringList Dcomposition,QStringList FileNames,gsl_vector* SLDsolvent, int Ksvd)
{	
	//Temp variables
	int mm,nn,ll,kk,j,KN, coln;
	double gslTemp, rhoHD, rho1, rho2, Bk, error;
	QString ss;
	
	//definition of variables...
	int M=Msvd;               			    	 //Number of H/D Concentrations
	int N; N=Nsvd*(Nsvd-1)/2+Nsvd; 		//Number of Partial structure factors
	int K=Ksvd; 		  	   				//Number of Q-points
	//
	int NM; if (Nsvd>Msvd) NM=Nsvd; else NM=Msvd; if (K>NM) NM=K;  //Table Dimension
	//
	bool TableName; //In-use when check existence of Tables 
	//
	QList<MdiSubWindow *> tables=app(this)->tableList();    //List of existing tables
	//
	Table *  t;     //Table-Pointer to Transfer Data
	
	//A-matrix SVD-on
	gsl_matrix * A = gsl_matrix_calloc (M,N);   //Contrast-matrix A
	gsl_matrix * U = gsl_matrix_calloc (M,N);   //A=USV^T
	gsl_matrix * V = gsl_matrix_calloc (N,N);   //
	gsl_vector * S = gsl_vector_calloc (N);     //Diagonal of S-matrix
	gsl_vector * work = gsl_vector_calloc (N);  //temp...
	//
	gsl_matrix * Idata = gsl_matrix_calloc(K,M);    //Matrix of Scattering Intensity
	gsl_matrix *dIdata = gsl_matrix_calloc(K,M);    //Matrix of errorbars of Scattering Intensity
	gsl_vector * b = gsl_vector_calloc (M);         //b=Idata(0..K,M)
	gsl_vector * x = gsl_vector_calloc (N);         //
	//
	Table* tableRho=app(this)->newTable(Name,NM,6+Nsvd+N+N+N+N+1+M+M+N+M+M); // Make Result Table
	
	//Col-Names
	tableRho->setColName(0,"Comment");          tableRho->setColPlotDesignation( 0,Table::None);
	tableRho->setColName(1,"hSLDcomponent");    tableRho->setColPlotDesignation( 1,Table::None);
	tableRho->setColName(2,"dSLDcomponent");    tableRho->setColPlotDesignation( 2,Table::None);
	tableRho->setColName(3,"TableName");        tableRho->setColPlotDesignation( 3,Table::None);
	tableRho->setColName(4,"HDcomposition");    tableRho->setColPlotDesignation( 4,Table::X);
	tableRho->setColName(5,"SLDsolvent");       tableRho->setColPlotDesignation( 5,Table::Y);
	
	
	//Transfer of Data: from SLDcomponent-Matrix and TexT-QStringList to tableRho
	for (nn=0; nn<Nsvd; nn++)
	{ 
		tableRho->setText(nn,0,TexT[nn]);
		gslTemp=gsl_matrix_get(SLDcomponent,nn,0); ss=ss.setNum(gslTemp); tableRho->setText(nn,1,ss);
		gslTemp=gsl_matrix_get(SLDcomponent,nn,1); ss=ss.setNum(gslTemp); tableRho->setText(nn,2,ss);
	};
	
	//Transfer of Data: from Dcomposition-QStringList, SLDsolvent-Vector and FileNames-QStringList to tableRho
	for (mm=0; mm<Msvd; mm++) 
	{   
		tableRho->setText(mm,3,FileNames[mm]);
		tableRho->setText(mm,4,Dcomposition[mm]);
		gslTemp=gsl_vector_get(SLDsolvent,mm); ss=ss.setNum(gslTemp); tableRho->setText(mm,5,ss);
		
	};
	
	coln=6; //number of active column
	
	//Calculation of Real SLD vs. DHcomposition
	for (nn=0; nn<Nsvd;nn++)
	{
		ss="SLD"; ss+=TexT[nn];
		tableRho->setColName(coln,ss);
		
		for(mm=0; mm<Msvd;mm++)
		{
			ss=Dcomposition[mm];
			gslTemp=ss.toDouble();
			
			rhoHD=gsl_matrix_get(SLDcomponent,nn,0)+gslTemp*(gsl_matrix_get(SLDcomponent,nn,1)-gsl_matrix_get(SLDcomponent,nn,0));
			ss=ss.setNum(rhoHD);
			tableRho->setText(mm,coln,ss);
		}
		coln++;
	}
	
	
	KN=coln;
	//Making Rho-matrix 
	for (nn=1; nn<(Nsvd+1);nn++) for (mm=nn; mm<(Nsvd+1);mm++)
	{
		ss=ss.setNum(10*nn+mm);                     //Indexing. For-example 11,12,13,22,23,33.
		tableRho->setColName(coln,ss.prepend('a')); //Set col-names like a11,a12,a13...
		
		for (ll=0; ll<Msvd; ll++)
		{
			ss=tableRho->text(ll,6+nn-1);   rho1=ss.toDouble();
			ss=tableRho->text(ll,6+mm-1);   rho2=ss.toDouble();
			ss=tableRho->text(ll,5);            rhoHD=ss.toDouble();
			
			gslTemp=(rho1-rhoHD)*(rho2-rhoHD);
			if (nn!=mm) gslTemp*=2;
			ss=ss.setNum(gslTemp);
			tableRho->setText(ll,coln,ss);
			gsl_matrix_set(A,ll,coln-KN,gslTemp);
		}
		coln++;
	}
	
	//
	gsl_matrix_memcpy(U,A);// A initial matrix and U result of SVD
	
	gsl_set_error_handler_off(); // GSL- error handler is of !!!!
	
	//SVD of A to U V and S
	int status=gsl_linalg_SV_decomp(U,V,S,work);
	
	// Handling of errors in GSL-functions by hand :)
	if (status) if (status != 0) 
	{
		ss=ss.setNum(status);
		QMessageBox::critical(this, tr("QtiSAS - Error"),
							  tr("<b> %1 </b>: GSL error - SVD").arg(ss));        
		goto NoTableLabel;
	}
	
	
	KN=coln; // current column number
	
	//Print of U-matrix to tableRho (u11,u12....)
	for (nn=1; nn<(Nsvd+1);nn++) for (mm=nn; mm<(Nsvd+1);mm++)
	{
		ss=ss.setNum(10*nn+mm); 
		tableRho->setColName(coln,ss.prepend('u')); 
		for (ll=0; ll<Msvd; ll++)
		{
			ss=ss.setNum(gsl_matrix_get(U,ll,coln-KN));
			tableRho->setText(ll,coln,ss);
		};
		coln++;
	};
	//
	KN=coln; // current column number
	
	//Print of V-matrix to tableRho (v11,v12....)
	for (nn=1; nn<(Nsvd+1);nn++)
		for (mm=nn; mm<(Nsvd+1);mm++)
		{
		ss=ss.setNum(10*nn+mm); 
		tableRho->setColName(coln,ss.prepend('V')); 
		for (ll=0; ll<N; ll++)
		{
			ss=ss.setNum(gsl_matrix_get(V,ll,coln-KN));
			tableRho->setText(ll,coln,ss);
		}
		coln++;
	};
	//
	tableRho->setColName(coln,"S");
	//Print of S-vector to tableRho (s1,s2...sN)
	for (ll=0; ll<N; ll++)
	{ 
		ss=ss.setNum(gsl_vector_get(S,ll));
		tableRho->setText(ll,coln,ss);
	};
	coln++;
	
	KN=coln; // current column number
	
	//Transfet data from individual tables to tableRho (I and Q); making MxN Idata matrix
	for (mm=0; mm<M; mm++)
	{
		ss=ss.setNum(mm+1);
		tableRho->setColName(coln+mm*3,'Q'+ss);
		tableRho->setColName(coln+mm*3+1,'I'+ss);
		tableRho->setColName(coln+mm*3+2,"dI"+ss);
		tableRho->setColPlotDesignation(coln+mm*3,Table::X); // Set first Q-column as X
		tableRho->setColPlotDesignation(coln+mm*3+2,Table::yErr); // Set first Q-column as dY
		ss=tableRho->text(mm,3);
		
		TableName=false;
		
		// checking of existance of individual tables
		for (j=0; j < (int)tables.count(); j++ ) if (tables.at(j)->name() == ss)
		{
			t=(Table *)tables.at(j);
			TableName=true;
		};
		if (!TableName)
		{
			QMessageBox::critical(this, tr("QtiSAS - Error"),
								  tr("<b> %1 </b>: Table Does Not Exist").arg(ss));
			
			goto NoTableLabel;
		}
		
		//
		for (kk=0; kk<K; kk++)
		{
			tableRho->setText(kk,coln+mm*3,t->text(kk,0));
			tableRho->setText(kk,coln+mm*3+1,t->text(kk,1));
			tableRho->setText(kk,coln+mm*3+2,t->text(kk,2));
			
			gsl_matrix_set(Idata,kk,mm,t->text(kk,1).toDouble());
			gsl_matrix_set(dIdata,kk,mm,t->text(kk,2).toDouble());
		}
	}
	
	
	
	coln+=3*M;
	
	//
	for (nn=1; nn<(Nsvd+1);nn++) for (mm=nn; mm<(Nsvd+1);mm++)
	{
		ss=ss.setNum(10*nn+mm); 
		tableRho->setColName(coln,ss.prepend('S')); 
		coln++;
		tableRho->setColName(coln,ss.prepend('d')); 
		tableRho->setColPlotDesignation(coln,Table::yErr);
		coln++;
	};  
	
	//°°°
	if (Nsvd==2)
	{
		tableRho->addColumns(6);
		tableRho->setColName(coln,"S13");
		tableRho->setColName(coln+1,"dS13");
		tableRho->setColPlotDesignation(coln+1,Table::yErr);
		
		tableRho->setColName(coln+2,"S23");
		tableRho->setColName(coln+3,"dS23");
		tableRho->setColPlotDesignation(coln+3,Table::yErr);
		
		tableRho->setColName(coln+4,"S33"); 
		tableRho->setColName(coln+5,"dS33");
		tableRho->setColPlotDesignation(coln+5,Table::yErr);
	}
	
	
	// °°° 
	coln=coln-2*N; 
	
	// °°°
	KN=coln;
	
	// °°°
	double S11, S12, S22, S13, S23, S33;
	
	// °°°
	for (kk=0; kk<K; kk++)
	{
		error=0;
		for (mm=0;mm<M;mm++)
		{
			gsl_vector_set(b,mm,gsl_matrix_get(Idata,kk,mm));   
			error+=gsl_matrix_get(dIdata,kk,mm)/gsl_matrix_get(Idata,kk,mm)*gsl_matrix_get(dIdata,kk,mm)/gsl_matrix_get(Idata,kk,mm);
		}
		error=sqrt(error);
		// °°°
		status=gsl_linalg_SV_solve(U,V,S,b,x);
		
		// °°°
		if (status) if (status != 0) 
		{
			ss=ss.setNum(status);
			QMessageBox::critical(this, tr("QtiSAS - Error"),
								  tr("<b> %1 </b>: GSL error - Solve").arg(ss));      
			goto NoTableLabel;
		}
		
		// °°°
		for (nn=0;nn<N;nn++)
		{	
			gslTemp=gsl_vector_get(x,nn);
			
			tableRho->setText(kk,coln+2*nn,QString::number(gslTemp));
			tableRho->setText(kk,coln+2*nn+1,QString::number(fabs(gslTemp*error)));
			if (Nsvd==2)
			{
				if (nn==0) S11=gslTemp;
				if (nn==1) S12=gslTemp;
				if (nn==2) S22=gslTemp;
			}
		}
		if (Nsvd==2)
		{
			S33=2*S12+S11+S22;
			S23=0.5*(S11-S22-S33);
			S13=0.5*(-S11+S22-S33);
			tableRho->setText(kk,coln+6,QString::number(S13));
			tableRho->setText(kk,coln+7,QString::number(fabs(S13*error)));
			tableRho->setText(kk,coln+8,QString::number(S23));
			tableRho->setText(kk,coln+9,QString::number(fabs(S23*error)));
			tableRho->setText(kk,coln+10,QString::number(S33));
			tableRho->setText(kk,coln+11,QString::number(fabs(S33*error)));			
		}
	}
	
	coln+=2*N; if (Nsvd==2) coln+=6;
	
	//
	for (mm=0; mm<M; mm++)
	{
		ss=ss.setNum(mm+1);
		tableRho->setColName(coln+mm,ss.prepend("Icon"));
	}
	
	KN=coln;
	
	for (mm=0;mm<Msvd;mm++)
	{
		for (kk=0;kk<Ksvd;kk++)
		{
			Bk=0;       
			for (nn=0;nn<N;nn++) 
			{
				ss=tableRho->text(kk,KN-N+nn);
				
				gslTemp=ss.toDouble();
				Bk+=gsl_matrix_get(A,mm,nn)*gslTemp;
				gslTemp=gsl_matrix_get(A,mm,nn);
			}
			ss=ss.setNum(Bk);
			tableRho->setText(kk,coln,ss);
		}
		coln++;
	}
	
	
	NoTableLabel: ;
}

//°°°°°°°°      Chech rhoHD    °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
void svd::slotCheckRhoHD()
{
	double change;
	change = ui.lineEditRhoH->text().toDouble();  ui.lineEditRhoH->setText( QString::number( change, 'e', 3 ) );
	change = ui.lineEditRhoD->text().toDouble();  ui.lineEditRhoD->setText( QString::number( change, 'e', 3 ) );   
}


//°°°°°°°°     SVD     °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
void svd::slotTableMsvdChanged( int raw, int col )
{
	slotCheckRhoHD();
	
	QString text="";
	if (ui.tableMsvd->item(raw,0)) text=ui.tableMsvd->item(raw,0)->text();
	double textD= text.toDouble();    
	if ((textD<0 || textD>1)) 	textD=0.0;

	ui.tableMsvd->item(raw,0)->setText(QString::number(textD, 'e', 3 ));
	if (col==0) 
	{
		double RhoH=ui.lineEditRhoH->text().toDouble();
		double RhoD=ui.lineEditRhoD->text().toDouble();

		if (!ui.tableMsvd->item(raw,1)) ui.tableMsvd->setItem(raw,1, new QTableWidgetItem() );
		ui.tableMsvd->item(raw,1)->setText ( QString::number(RhoH+textD*(RhoD-RhoH), 'e', 3)  );
	}
	else if (col==1) 
	{
		QTableWidgetItem *_item1=ui.tableMsvd->item(raw,1);
		text="";		
		if (_item1) text=_item1->text();
		textD= text.toDouble();    
		_item1->setText ( QString::number(textD, 'e', 3 ) );
	}
	else if (col==2) 
	{
		QTableWidgetItem *_item1=ui.tableMsvd->item(raw,2);
		text="";
		if (_item1) text=_item1->text();

		if (!checkTableExistence(text)) _item1->setBackground(QBrush (Qt::red));
		else _item1->setBackground(QBrush (Qt::green));
	}
}

//°°°°°°°°      Chech Table Name   °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
void svd::slotTabNameSVDcheck()
{
	QString checkExistence=ui.lineEditTabNameSVD->text();
	bool OK=checkTableExistence(checkExistence);
	QPalette palette;
	if (OK) {
        palette.setColor(ui.lineEditTabNameSVD->backgroundRole(), Qt::red);
        ui.lineEditTabNameSVD->setPalette(palette);
    }
	else {
        palette.setColor(ui.lineEditTabNameSVD->backgroundRole(), Qt::green);
        ui.lineEditTabNameSVD->setPalette(palette);
    }
}

//°°°°°°°°      Chech rhoHD    °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
void svd::slotSVD1dataTransfer()
{
	
	if (ui.lineEditTabNameSVD->palette().color(QPalette::Background) != Qt::green) {
		QMessageBox::warning(this, tr("QtiKWS - Table exist"),tr("Change Name"));
		return;
	}
	
	QString text;
	QStringList Dcomposition,FileNames, TexT;
	int mm,nn;
	int Nsvd=    ui.spinBoxNsvd->value();
	int Msvd=    ui.spinBoxMsvd->value();
	int Ksvd=    ui.spinBoxKsvd->value();
	gsl_vector * SLDsolvent        = gsl_vector_calloc(Msvd);
	gsl_matrix * SLDcomponent  = gsl_matrix_calloc(Nsvd,2);
	//
	for (mm=0; mm<Msvd; mm++) 
	{	
		text="";
		if (ui.tableMsvd->item(mm,1)) text=ui.tableMsvd->item(mm,1)->text(); gsl_vector_set(SLDsolvent, mm,  text.toDouble()); 
		text="";
		if (ui.tableMsvd->item(mm,0)) text=ui.tableMsvd->item(mm,0)->text(); Dcomposition.append(text);
		text="";
		if (ui.tableMsvd->item(mm,2)) text=ui.tableMsvd->item(mm,2)->text(); FileNames.append(text);
		// text= tableMsvd->text(mm,1) ; gsl_vector_set(SLDsolvent, mm,  text.toDouble()); 
		// Dcomposition.append(tableMsvd->text(mm,0));
		// FileNames.append(tableMsvd->text(mm,2));
	};
	//
	for (nn=0; nn<Nsvd; nn++)
	{ 
		text="";
		if (ui.tableNsvd->item(nn,1)) text=ui.tableNsvd->item(nn,1)->text(); gsl_matrix_set(SLDcomponent, nn, 0,  text.toDouble()); 
		text="";		
		if (ui.tableNsvd->item(nn,2)) text=ui.tableNsvd->item(nn,2)->text(); gsl_matrix_set(SLDcomponent, nn, 1,  text.toDouble()); 
		text="";		
		if (ui.tableNsvd->item(nn,0)) text=ui.tableNsvd->item(nn,0)->text(); TexT.append(text); 

		//text= tableNsvd->text(nn,1) ; gsl_matrix_set(SLDcomponent, nn, 0,  text.toDouble()); 
		//text= tableNsvd->text(nn,2) ; gsl_matrix_set(SLDcomponent, nn, 1,  text.toDouble()); 
		//TexT.append(tableNsvd->text(nn,0));
	};
	
	slotSVD1dataTransfer(ui.lineEditTabNameSVD->text(), Nsvd, TexT, SLDcomponent, Msvd, Dcomposition,FileNames, SLDsolvent, Ksvd);   
	
	// Set Table-Name as used after procedure
	
	QString checkExistence=ui.lineEditTabNameSVD->text();
    QPalette palette;
	if (checkTableExistence(checkExistence)) {
        palette.setColor(ui.lineEditTabNameSVD->backgroundRole(), Qt::red);
        ui.lineEditTabNameSVD->setPalette(palette);
    }
	else {
        palette.setColor(ui.lineEditTabNameSVD->backgroundRole(), Qt::green);
        ui.lineEditTabNameSVD->setPalette(palette);
    }
}

//°°°°°°°°      Chech Nsvd   °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
void svd::slotTableNsvdChanged( int row, int col )
{
	slotCheckRhoHD();

	if (col==1 || col==2) 
	{    
		QString text="";

		QTableWidgetItem *_item=ui.tableNsvd->item(row,col);

		if (_item) text=_item->text();
		
		double textD= text.toDouble();

		_item->setText ( QString::number(textD, 'e', 3 ) );
	}
	
}

//°°°°°°°°                        °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
void svd::slotReadDataFromTable()
{
	slotTabNameSVDcheck();
	if (ui.lineEditTabNameSVD->palette().color(QPalette::Background) == Qt::red) {
		slotReadSvdData(ui.lineEditTabNameSVD->text());
	}
	
}


//+++ Help
void svd::openHelpOnline( const QUrl &link)
{
    // to QtiPlot
    QDesktopServices::openUrl(link);

//    ui.textBrowserHelp->backward();
//    ui.textBrowserHelp->setSource(ui.textBrowserHelp->source());
  
}


void svd::forceReadSettings()
{
    if (ui.lineEditTabNameSVD->text()=="TEST1") readSettings();
}

void svd::setFont(QFont fonts)
{
//  ui.toolBox20->setFont(fonts);
    ui.lineEditTabNameSVD->setFont(fonts);
    ui.textLabelInfo->setFont(fonts);
    ui.textLabelInfoSAS->setFont(fonts);
	ui.textLabelInfoAuthor->setFont(fonts);
	ui.textLabelVersion->setFont(fonts);
    
    /*
	ui.toolBoxSVD->setFont(fonts);
	ui.textLabelInfo->setFont(fonts);
	ui.textLabelInfoSAS->setFont(fonts);
	ui.textLabelInfoVersion->setFont(fonts);
	ui.textLabelInfoAuthor->setFont(fonts);
	ui.tableMsvd->setFont(fonts);
	ui.textBrowserHelp->setFont(fonts);
     */
}
