/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A wizard type dialog to create new plots
 ******************************************************************************/

#include <QApplication>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>

#include "ApplicationWindow.h"
#include "ErrorBarsCurve.h"
#include "Graph.h"
#include "Graph3D.h"
#include "MultiLayer.h"
#include "PlotWizard.h"
#include "Table.h"


PlotWizard::PlotWizard( QWidget* parent, Qt::WindowFlags fl )
: QDialog( parent, fl )
{
	setWindowTitle( tr("QtiSAS - Select Columns to Plot") );
	setAttribute(Qt::WA_DeleteOnClose);
	setSizeGripEnabled( true );

	// top part starts here
	groupBox1 = new QGroupBox();

    QGridLayout *gl1 = new QGridLayout();
	buttonX = new QPushButton("<->" + tr("&X"));
	buttonX->setAutoDefault( false );
	gl1->addWidget( buttonX, 0, 0);

	buttonXErr = new QPushButton("<->" + tr("x&Err"));
	buttonXErr->setAutoDefault( false );
	gl1->addWidget( buttonXErr, 0, 1);

	buttonY = new QPushButton("<->" + tr("&Y"));
	buttonY->setAutoDefault( false );
	gl1->addWidget( buttonY, 1, 0);

	buttonYErr = new QPushButton("<->" + tr("yE&rr"));
	buttonYErr->setAutoDefault( false );
	gl1->addWidget( buttonYErr, 1, 1);

	buttonZ = new QPushButton("<->" + tr("&Z"));
	buttonZ->setAutoDefault( false );
	gl1->addWidget( buttonZ, 2, 0);
    gl1->setRowStretch(3,1);

    QHBoxLayout *hl2 = new QHBoxLayout();
    buttonNew = new QPushButton(tr("&New curve"));
    buttonNew->setDefault( true );
    buttonNew->setAutoDefault( true );
	hl2->addWidget(buttonNew);

	buttonDelete = new QPushButton(tr("&Delete curve"));
    buttonDelete->setAutoDefault( false );
	hl2->addWidget(buttonDelete);

    QVBoxLayout *vl = new QVBoxLayout();
    vl->addLayout(gl1);
    vl->addStretch();
    vl->addLayout(hl2);

    QGridLayout *gl2 = new QGridLayout(groupBox1);
    gl2->addWidget(new QLabel(tr( "Worksheet" )), 0, 0);
    boxTables = new QComboBox();
    gl2->addWidget(boxTables, 0, 1);
    columnsList = new QListWidget();
    gl2->addWidget(columnsList, 1, 0);
    gl2->addLayout(vl, 1, 1);

	// middle part is only one widget
	plotAssociations = new QListWidget();

	// bottom part starts here
	QHBoxLayout * bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();

	buttonOk = new QPushButton(tr("&Plot"));
    buttonOk->setAutoDefault( false );
	bottomLayout->addWidget( buttonOk );

	buttonCancel = new QPushButton(tr("&Close"));
    buttonCancel->setAutoDefault( false );
	bottomLayout->addWidget( buttonCancel );

	QVBoxLayout* vlayout = new QVBoxLayout( this );
	vlayout->addWidget( groupBox1 );
	vlayout->addWidget( plotAssociations );
	vlayout->addLayout( bottomLayout );

	// signals and slots connections
    connect(boxTables, &QComboBox::textActivated, this, &PlotWizard::changeColumnsList);
	connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
	connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
	connect( buttonNew, SIGNAL( clicked() ), this, SLOT( addCurve() ) );
	connect( buttonDelete, SIGNAL( clicked() ), this, SLOT( removeCurve() ) );
	connect( buttonX, SIGNAL( clicked() ), this, SLOT(addXCol()));
	connect( buttonY, SIGNAL( clicked() ), this, SLOT(addYCol()));
	connect( buttonXErr, SIGNAL( clicked() ), this, SLOT(addXErrCol()));
	connect( buttonYErr, SIGNAL( clicked() ), this, SLOT(addYErrCol()));
	connect( buttonZ, SIGNAL( clicked() ), this, SLOT(addZCol()));
}

QSize PlotWizard::sizeHint() const
{
	return QSize(350, 400);
}

void PlotWizard::accept()
{
	QStringList curves, curves3D, ribbons;
	for (int i=0; i < plotAssociations->count(); i++){
		QString text = plotAssociations->item(i)->text();
		if (text.endsWith("(X)")){
			QMessageBox::critical(this, tr("QtiSAS - Error"),
			tr("Please define a Y column for the following curve") + ":\n\n" + text);
			return;
		}

		if ( text.contains("(Z)") ){
			if ( text.contains("(Y)") && !curves3D.contains(text) )
				curves3D << text;
			else if ( !text.contains("(Y)") && !ribbons.contains(text) )
				ribbons << text;
		} else if ( text.contains("(xErr)") || text.contains("(yErr)")){
            QStringList lst = text.split(",", Qt::SkipEmptyParts);
			lst.pop_back();
			QString master_curve = lst.join(",");
			if (!curves.contains(master_curve))
				curves.prepend(master_curve);
			if (!curves.contains(text))
				curves << text; //add error bars at the end of the list.
		} else
			curves.prepend(text);
	}

	if (curves.count()>0)
		plot2D(curves);

	if (curves3D.count()>0)
		plot3D(curves3D);

	if (ribbons.count()>0)
		plot3DRibbon(ribbons);

	if(!noCurves())
		close();
}

