/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2011 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Base class for import/export plugins
 ******************************************************************************/

#ifndef INTERFACES_H
#define INTERFACES_H

#include <QtPlugin>
#include <ApplicationWindow.h>

class QStringList;

class ImportExportPlugin
{
public:
	virtual ~ImportExportPlugin(){}

	virtual QStringList importFormats(){return QStringList();};
	virtual Table* import(const QString & /*fileName*/, int /*sheet*/ = -1){return 0;}

	virtual QStringList exportFormats() const {return QStringList();};
	virtual bool exportTable(Table *, const QString& /*fname*/, bool /*withLabels*/, bool /*exportComments*/, bool /*exportSelection*/){return false;}
	virtual bool exportMatrix(Matrix *, const QString& /*fname*/, bool /*exportSelection*/){return false;}
	virtual bool exportGraph(Graph *, const QString& /*fname*/, const QSizeF& /*customSize*/, int /*unit*/, double /*fontsFactor*/ = 1){return false;}
	virtual bool exportMultiLayerPlot(MultiLayer *, const QString& /*fname*/, const QSizeF& /*customSize*/, int /*unit*/, double /*fontsFactor*/ = 1){return false;}

	void setApplicationWindow(ApplicationWindow *app){d_app = app;};
	ApplicationWindow *applicationWindow(){return d_app;};

private:
	ApplicationWindow *d_app;
};

Q_DECLARE_INTERFACE(ImportExportPlugin, "com.ProIndependent.QtiPlot.ImportExportPlugin/1.0")
#endif