void PlotWizard::changeColumnsList(const QString &table)
{
	QStringList newList;

	for( int i=0 ; i<columns.count() ; i++)
	{
		QString s = columns[i];
		if ( s.contains(table) )
			newList << s.remove(table+"_");
	}
	setColumnsListBoxContents(newList);
}

void PlotWizard::addXCol()
{
	if (noCurves())
		return;

	QString text = plotAssociations->currentItem()->text();
	if ( text.contains("(X)") )
		QMessageBox::warning(this, tr("QtiSAS - Error"), tr("You have already defined a X column!"));
	else
	{
		plotAssociations->currentItem()->setText(text+columnsList->currentItem()->text()+"(X)");
	}
}

void PlotWizard::addYCol()
{
	if (noCurves())
		return;

	QString text = plotAssociations->currentItem()->text();
	if ( !text.contains("(X)") )
		QMessageBox::warning(this, tr("QtiSAS - Error"),tr("You must define a X column first!"));
	else if ( text.contains("(Y)") )
		QMessageBox::warning(this, tr("QtiSAS - Error"), tr("You have already defined a Y column!"));
	else
	{
		plotAssociations->currentItem()->setText(text+", "+columnsList->currentItem()->text()+"(Y)");
	}
}

void PlotWizard::addZCol()
{
	if (noCurves())
		return;

	QString text = plotAssociations->currentItem()->text();
	if ( text.contains("(xErr)") || text.contains("(yErr)") )
		QMessageBox::warning(this, tr("QtiSAS - Error"), tr("This kind of curve is not handled by QtiSAS!"));
	else if ( !text.contains("(X)") )
		QMessageBox::warning(this, tr("QtiSAS - Error"), tr("You must define a X column first!"));
	else if ( text.contains("(Z)") )
		QMessageBox::warning(this, tr("QtiSAS - Error"), tr("You have already defined a Z column!"));
	else
	{
		plotAssociations->currentItem()->setText(text+", "+columnsList->currentItem()->text()+"(Z)");
	}
}

void PlotWizard::addXErrCol()
{
	if (noCurves())
		return;

	QString text = plotAssociations->currentItem()->text();
	if ( text.contains("(Z)") )
		QMessageBox::warning(this, tr("QtiSAS - Error"), tr("This kind of curve is not handled by QtiSAS!"));
	else if ( !text.contains("(X)") )
		QMessageBox::warning(this, tr("QtiSAS - Error"), tr("You must define a X column first!"));
	else if ( !text.contains("(Y)") )
		QMessageBox::warning(this, tr("QtiSAS - Error"), tr("You must define a Y column first!"));
	else if ( text.contains("(xErr)") || text.contains("(yErr)") )
		QMessageBox::warning(this, tr("QtiSAS - Error"), tr("You have already defined an error-bars column!"));
	else
	{
		plotAssociations->currentItem()->setText(text+", "+columnsList->currentItem()->text()+"(xErr)");
	}
}

void PlotWizard::addYErrCol()
{
	if (noCurves())
		return;

	QString text = plotAssociations->currentItem()->text();
	if ( text.contains("(Z)") )
		QMessageBox::warning(this, tr("QtiSAS - Error"), tr("This kind of curve is not handled by QtiSAS!"));
	else if ( !text.contains("(X)") )
		QMessageBox::warning(this, tr("QtiSAS - Error"), tr("You must define a X column first!"));
	else if ( !text.contains("(Y)") )
		QMessageBox::warning(this, tr("QtiSAS - Error"), tr("You must define a Y column first!"));
	else if ( text.contains("(xErr)") || text.contains("(yErr)") )
		QMessageBox::warning(this, tr("QtiSAS - Error"), tr("You have already defined an error-bars column!"));
	else
	{
		plotAssociations->currentItem()->setText(text+", "+columnsList->currentItem()->text()+"(yErr)");
	}
}

void PlotWizard::addCurve()
{
	plotAssociations->addItem( boxTables->currentText() + ": " );
	plotAssociations->setCurrentRow( plotAssociations->count()-1 );
}

void PlotWizard::removeCurve()
{
	plotAssociations->takeItem( plotAssociations->currentRow() );
}

void PlotWizard::insertTablesList(const QStringList& tables)
{
	boxTables->addItems(tables);
}

void PlotWizard::setColumnsListBoxContents(const QStringList& cols)
{
	columnsList->clear();
	columnsList->insertItems(0, cols);
	columnsList->setCurrentRow(0);
}

void PlotWizard::setColumnsList(const QStringList& cols)
{
	columns = cols;
}

bool PlotWizard::noCurves()
{
	if ( plotAssociations->count() == 0 )
	{
		QMessageBox::warning(0, tr("QtiSAS- Error"), tr("You must add a new curve first!"));
		return true;
	}
	else
		return false;
}

void PlotWizard::plot3DRibbon(const QStringList& lst)
{
    ApplicationWindow *app = (ApplicationWindow *)this->parent();
    if (!app)
        return;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    for (int i=0; i< lst.count(); i++)
    {
        QString s = lst[i];
        int pos = s.indexOf(":", 0);
        QString table_name = s.left(pos) + "_";
        Table *t = app->table(table_name);
        if (t)
        {
            int posX = s.indexOf("(", pos);
            QString xColName = table_name + s.mid(pos+2, posX-pos-2);

            posX = s.indexOf(",", posX);
            int posY = s.indexOf("(", posX);
            QString yColName = table_name + s.mid(posX+2, posY-posX-2);

            Graph3D *g = app->newPlot3D();
            if (g) {
                g->addRibbon(t, xColName, yColName);
                g->setDataColorMap(app->d_3D_color_map);
                g->update();
            }
        }
    }
    QApplication::restoreOverrideCursor();
}

void PlotWizard::plot3D(const QStringList& lst)
{
    ApplicationWindow *app = (ApplicationWindow *)this->parent();
    if (!app)
        return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    for (int i=0; i< lst.count(); i++)
    {
        QString s = lst[i];
        int pos = s.indexOf(":", 0);
        QString table_name = s.left(pos) + "_";
        Table *t = app->table(table_name);
        if (t)
        {
            int posX = s.indexOf("(", pos);
            QString xColName = table_name + s.mid(pos+2, posX-pos-2);

            posX = s.indexOf(",", posX);
            int posY = s.indexOf("(", posX);
            QString yColName = table_name + s.mid(posX+2, posY-posX-2);

            posY = s.indexOf(",", posY);
            int posZ = s.indexOf("(", posY);
            QString zColName = table_name + s.mid(posY+2, posZ-posY-2);

            int xCol = t->colIndex(xColName);
            int yCol = t->colIndex(yColName);
            int zCol = t->colIndex(zColName);
            if (xCol >= 0 && yCol >= 0 && zCol >= 0) {
                Graph3D *g = app->newPlot3D();
                if (g)
                    g->addData(t, xCol, yCol, zCol, 1);
            }
        }
    }
	QApplication::restoreOverrideCursor();
}

void PlotWizard::plot2D(const QStringList& colList)
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	if (!app)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	MultiLayer* g = new MultiLayer(app);
	app->initMultilayerPlot(g, "");

	Graph *ag = g->activeLayer();
	app->setPreferences(ag);

	int curves = (int)colList.count();
	int errorBars = 0;
	for (int i = 0; i < curves; i++) {
		if (colList[i].contains("(yErr)") || colList[i].contains("(xErr)"))
			errorBars++;
	}

	for (int i = 0; i < curves; i++){
		QString s = colList[i];
		int pos = s.indexOf(":", 0);
		QString caption = s.left(pos) + "_";
		Table *w = (Table *)app->table(caption);

		int posX = s.indexOf("(X)", pos);
		QString xColName = caption + s.mid(pos+2, posX-pos-2);

		posX = s.indexOf(",", posX);
		int posY = s.indexOf("(Y)", posX);
		QString yColName = caption+s.mid(posX+2, posY-posX-2);

		PlotCurve *c = nullptr;
		if (s.contains("(yErr)") || s.contains("(xErr)")){
			posY = s.indexOf(",", posY);
			int posErr, errType;
			if (s.contains("(yErr)")){
				errType = ErrorBarsCurve::Vertical;
				posErr = s.indexOf("(yErr)", posY);
			} else {
				errType = ErrorBarsCurve::Horizontal;
				posErr = s.indexOf("(xErr)",posY);
			}

			QString errColName = caption+s.mid(posY+2, posErr-posY-2);
			c = (PlotCurve *)ag->addErrorBars(xColName, yColName, w, errColName, errType);
		} else
			c = (PlotCurve *)ag->insertCurve(w, xColName, yColName, app->defaultCurveStyle);

		CurveLayout cl = ag->initCurveLayout(app->defaultCurveStyle, curves - errorBars);
		cl.lWidth = app->defaultCurveLineWidth;
		cl.sSize = app->defaultSymbolSize;
		ag->updateCurveLayout(c, &cl);
	}
	ag->updatePlot();
	ag->updateAxesTitles();
	ag->newLegend();
	g->arrangeLayers(false, true);
	QApplication::restoreOverrideCursor();
}
