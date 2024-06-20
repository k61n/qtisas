/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Alex Kargovsky <kargovsky@yumr.phys.msu.su>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Table worksheet class
 ******************************************************************************/

#include <QMessageBox>
#include <QDateTime>
#include <QTextStream>
#include <QClipboard>
#include <QApplication>
#include <QPainter>
#include <QEvent>
#include <QLayout>
#include <QPrintDialog>
#include <QLocale>
#include <QShortcut>
#include <QProgressDialog>
#include <QFile>
#include <QRegion>
#include <QTextDocumentWriter>
#include <QTextTable>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QScreen>

#include <gsl/gsl_math.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_sort.h>
#include <gsl/gsl_heapsort.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

#include "Table.h"
#include "SortDialog.h"
#include "ImportASCIIDialog.h"
#include "muParserScript.h"
#include "ApplicationWindow.h"
#include "ImportExportPlugin.h"


Table::Table(ScriptingEnv *env, int r, int c, const QString& label, ApplicationWindow* parent, const QString& name, Qt::WindowFlags f)
: MdiSubWindow(label,parent,name,f), scripted(env)
{
	init(r,c);
    updateVerticalHeaderByFont(parent->tableTextFont,-1);
}

void Table::init(int rows, int cols)
{
	selectedCol=-1;
	d_saved_cells = 0;
	d_show_comments = false;
	d_numeric_precision = 13;

	d_table = new MyTable(rows, cols, this);
	d_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
	d_table->setCurrentCell(-1, -1);
    d_table->setDragDropMode(QAbstractItemView::DragOnly);
    
    //+++ qtisas
    QRect rec = QGuiApplication::primaryScreen()->availableGeometry();
    int recwidth=rec.width();
    colWidth=int(recwidth/15);
    for(int c=0;c<cols; c++) d_table->setColumnWidth(c, colWidth);
    //---

	connect(d_table->verticalHeader(), SIGNAL(sectionMoved(int, int, int)), this, SLOT(notifyChanges()));

	setFocusPolicy(Qt::StrongFocus);
	setFocus();

	for (int i=0; i<cols; i++) {
		commands << "";
		colTypes << Numeric;
		col_format << "0/16";
		comments << "";
		col_label << QString::number(i+1);
		col_plot_type << Y;
	}

	QHeaderView* head = d_table->horizontalHeader();
    head->setSectionResizeMode(QHeaderView::Interactive);
    head->installEventFilter(this);
    head->setMinimumWidth(200);// 40
    
	connect(head, SIGNAL(sectionResized(int, int, int)), this, SLOT(colWidthModified(int, int, int)));

	col_plot_type[0] = X;
	setHeaderColType();

    int w =  3*(d_table->horizontalHeader())->sectionSize(0);
	int h =rec.height()/3;// 15*(d_table->verticalHeader())->sectionSize(0);
	setGeometry(150, 150, w , h);

	d_table->verticalHeader()->installEventFilter(this);
    d_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    
	setWidget(d_table);

	QShortcut *accelTab = new QShortcut(QKeySequence(Qt::Key_Tab), this);
	connect(accelTab, SIGNAL(activated()), this, SLOT(moveCurrentCell()));
	QShortcut *accelAll = new QShortcut(QKeySequence(Qt::CTRL+Qt::Key_A), this);
	connect(accelAll, SIGNAL(activated()), this, SLOT(selectAllTable()));
	connect(d_table, SIGNAL(itemChanged(QTableWidgetItem *)), this, SLOT(cellEdited(QTableWidgetItem *)));

	setAutoUpdateValues(applicationWindow()->autoUpdateTableValues());

    //+++ 08.2019:
    //d_table->setFocusStyle(Q3Table::FollowStyle);
    d_table->setShowGrid(false);
    d_table->setShowGrid(true);
}

void Table::setAutoUpdateValues(bool on)
{
	if (on){
		connect(this, SIGNAL(modifiedData(Table *, const QString&)),
            	this, SLOT(updateValues(Table*, const QString&)));
	} else {
		disconnect(this, SIGNAL(modifiedData(Table *, const QString&)),
            	this, SLOT(updateValues(Table*, const QString&)));
	}
}

void Table::colWidthModified(int, int, int)
{
	emit modifiedWindow(this);
	setHeaderColType();
}

void Table::setBackgroundColor(const QColor& col)
{
    QPalette palette;
    palette.setColor(backgroundRole(), col);
	d_table->setPalette(palette);
}

void Table::setTextColor(const QColor& col)
{
    QPalette palette;
    palette.setColor(backgroundRole(), col);
	d_table->setPalette(palette);
}

void Table::setTextFont(const QFont& fnt)
{
	d_table->setFont (fnt);
    updateVerticalHeaderByFont(fnt,-1);
}

void Table::updateVerticalHeaderByFont(const QFont& fnt, int rrr)
{
    int delta=0;
#ifdef Q_OS_MACOS //! Highlighting of the header text
    if (QGuiApplication::primaryScreen()->availableGeometry().width() < 1700)
        delta = 8;
#endif
    
    CompatQFontMetrics fm(d_table->verticalHeader()->font());
    int lm = fm.horizontalAdvance(QString::number(10 * d_table->rowCount()));
    QMargins margins = d_table->contentsMargins();
    margins.setLeft(lm + 3 + delta);
    d_table->setContentsMargins(margins);

    if (rrr<0)
    {
        int rows=d_table->rowCount();
        for (int r = 0; r < rows; r++)
            d_table->setRowHeight(r, fm.height() + 6 + delta);
    }
    else
        d_table->setRowHeight(rrr, fm.height() + 6 + delta);

}

void Table::setHeaderColor(const QColor& col)
{
	QPalette palette = d_table->horizontalHeader()->palette ();
    palette.setColor (QPalette::ButtonText, col);
#ifdef Q_OS_MACOS //! Highlighting of the header text
    palette.setColor (QPalette::BrightText, col);
#endif
    d_table->horizontalHeader()->setPalette (palette);
}

void Table::setHeaderColorRows(const QColor& col)
{
    QPalette palette = d_table->verticalHeader()->palette ();
    palette.setColor (QPalette::ButtonText, col);
#ifdef Q_OS_MACOS //! Highlighting of the header text
    palette.setColor (QPalette::BrightText, col);
#endif
    d_table->verticalHeader()->setPalette (palette);
}

void Table::setHeaderFont(const QFont& fnt)
{
	d_table->horizontalHeader()->setFont (fnt);
    updateHorizontalHeaderByFont(fnt);
}

void Table::updateHorizontalHeaderByFont(const QFont& fnt)
{
    int delta=0;
#ifdef Q_OS_MACOS //! Highlighting of the header text
    delta=2;
    if (QGuiApplication::primaryScreen()->availableGeometry().width() < 1700)
        delta = 10;
#endif
    QFontMetrics fm(d_table->horizontalHeader()->font());
    QMargins margins = d_table->contentsMargins();
    if (d_show_comments) {
        margins.setTop(3 * fm.height() + 6 + delta);
        d_table->setContentsMargins(margins);
    }
    else {
        margins.setTop(fm.height() + 6 + delta);
        d_table->setContentsMargins(margins);
    }
}

void Table::exportPDF(const QString& fileName)
{
	print(fileName);
}

void Table::print()
{
    print(QString());
}

void Table::print(QPrinter *printer)
{
	if (!printer)
		return;

	QPainter p;
	if (!p.begin(printer))
		return;  // paint on printer

	QPaintDevice* metrics = p.device();
	int dpiy = metrics->logicalDpiY();
  	const int margin = static_cast<int>((1 / 2.54) * dpiy);  // 2 cm margins

	QHeaderView *hHeader = d_table->horizontalHeader();
	QHeaderView *vHeader = d_table->verticalHeader();

	int rows=d_table->rowCount();
	int cols=d_table->columnCount();
	int height=margin;
	int i,vertHeaderWidth=vHeader->width();
	int right = margin + vertHeaderWidth;

	// print header
	p.setFont(hHeader->font());
	QString header_label = d_table->model()->headerData(0, Qt::Horizontal).toString();
	QRect br = QRect();
	br = p.boundingRect(br, Qt::AlignCenter, header_label);
	p.drawLine(right, height, right, height + br.height());
	QRect tr(br);

	for (i = 0; i < cols; i++) {
		int w = columnWidth(i);
		tr.setTopLeft(QPoint(right, height));
		tr.setWidth(w);
		tr.setHeight(br.height());
		header_label = d_table->model()->headerData(i, Qt::Horizontal).toString();
		p.drawText(tr, Qt::AlignCenter, header_label);
		right += w;
		p.drawLine(right, height, right, height + tr.height());

	    if (right >= printer->width() - 2 * margin) break;
	}

	p.drawLine(margin + vertHeaderWidth, height, right - 1, height);  // first horizontal line
	height += tr.height();
	p.drawLine(margin, height, right - 1, height);

	// print table values
	for (i = 0; i < rows; i++) {
		right = margin;
		QString cell_text =
			d_table->model()->headerData(i, Qt::Vertical).toString() + "\t";
		tr = p.boundingRect(tr, Qt::AlignCenter, cell_text);
		p.drawLine(right, height, right, height + tr.height());

		br.setTopLeft(QPoint(right, height));
		br.setWidth(vertHeaderWidth);
		br.setHeight(tr.height());
		p.drawText(br, Qt::AlignCenter, cell_text);
		right += vertHeaderWidth;
		p.drawLine(right, height, right, height + tr.height());

		for (int j = 0; j < cols; j++) {
    		int w = columnWidth(j);
    		cell_text = text(i, j) + "\t";
    		tr = p.boundingRect(tr, Qt::AlignCenter, cell_text);
    		br.setTopLeft(QPoint(right, height));
    		br.setWidth(w);
    		br.setHeight(tr.height());
    		p.drawText(br, Qt::AlignCenter, cell_text);
    		right += w;
			p.drawLine(right, height, right, height + tr.height());

			if (right >= printer->width() - 2 * margin) break;
		}
		height += br.height();
		p.drawLine(margin, height, right - 1, height);

		if (height >= printer->height() - margin) {
    		printer->newPage();
    		height = margin;
    		p.drawLine(margin, height, right, height);
		}
  	}
	p.end();
}

void Table::print(const QString& fileName)
{
	QPrinter printer;
	printer.setColorMode (QPrinter::GrayScale);
	if (!fileName.isEmpty()){
	    printer.setCreator("QtiSAS");
	    printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(fileName);
	} else {
        QPrintDialog printDialog(&printer, applicationWindow());
        if (printDialog.exec() != QDialog::Accepted)
            return;
    }

	printer.setFullPage( true );
	print(&printer);
}


void Table::cellEdited(QTableWidgetItem *it)
{
	QString text = it->text().remove(QRegExp("\\s"));
    int col = it->column();
    int row = it->row();
	if (columnType(col) != Numeric || text.isEmpty()){
		emit modifiedData(this, colName(col));
		emit modifiedWindow(this);
		return;
	}

	char f;
	int precision;
  	columnNumericFormat(col, &f, &precision);
  	bool ok = true;
  	double res = locale().toDouble(text, &ok);
  	if (ok)
  		d_table->setText(row, col, locale().toString(res, f, precision));
  	else {
  		Script *script = scriptEnv->newScript(d_table->text(row, col),
            this, QString("<%1_%2_%3>").arg(objectName()).arg(row+1).arg(col+1));
  		connect(script, SIGNAL(error(const QString&,const QString&,int)), scriptEnv, SIGNAL(error(const QString&,const QString&,int)));

  		script->setInt(row+1, "i");
  		script->setInt(col+1, "j");
  		QVariant ret = script->eval();
  		if(ret.type()==QVariant::Int || ret.type()==QVariant::UInt || ret.type()==QVariant::LongLong || ret.type()==QVariant::ULongLong)
  			d_table->setText(row, col, ret.toString());
  		else if(ret.canConvert(QVariant::Double))
  			d_table->setText(row, col, locale().toString(ret.toDouble(), f, precision));
  		else
  			d_table->setText(row, col, "");
  	}

  	emit modifiedData(this, colName(col));
	emit modifiedWindow(this);
}

int Table::colX(int col)
{
	int i, xcol = -1;
	for(i = col - 1; i >= 0; i--){
		if (col_plot_type[i] == X)
			return i;
	}
	for(i = col + 1; i < d_table->columnCount(); i++) {
		if (col_plot_type[i] == X)
			return i;
	}
	return xcol;
}

int Table::colY(int col, int xCol, const QStringList &lst)
{
	int yCol = -1;
	int cols = numCols();
	if (!lst.isEmpty()){
		if (xCol >= 0){
			for(int i = col - 1; i >= 0; i--){
				if (col_plot_type[i] == Y && colX(i) == xCol && lst.contains(colName(i)))
					return i;
			}
			for(int i = col + 1; i < cols; i++){
				if (col_plot_type[i] == Y && colX(i) == xCol && lst.contains(colName(i)))
					return i;
			}
		} else {
			for(int i = col - 1; i >= 0; i--){
				if (col_plot_type[i] == Y && lst.contains(colName(i)))
					return i;
			}
			for(int i = col + 1; i < cols; i++){
				if (col_plot_type[i] == Y && lst.contains(colName(i)))
					return i;
			}
		}
	} else {
		if (xCol >= 0){
			for(int i = col - 1; i >= 0; i--){
				if (col_plot_type[i] == Y && colX(i) == xCol)
					return i;
			}
			for(int i = col + 1; i < cols; i++){
				if (col_plot_type[i] == Y && colX(i) == xCol)
					return i;
			}
		} else {
			for(int i = col - 1; i >= 0; i--){
				if (col_plot_type[i] == Y)
					return i;
			}
			for(int i = col + 1; i < cols; i++){
				if (col_plot_type[i] == Y)
					return i;
			}
		}
	}
	return yCol;
}

void Table::setPlotDesignation(PlotDesignation pd, bool rightColumns)
{
	if (rightColumns){
		 int cols = d_table->columnCount();
		 for (int i = selectedCol; i<cols; i++){
			col_plot_type[i] = pd;
			if (pd == Table::Label)
				colTypes[i] = Text;
		}
	} else {
		QStringList list = selectedColumns();
		for (int i = 0; i < list.count(); i++){
			int col = colIndex(list[i]);
			col_plot_type[col] = pd;
			if (pd == Table::Label)
				colTypes[col] = Text;
		}
	}

	setHeaderColType();
	emit modifiedWindow(this);
}

void Table::setColPlotDesignation(int col, PlotDesignation pd)
{
	if (col < 0 || col >= d_table->columnCount() || col_plot_type[col] == pd)
		return;

	col_plot_type[col] = pd;
}

void Table::columnNumericFormat(int col, int *f, int *precision)
{
    QStringList format = col_format[col].split("/");
	if (format.count() == 2){
		*f = format[0].toInt();
		*precision = format[1].toInt();
		if (*precision > 14)
			*precision = 14;
	} else {
		*f = 0;
		*precision = 14;
	}
}

void Table::columnNumericFormat(int col, char *f, int *precision)
{
    QStringList format = col_format[col].split("/");
	if (format.count() == 2){
		switch(format[0].toInt()){
			case 0:
			*f = 'g';
			break;

			case 1:
			*f = 'f';
			break;

			case 2:
			*f = 'e';
			break;
		}
		*precision = format[1].toInt();
		if (*precision > 14)
			*precision = 14;
	} else {
		*f = 'g';
		*precision = 14;
	}
}

int Table::columnWidth(int col)
{
	return d_table->columnWidth(col);
}

QStringList Table::columnWidths()
{
	QStringList widths;
	for (int i = 0; i < d_table->columnCount(); i++)
		widths << QString::number(d_table->columnWidth(i));

	return widths;
}

void Table::setColWidths(const QStringList& widths)
{
	for (int i=0; i<(int)widths.count(); i++)
		d_table->setColumnWidth(i, widths[i].toInt());
}

void Table::setColumnTypes(const QStringList& ctl)
{
	int n = qMin((int)ctl.count(), numCols());
	for (int i=0; i<n; i++){
		QStringList l = ctl[i].split(";");
		colTypes[i] = l[0].toInt();

		if ((int)l.count() == 2 && !l[1].isEmpty())
			col_format[i] = l[1];
		else
			col_format[i] = "0/6";
	}
}

void Table::clearCommands()
{
	int count = (int)commands.size();
	for (int i = 0; i < count; i++)
        commands[i] = QString();
}

void Table::setCommands(const QStringList& com)
{
	commands.clear();
	for(int i=0; i<(int)com.size() && i<numCols(); i++)
		commands << com[i].trimmed();
}

void Table::setCommand(int col, const QString& com)
{
	if(col<(int)commands.size())
		commands[col] = com.trimmed();
}

void Table::setCommands(const QString& com)
{
	QStringList lst = com.split("\t");
	lst.pop_front();
	setCommands(lst);
}

bool Table::calculate()
{
    MySelection sel = getSelection();
	bool success = true;
	for (int col = sel.leftColumn(); col <= sel.rightColumn(); col++)
		if (!calculate(col, sel.topRow(), sel.bottomRow()))
			success = false;
	return success;
}

bool Table::muParserCalculate(int col, int startRow, int endRow, bool notifyChanges)
{
	if (startRow < 0)
		startRow = 0;
	if (endRow >= numRows())
		resizeRows(endRow + 1);

	QString cmd = commands[col];
	int colType = colTypes[col];
	if (cmd.isEmpty() || colType == Text){
		for (int i = startRow; i <= endRow; i++)
			d_table->setText(i, col, cmd);
        if (notifyChanges)
            emit modifiedData(this, colName(col));
        emit modifiedWindow(this);
        return true;
	}

	if (cmd.count("\n") > 0){
        QString mess = tr("Multiline expressions take much more time to evaluate! Do you want to continue anyways?");
        if (QMessageBox::Yes != QMessageBox::warning(this, tr("QtiSAS") + " - " + tr("Warning"), mess,
                           QMessageBox::Yes, QMessageBox::Cancel))
			return false;
	}

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    muParserScript *mup = new muParserScript(scriptEnv, cmd, this,
                                             QString("<%1>").arg(colName(col)));
    double *r = mup->defineVariable("i");
    mup->defineVariable("j", (double)col);
    mup->defineVariable("sr", startRow + 1.0);
    mup->defineVariable("er", endRow + 1.0);

	if (!mup->compile()){
		QApplication::restoreOverrideCursor();
		return false;
	}

	setAutoUpdateValues(false);

    if (mup->codeLines() == 1){
		if (colType == Date || colType == Time){
			QString fmt = col_format[col];
			for (int i = startRow; i <= endRow; i++){
				*r = i + 1.0;
				double val = mup->evalSingleLine();
				if (gsl_finite(val))
					d_table->setText(i, col, dateTime(val).toString(fmt));
			}
		} else if (colType == Numeric){
            //+++
            //QLocale loc = locale();
            QLocale loc = QLocale::c();
            loc.setNumberOptions(QLocale::OmitGroupSeparator);
            //---
			int prec;
			char f;
			columnNumericFormat(col, &f, &prec);
			for (int i = startRow; i <= endRow; i++){
				*r = i + 1.0;
				d_table->setText(i, col, mup->evalSingleLineToString(loc, f, prec));
			}
		}
	} else {
		if (colType == Date || colType == Time){
			QString fmt = col_format[col];
			for (int i = startRow; i <= endRow; i++){
				*r = i + 1.0;
				QVariant ret = mup->eval();
				if (ret.type() == QVariant::Double){
					double val = ret.toDouble();
					if (gsl_finite(val))
						d_table->setText(i, col, dateTime(val).toString(fmt));
				} else {
					QApplication::restoreOverrideCursor();
					return false;
				}
			}
        } else if (colType == Numeric) {            //+++
            //QLocale loc = locale();
            QLocale loc = QLocale::c();
            loc.setNumberOptions(QLocale::OmitGroupSeparator);
            //---
			int prec;
			char f;
			columnNumericFormat(col, &f, &prec);

			for (int i = startRow; i <= endRow; i++) {
				*r = i + 1.0;
				QVariant ret = mup->eval();
				if (ret.type() == QVariant::Double)
					d_table->setText(i, col, loc.toString(ret.toDouble(), f, prec));
				else
                    if(ret.canConvert(QVariant::String))
					    d_table->setText(i, col, ret.toString());
                    else {
                        QApplication::restoreOverrideCursor();
                        return false;
                    }
			}
		}
	}
	if (notifyChanges)
        emit modifiedData(this, colName(col));
	emit modifiedWindow(this);

	setAutoUpdateValues(applicationWindow()->autoUpdateTableValues());
	QApplication::restoreOverrideCursor();
	return true;
}

bool Table::calculate(int col, int startRow, int endRow, bool forceMuParser, bool notifyChanges)
{
	if (col < 0 || col >= d_table->columnCount())
		return false;

    if (d_table->isColumnReadOnly(col)){
        QMessageBox::warning(this, tr("QtiSAS - Error"),
        tr("Column '%1' is read only!").arg(col_label[col]));
        return false;
    }

	if (QString(scriptEnv->objectName()) == "muParser" || forceMuParser)
		return muParserCalculate(col, startRow, endRow, notifyChanges);

	if (startRow < 0)
		startRow = 0;
	if (endRow >= numRows())
		resizeRows(endRow + 1);

	QString cmd = commands[col];
	if (cmd.isEmpty()){
		for (int i = startRow; i <= endRow; i++)
			d_table->setText(i, col, cmd);
		if (notifyChanges)
			emit modifiedData(this, colName(col));
		emit modifiedWindow(this);
		return true;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);

	Script *colscript = scriptEnv->newScript(cmd, this,  QString("<%1>").arg(colName(col)));
	connect(colscript, SIGNAL(error(const QString&,const QString&,int)), scriptEnv, SIGNAL(error(const QString&,const QString&,int)));
	connect(colscript, SIGNAL(print(const QString&)), scriptEnv, SIGNAL(print(const QString&)));

	if (!colscript->compile()){
		QApplication::restoreOverrideCursor();
		return false;
	}

	setAutoUpdateValues(false);

	colscript->setDouble(col + 1.0, "j");
	colscript->setDouble(startRow + 1.0, "sr");
	colscript->setDouble(endRow + 1.0, "er");

	int colType = colTypes[col];
	if (colType == Date || colType == Time){
		QString fmt = col_format[col];
		for (int i = startRow; i <= endRow; i++){
			colscript->setDouble(i + 1.0, "i");
			QVariant ret = colscript->eval();
			if (ret.type() == QVariant::Double){
				double val = ret.toDouble();
				if (gsl_finite(val))
					d_table->setText(i, col, dateTime(val).toString(fmt));
			} else {
				QApplication::restoreOverrideCursor();
				return false;
			}
		}
	} else if (colType == Numeric){
        //+++
        //QLocale loc = locale();
        QLocale loc = QLocale::c();
        loc.setNumberOptions(QLocale::OmitGroupSeparator);
        //---
		int prec;
		char f;
		columnNumericFormat(col, &f, &prec);

		for (int i = startRow; i <= endRow; i++){
			colscript->setDouble(i + 1.0, "i");
			QVariant ret = colscript->eval();
			if(ret.type() == QVariant::Double)
				d_table->setText(i, col, loc.toString(ret.toDouble(), f, prec));
			else if(ret.canConvert(QVariant::String))
				d_table->setText(i, col, ret.toString());
			else {
				QApplication::restoreOverrideCursor();
				return false;
			}
		}
	}

	if (notifyChanges)
		emit modifiedData(this, colName(col));
	emit modifiedWindow(this);

	setAutoUpdateValues(applicationWindow()->autoUpdateTableValues());

	QApplication::restoreOverrideCursor();
	return true;
}

Table* Table::extractData(const QString& name, const QString& condition, int startRow, int endRow)
{
	if (startRow < 0)
		startRow = 0;

	if (endRow < 0 || endRow >= numRows())
		endRow = numRows();

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	muParserScript *mup = new muParserScript(scriptEnv, condition, this,  QString("<%1>").arg(this->objectName()));
	double *r = mup->defineVariable("i");
	mup->defineVariable("sr", startRow + 1.0);
	mup->defineVariable("er", endRow + 1.0);

	if (!mup->compile()){
		QApplication::restoreOverrideCursor();
		return 0;
	}

	ApplicationWindow *app = applicationWindow();
	if (!app)
		return 0;

	int cols = d_table->columnCount();
	Table *dest = app->table(name);
	if (dest){
		dest->setNumCols(cols);
		dest->setNumRows(numRows());
	} else
		dest = app->newTable(numRows(), cols, name);

	if (!dest)
		return 0;

	int aux = 0;
	if (mup->codeLines() == 1){
		for (int i = startRow; i <= endRow; i++){
			*r = i + 1.0;
			if (mup->evalSingleLine()){
				for (int j = 0; j < cols; j++)
					dest->setText(aux, j, this->text(i, j));

				aux++;
			}
		}
	} else {
		QVariant ret;
		for (int i = startRow; i <= endRow; i++){
			*r = i + 1.0;
			ret = mup->eval();
			if (ret.type() == QVariant::Double && ret.toDouble()){
				for (int j = 0; j < cols; j++)
					dest->setText(aux, j, this->text(i, j));

				aux++;
			}
		}
	}

	if (aux)
		dest->setNumRows(aux);
	dest->copy(this, false);
	QApplication::restoreOverrideCursor();
	return dest;
}

void Table::updateValues(Table* t, const QString& columnName)
{
    if (!t || t != this)
        return;

    QString colLabel = columnName;
    colLabel.remove(this->objectName()).remove("_");

	int cols = numCols();
	int endRow = numRows() - 1;
    for (int i = 0; i < cols; i++){
		QString cmd = commands[i];
        if (cmd.isEmpty() || colTypes[i] != Numeric || !cmd.contains("\"" + colLabel + "\"") ||
			cmd.contains("\"" + col_label[i] + "\""))
            continue;

        calculate(i, 0, endRow, false, true);
	}
}

MySelection Table::getSelection()
{
    MySelection sel;
	if (d_table->selectedRanges().count() == 0) {
        d_table->setCurrentCell(d_table->currentRow(), d_table->currentColumn());
        MySelection currentRange(d_table->currentRow(), d_table->currentColumn(), d_table->currentRow(), d_table->currentColumn());
		d_table->setRangeSelected(currentRange, true);
	} else
        if (!d_table->currentSelection().isEmpty())
            sel = d_table->currentSelection();
        else
            sel = d_table->selectedRanges().at(0);
	return sel;
}

QString Table::saveColumnWidths()
{
        QString s="ColWidth\t";
        for (int i=0;i<d_table->columnCount();i++)
                s+=QString::number(d_table->columnWidth (i))+"\t";
        return s+"\n";
}

QString Table::saveColumnTypes()
{
        QString s="ColType";
        for (int i=0; i<d_table->columnCount(); i++)
                s += "\t"+QString::number(colTypes[i])+";"+col_format[i];
        return s+"\n";
}

QString Table::saveCommands()
{
        QString s="<com>\n";
        for (int col=0; col<numCols(); col++)
                if (!commands[col].isEmpty()){
                        s += "<col nr=\""+QString::number(col)+"\">\n";
                        s += commands[col];
                        s += "\n</col>\n";
                }
        s += "</com>\n";
        return s;
}

QString Table::saveComments()
{
	QString s = "Comments\t";
	for (int i = 0; i < d_table->columnCount(); i++){
		if (comments.count() > i)
			s += comments[i].replace("\n", " ").replace("\t", " ") + "\t";
		else
			s += "\t";
	}
	return s + "\n";
}

QString Table::saveHeader()
{
        QString s = "header";
        for (int j=0; j<d_table->columnCount(); j++){
                if (col_plot_type[j] == X)
                        s += "\t" + colLabel(j) + "[X]";
                else if (col_plot_type[j] == Y)
                        s += "\t" + colLabel(j) + "[Y]";
                else if (col_plot_type[j] == Z)
                        s += "\t" + colLabel(j) + "[Z]";
                else if (col_plot_type[j] == xErr)
                        s += "\t" + colLabel(j) + "[xEr]";
                else if (col_plot_type[j] == yErr)
                        s += "\t" + colLabel(j) + "[yEr]";
                else if (col_plot_type[j] == Label)
                        s += "\t" + colLabel(j) + "[L]";
                else
                        s += "\t" + colLabel(j);
        }
        return s += "\n";
}

QString Table::saveReadOnlyInfo()
{
        QString s = "ReadOnlyColumn";
        for (int i=0; i<d_table->columnCount(); i++)
                s += "\t" + QString::number(d_table->isColumnReadOnly(i));
        return s += "\n";
}

QString Table::saveHiddenColumnsInfo()
{
        QString s = "HiddenColumn";
        for (int i=0; i<d_table->columnCount(); i++)
                s += "\t" + QString::number(d_table->isColumnHidden(i));
        return s += "\n";
}

void Table::save(const QString& fn, const QString& geometry, bool saveAsTemplate)
{
	QFile f(fn);
	if (!f.isOpen()){
		if (!f.open(QIODevice::Append))
			return;
	}
	QTextStream t( &f );
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    t.setCodec("UTF-8");
#endif
	t << "<table>";
	if (saveAsTemplate){
	    t << "\t" + QString::number(d_table->rowCount()) + "\t";
        t << QString::number(d_table->columnCount()) + "\n";
	} else {
	    t << "\n" + QString(objectName()) + "\t";
        t << QString::number(d_table->rowCount()) + "\t";
        t << QString::number(d_table->columnCount()) + "\t";
        t << birthDate() + "\n";
	}
	t << geometry;
	t << saveHeader();
	t << saveColumnWidths();
	t << saveCommands();
	t << saveColumnTypes();
	t << saveReadOnlyInfo();
	t << saveHiddenColumnsInfo();
	t << saveComments();

	if (!saveAsTemplate){
        t << "WindowLabel\t" + windowLabel() + "\t" + QString::number(captionPolicy()) + "\n";
        t << "<data>\n";
		int cols = d_table->columnCount() - 1;
		int rows = d_table->rowCount();
		for (int i=0; i<rows; i++){
			if (!isEmptyRow(i)){
				t << QString::number(i) + "\t";
				for (int j=0; j<cols; j++){
			    	if (colTypes[j] == Numeric && !d_table->text(i, j).isEmpty())
						t << QString::number(cell(i, j), 'g', 14) + "\t";
					else
						t << d_table->text(i, j) + "\t";
				}
            	if (colTypes[cols] == Numeric && !d_table->text(i, cols).isEmpty())
					t << QString::number(cell(i, cols), 'g', 14) + "\n";
				else
					t << d_table->text(i, cols) + "\n";
			}
		}
		t << "</data>\n";
	}
	t << "</table>\n";
}

int Table::firstXCol()
{
	int xcol = -1;
	for (int j=0; j<d_table->columnCount(); j++)
	{
		if (col_plot_type[j] == X)
			return j;
	}
	return xcol;
}

QString Table::colLabel(int col)
{
	if (col < 0 || col >= d_table->columnCount())
		return {};

	return col_label[col];
}

QString Table::comment(int col)
{
	if (col < 0 || col >= d_table->columnCount())
		return {};

	return comments[col];
}

void Table::setColComment(int col, const QString& s)
{
	if (col < 0 || col >= d_table->columnCount())
		return;

	if (comments[col] == s)
		return;

	comments[col] = s;

	if (d_show_comments)
		setHeaderColType();

	emit modifiedWindow(this);
}

void Table::setColumnWidth(int width, bool allCols)
{
	int cols=d_table->columnCount();
	if (allCols)
	{
		for (int i=0;i<cols;i++)
			d_table->setColumnWidth (i, width);

		emit modifiedWindow(this);
	}
	else
	{
		if (d_table->columnWidth(selectedCol) == width)
			return;

		d_table->setColumnWidth (selectedCol, width);
		emit modifiedWindow(this);
	}
}

void Table::adjustColumnsWidth(bool selection)
{
	int cols = d_table->columnCount();

	bool commentsAreShown=commentsEnabled();

	showComments(false); 

	if (selection){
		for (int i = 0; i < cols; i++){
			if(d_table->isColumnSelected (i, true))
				d_table->resizeColumnToContents(i);
		}
	} else {
		for (int i = 0; i < cols; i++)
			d_table->resizeColumnToContents(i);
	}

	showComments(commentsAreShown);

	if (selection){
		for (int i = 0; i < cols; i++){
			if(d_table->isColumnSelected (i, true))
				d_table->resizeColumnToContents(i);
		}
	} else {
		for (int i = 0; i < cols; i++)
			d_table->resizeColumnToContents(i);
	}
		
	emit modifiedWindow(this);
}

void Table::setColumnWidth(int col, int width)
{

	if (d_table->columnWidth(col) == width)
		return;

	d_table->setColumnWidth (col, width);
	emit modifiedWindow(this);
}

QString Table::colName(int col)
{//returns the table name + horizontal header text
	if (col<0 || col >= col_label.count())
		return QString();

	return QString(this->objectName()) + "_" + col_label[col];
}

void Table::setColName(int col, const QString& text, bool enumerateRight, bool warn)
{
    if (text.isEmpty() || col<0 || col >= d_table->columnCount())
		return;

    if (col_label[col] == text && !enumerateRight)
        return;

	QString caption = objectName();
	QString oldLabel = col_label[col];
	int cols = col + 1;
	if (enumerateRight)
        cols = (int)d_table->columnCount();

    int n = 1;
	for (int i = col; i<cols; i++){
		QString newLabel = text;
		if (enumerateRight)
            newLabel += QString::number(n);

		if (col_label.contains(newLabel)){
			if (warn){
				QMessageBox::critical(0, tr("QtiSAS - Error"),
                    tr(QString("There is already a column called : <b>" + 
                    newLabel + "</b> in table <b>" + caption + 
                    "</b>!<p>Please choose another name!").toLocal8Bit().constData()));
			}
			return;
        }
		n++;
	}

    n = 1;
    caption += "_";
	for (int i = col; i<cols; i++){
		QString newLabel = text;
		if (enumerateRight)
            newLabel += QString::number(n);

        commands.replaceInStrings("\"" + col_label[i] + "\"", "\"" + newLabel + "\"");
		emit changedColHeader(caption + col_label[i], caption + newLabel);
        col_label[i] = newLabel;
		n++;
	}

    setHeaderColType();
	emit modifiedWindow(this);
}

QStringList Table::selectedColumns()
{
	QStringList names;
	for (int i=0; i<d_table->columnCount(); i++){
		if(d_table->isColumnSelected (i, true))
			names << QString(name()) + "_" + col_label[i];
	}
	return names;
}

QStringList Table::YColumns()
{
	QStringList names;
	for (int i = 0; i < d_table->columnCount(); i++){
		if(col_plot_type[i] == Y)
			names << QString(name()) + "_" + col_label[i];
	}
	return names;
}

QStringList Table::selectedYColumns()
{
	QStringList names;
	for (int i = 0; i < d_table->columnCount(); i++){
		if(d_table->isColumnSelected (i) && col_plot_type[i] == Y)
			names << QString(objectName()) + "_" + col_label[i];
  	}
  	return names;
}

QStringList Table::selectedErrColumns()
{
  	QStringList names;
  	for (int i=0;i<d_table->columnCount();i++){
  	    if(d_table->isColumnSelected (i, true) &&
  	       (col_plot_type[i] == yErr || col_plot_type[i] == xErr))
  	       	names<<QString(objectName())+"_"+col_label[i];
	}
  	return names;
}

QStringList Table::drawableColumnSelection()
{
  	QStringList names;
  	for (int i=0; i<d_table->columnCount(); i++){
	if(d_table->isColumnSelected (i) && col_plot_type[i] == Y)
		names << QString(objectName()) + "_" + col_label[i];
    }

  	for (int i=0; i<d_table->columnCount(); i++){
  	 	if(d_table->isColumnSelected (i) &&
			(col_plot_type[i] == yErr || col_plot_type[i] == xErr || col_plot_type[i] == Label))
  	    	names << QString(objectName()) + "_" + col_label[i];
  	}
	return names;
}

QStringList Table::selectedYLabels()
{
	QStringList names;
	for (int i=0;i<d_table->columnCount();i++){
		if(d_table->isColumnSelected (i) && col_plot_type[i] == Y)
			names<<col_label[i];
	}
	return names;
}

QStringList Table::columnsList()
{
	QStringList names;
	for (int i = 0; i < d_table->columnCount(); i++)
		names << QString(objectName()) + "_" + col_label[i];
	return names;
}

int Table::firstSelectedColumn()
{
	for (int i = 0; i < d_table->columnCount(); i++)
		if(d_table->isColumnSelected (i,true))
			return i;
	return -1;
}

int Table::numSelectedRows()
{
	int r=0;
	for (int i = 0; i < d_table->rowCount(); i++)
		if(d_table->isRowSelected (i,true))
			r++;
	return r;
}

int Table::selectedColsNumber()
{
	int c=0;
	for (int i=0;i<d_table->columnCount(); i++)
		if(d_table->isColumnSelected (i,true))
			c++;
	return c;
}

QVarLengthArray<double> Table::col(int c)
{
	if (c < 0 || c >= d_table->columnCount())
		return QVarLengthArray<double>();

	int rows = d_table->rowCount();
	QVarLengthArray<double> Y(rows);
	
    //+++
    //QLocale l = locale();
    QLocale l = QLocale::c();
    l.setNumberOptions(QLocale::OmitGroupSeparator);
    //---
    
	for (int i = 0; i<rows; i++)
		Y[i] = l.toDouble(d_table->text(i, c));
	return Y;
}

void Table::columnRange(int c, double *min, double *max)
{
	if (c < 0 || c >= d_table->columnCount())
		return;

	double d_min = 0.0;
	double d_max = 0.0;

    MySelection selection = getSelection();

    //+++
    //QLocale l = locale();
    QLocale l = QLocale::c();
    l.setNumberOptions(QLocale::OmitGroupSeparator);
    //---
    
	int startRow = selection.topRow();
	for (int i = selection.topRow(); i <= selection.bottomRow(); i++){
		QString s = d_table->text(i, c);
		if (!s.isEmpty()){
			d_min = l.toDouble(s);
			d_max = d_min;
			startRow = i;
			break;
		}
	}

	for (int i = startRow; i <= selection.bottomRow(); i++){
		QString s = d_table->text(i, c);
		if (!s.isEmpty()){
			double aux = l.toDouble(s);

			if (aux <= d_min)
				d_min = aux;

			if (aux >= d_max)
				d_max = aux;
		}
	}

	*min = d_min;
	*max = d_max;
}

void Table::insertCols(int start, int count)
{
    if (start < 0)
        start = 0;

	int max = 0;
	int cols = d_table->columnCount();
	QList<bool> hiddenCols;

	for (int i = 0; i<cols; i++){
		if (!col_label[i].contains(QRegExp ("\\D"))){
			int id = col_label[i].toInt();
			if (id > max)
				max = id;
		}
		hiddenCols << d_table->isColumnHidden(i);
	}
    max++;

    for (int i = 0; i < count; i++)
        d_table->insertColumn(start);


	for(int i = 0; i<count; i++ ){
        int j = start + i;
		commands.insert(j, QString());
		col_format.insert(j, "0/" + QString::number(d_numeric_precision));
		comments.insert(j, QString());
		col_label.insert(j, QString::number(max + i));
		colTypes.insert(j, Numeric);
		col_plot_type.insert(j, Y);
		hiddenCols.insert(j, false);
	}
	setHeaderColType();

	for (int i = 0; i<d_table->columnCount(); i++)
		hideColumn(i, hiddenCols[i]);

    
    for(int cc=0;cc<count; cc++) d_table->setColumnWidth(start+cc, colWidth);
	emit modifiedWindow(this);
}

void Table::insertCol()
{
	insertCols(selectedCol, 1);
}

void Table::insertRow()
{
	int cr = d_table->currentRow();
	if (d_table->isRowSelected (cr, true))
	{
		d_table->insertRow(cr);
        updateVerticalHeaderByFont(d_table->font(),cr);
		emit modifiedWindow(this);
	}
}

void Table::addCol(PlotDesignation pd)
{
	d_table->clearSelection();
	int index, max = 0, cols = d_table->columnCount();
	for (int i=0; i<cols; i++){
		if (!col_label[i].contains(QRegExp ("\\D"))){
			index = col_label[i].toInt();
			if (index > max)
				max = index;
		}
	}
	d_table->insertColumn(cols);

    QTableWidgetItem *it = d_table->item(0, cols);
    d_table->scrollToItem(it, QAbstractItemView::EnsureVisible);

	comments << QString();
	commands << "";
	colTypes << Numeric;
	col_format << "0/" + QString::number(d_numeric_precision);
	col_label << QString::number(max+1);
	col_plot_type << pd;

	setHeaderColType();
	emit addedCol(objectName() + "_" + QString::number(max+1));
	emit modifiedWindow(this);
}

void Table::addColumns(int c)
{
	int max = 0;
    int cols = d_table->columnCount();
	for (int i = 0; i < cols; i++) {
		if (!col_label[i].contains(QRegExp ("\\D"))){
			int index = col_label[i].toInt();
			if (index > max)
				max = index;
		}
	}
	max++;
    for (int i = 0; i < c; i++) {
	    d_table->insertColumn(d_table->columnCount());
        d_table->setColumnWidth(max + i, colWidth);
		comments << QString();
		commands << "";
		colTypes << Numeric;
		col_format << "0/" + QString::number(d_numeric_precision);
		QString label = QString::number(max + i);
		col_label << label;
		col_plot_type << Y;
		emit addedCol(objectName() + "_" + label);
	}
}

void Table::clearCol()
{
	if (d_table->isColumnReadOnly(selectedCol))
		return;
	for (int i = 0; i < d_table->rowCount(); i++)
		if (d_table->item(i, selectedCol)->isSelected())
			d_table->setText(i, selectedCol, "");
	emit modifiedData(this, colName(selectedCol));
}

void Table::clearCell(int row, int col)
{
	if (col < 0 || col >= d_table->columnCount())
        return;

	if (d_table->isColumnReadOnly(col)){
		QMessageBox::warning(this, tr("QtiSAS - Error"), tr("Column '%1' is read only!").arg(colName(col)));
        return;
	}

	d_table->setText(row, col, "");

	emit modifiedData(this, colName(col));
	emit modifiedWindow(this);
}

void Table::deleteSelectedRows()
{
	MySelection sel = d_table->selectedRanges().at(0);
	deleteRows(sel.topRow() + 1, sel.bottomRow() + 1);
}

void Table::deleteRows(int startRow, int endRow)
{
	for(int i=0; i<d_table->columnCount(); i++){
        if (d_table->isColumnReadOnly(i)){
			QMessageBox::warning(this, tr("QtiSAS - Error"),
        	tr("The table '%1' contains read-only columns! Operation aborted!").arg(objectName()));
			return;
		}
	}

    int start = qMin(startRow, endRow);
    int end = qMax(startRow, endRow);

    start--;
    end--;
    if (start < 0)
        start = 0;
    if (end >= d_table->rowCount())
        end = d_table->rowCount() - 1;

    int rows = abs(end - start) + 1;
    for (int i = 0; i < rows; i++)
        d_table->removeRow(end - i);
    notifyChanges();
}

void Table::cutSelection()
{
	copySelection();
	clearSelection();
}

void Table::selectAllTable()
{
    MySelection range(0, 0, d_table->rowCount() - 1, d_table->columnCount() - 1);
	d_table->setRangeSelected(range, true);
}

void Table::deselect()
{
	d_table->clearSelection();
}

void Table::clearSelection()
{
    QStringList list = selectedColumns();
    int n = int(list.count());

    if (n > 0)
    {
        for (int i = 0; i < n; i++)
        {
            selectedCol = colIndex(list[i]);
            if (d_table->isColumnReadOnly(selectedCol))
            {
                QMessageBox::warning(this, tr("QtiSAS - Warning"),
                                     tr("The folowing column") + ":\n" + colName(selectedCol) + "\n" +
                                         tr("are read only!"));
                continue;
            }
            for (int j = 0; j < d_table->rowCount(); j++)
                d_table->setText(j, selectedCol, "");
            emit modifiedData(this, colName(selectedCol));
        }
    }
    else
    {
        foreach (const MySelection &selection, d_table->selectedRanges())
        {
            int top = selection.topRow();
            int bottom = selection.bottomRow();
            int left = selection.leftColumn();
            int right = selection.rightColumn();

            for (int i = left; i <= right; i++)
            {
                if (d_table->isColumnReadOnly(i))
                {
                    QMessageBox::warning(this, tr("QtiSAS - Warning"),
                                         tr("The folowing column") + ":\n" + colName(i) + "\n" + tr("are read only!"));
                    continue;
                }
                for (int j = top; j <= bottom; j++)
                    d_table->setText(j, i, "");
                QString name = colName(i);
                emit modifiedData(this, name);
            }
        }
    }
    emit modifiedWindow(this);
}

void Table::copySelection()
{
	QString text;
	int rows = d_table->rowCount();
	int cols = d_table->columnCount();
	QString eol = applicationWindow()->endOfLine();

	QVector<int> selectedCols;
	for (int i = 0; i < cols; i++)
		if (d_table->isColumnSelected(i, true))
            selectedCols.append(i);
	if (selectedCols.count() > 0) {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < selectedCols.count() - 1; j++)
                text += d_table->text(i, selectedCols[j]) + "-\t-";
            text += d_table->text(i, selectedCols.last()) + "@eol@" + eol;
        }
	} else {
        MySelection sel;
        if (d_table->selectedRanges().count() > 0)
		     sel = d_table->selectedRanges().at(0);
		int right = sel.rightColumn();
		int bottom = sel.bottomRow();
		if (right < 0 || bottom < 0)
			text = d_table->text(d_table->currentRow(), d_table->currentColumn());
		else {
			for (int i = sel.topRow(); i<bottom; i++){
				for (int j = sel.leftColumn(); j<right; j++)
					text += d_table->text(i, j) + "-\t-";
				text += d_table->text(i, right) + "@eol@"+eol;
			}
			for (int j = sel.leftColumn(); j<right; j++)
					text += d_table->text(bottom, j) + "-\t-";
				text += d_table->text(bottom, right);
		}
	}
	QApplication::clipboard()->setText(text.trimmed().replace("-\t-","\t").remove("@eol@"));
}

// Paste text from the clipboard
void Table::pasteSelection()
{
	QString text = QApplication::clipboard()->text();
	if (text.isEmpty()) return;

	QString eol = ApplicationWindow::guessEndOfLine(text);
	if (text.endsWith(eol)) text.chop(eol.size());
    
	QStringList linesList = text.split(eol);
	int rows = linesList.size();
	if (rows < 1) return;

    QStringList firstLine = linesList[0].split("\t", Qt::SkipEmptyParts);
	int cols = firstLine.count();
	for (int i = 1; i < rows; i++)
    {
		int aux = linesList[i].split("\t").count();
		if (aux > cols) cols = aux;
	}

    //+++
    //QLocale l = locale();
    //QLocale clipboardLocale = applicationWindow()->clipboardLocale();
    QLocale l = QLocale::c();
    l.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale clipboardLocale = QLocale::c();
    clipboardLocale.setNumberOptions(QLocale::OmitGroupSeparator);
    //---
    
	bool allNumbers = true;
	bool pasteComments = false;
	bool pasteHeader = false;
	if (rows > 1 && applicationWindow() && applicationWindow()->d_show_table_paste_dialog)
    {
		for (int i = 0; i < firstLine.size(); i++){
		//verify if the strings in the line used to rename the columns are not all numbers
			clipboardLocale.toDouble(firstLine[i], &allNumbers);
			if (!allNumbers){
				QMessageBox msgBox(this);
				msgBox.setIconPixmap(QPixmap(":/qtisas_logo.png"));
				msgBox.setWindowTitle(tr("QtiSAS") + " - " + tr("Paste operation"));
				msgBox.setText(tr("How should QtiSAS interpret first clipboard line?"));
				msgBox.addButton(tr("&Values"), QMessageBox::AcceptRole);
				QPushButton *namesButton = msgBox.addButton(tr("Column &Names"), QMessageBox::AcceptRole);
				msgBox.setDefaultButton(namesButton);
				QAbstractButton *commentsButton = msgBox.addButton(tr("&Comments"), QMessageBox::AcceptRole);
				msgBox.addButton(QMessageBox::Cancel);

				if (msgBox.exec() == QMessageBox::Cancel)
					return;

				if (msgBox.clickedButton() == namesButton){
					pasteHeader = true;
					linesList.pop_front();
					rows--;
				} else if (msgBox.clickedButton() == commentsButton){
					pasteComments = true;
					linesList.pop_front();
					rows--;
				}

				break;
			}
		}
	}

	int top, left, firstCol = firstSelectedColumn();
    int c=selectedColsNumber();

    MySelection sel = d_table->selectedRanges().at(0);
	if (!sel.isEmpty()){// not columns but only cells are selected
		top = sel.topRow();
		left = sel.leftColumn();
	} else {
		top = d_table->currentRow();
		left = d_table->currentColumn();
		if (firstCol >= 0)// columns are selected
			left = firstCol;
	}

    //* 2021
    if (cols>1 && c==1 && columnType(left) == Table::Text)
    {
        for (int i = 0; i < linesList.count(); i++) linesList[i]=linesList[i].remove("\t");
        cols=1;
    }
    //-
    
    if (top + rows > d_table->rowCount())
        //d_table->setRowCount(top + rows);
        setNumRows(top + rows);
    if (left + cols > d_table->columnCount()){
        addColumns(left + cols - d_table->columnCount());
        setHeaderColType();
    }

	QStringList lstReadOnly;
	for (int i = left; i < left + cols; i++){
		QString name = colName(i);
		if (d_table->isColumnReadOnly(i))
			lstReadOnly << name;
	}
	if (lstReadOnly.count() > 0){
		QMessageBox::warning(this, tr("QtiSAS - Error"),
		tr("The folowing columns") + ":\n" + lstReadOnly.join("\n") + "\n" + tr("are read only!"));
    }

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QStringList colLabels;
	for (int i = 0; i < left && i < col_label.size(); i++)
		colLabels << col_label[i];
	for (int i = left + cols; i < col_label.size(); i++)
		colLabels << col_label[i];

	if (pasteComments || pasteHeader)
    {
		for (int j = left; j < left + cols; j++){
			if (d_table->isColumnReadOnly(j))
				continue;

			int colIndex = j - left;
			if (colIndex < 0 || colIndex >= firstLine.count())
				continue;

			if (pasteComments)
				comments[j] = firstLine[colIndex];
			else if (pasteHeader){
				QString colName = firstLine[colIndex].replace("-", "_").remove(QRegExp("\\W")).replace("_", "-");
				while(colLabels.contains(colName))
					colName += "2";
				if (!colLabels.contains(colName)){
					col_label[j] = colName;
					colLabels << colName;
				}
			}
		}
		if (pasteComments){
			if (d_show_comments)
				setHeaderColType();
			else
				showComments();
		} else if (pasteHeader)
			setHeaderColType();
	}
    
    
	for (int i = 0; i < rows; i++) {
		int row = top + i;
		QStringList cells = linesList[i].split("\t");

        for (int j = left; j < left + cols; j++) {
			if (d_table->isColumnReadOnly(j))continue;

            int colIndex = j-left;
            if (colIndex >= cells.count())
                break;

			bool numeric = false;
			double value = clipboardLocale.toDouble(cells[colIndex], &numeric);
            
            bool colNumetic=false;
            if (columnType(j) == Table::Numeric) colNumetic=true;

            QTableWidgetItem *it;
            if (numeric && colNumetic) {
			    int prec;
                char f;
				columnNumericFormat(j, &f, &prec);
				d_table->setText(row, j, l.toString(value, f, prec));
			} else
				d_table->setText(row, j, cells[colIndex]);
		}
	}

	for (int i = left; i< left + cols; i++)
		if (!d_table->isColumnReadOnly(i))
            emit modifiedData(this, colName(i));
	emit modifiedWindow(this);
	QApplication::restoreOverrideCursor();
}

void Table::removeCol()
{
	removeCol(selectedColumns());
}

void Table::removeCol(const QStringList& list)
{
	QStringList lstReadOnly;
	for (int i = 0; i < list.count(); i++){
		QString name = list[i];
		int col = colIndex(name);
		if (d_table->isColumnReadOnly(col))
			lstReadOnly << name;
	}

	if (lstReadOnly.count() > 0){
		QMessageBox::warning(this, tr("QtiSAS - Error"),
		tr("The folowing columns") + ":\n"+ lstReadOnly.join("\n") + "\n"+ tr("are read only!"));
    }

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	for (int i = 0; i < list.count(); i++){
		QString name = list[i];
		int id = colIndex(name);
		if (id >= 0){
			if (d_table->isColumnReadOnly(id))
				continue;

			if ( id < commands.size() )
				commands.removeAt(id);

			if ( id < col_label.size() )
				col_label.removeAt(id);

			if ( id < col_format.size() )
				col_format.removeAt(id);

			if ( id < comments.size() )
				comments.removeAt(id);

			if ( id < colTypes.size() )
				colTypes.removeAt(id);

			if ( id < col_plot_type.size() )
				col_plot_type.removeAt(id);

			d_table->removeColumn(id);
			emit removedCol(name);
			emit removedCol(id);
		}
	}
	emit modifiedWindow(this);
	QApplication::restoreOverrideCursor();
}

void Table::normalizeSelection()
{
	QStringList lst = writableSelectedColumns();
	if (lst.isEmpty())
		return;

	for (int i = 0; i < lst.count(); i++)
		normalizeCol(colIndex(lst[i]));

	emit modifiedWindow(this);
}

void Table::normalize()
{
	QStringList lstReadOnly;
	for (int i=0; i<d_table->columnCount(); i++){
		if (d_table->isColumnReadOnly(i))
			lstReadOnly << col_label[i];
	}

	if (lstReadOnly.count() > 0){
		QMessageBox::warning(this, tr("QtiSAS - Error"),
        tr("The folowing columns")+":\n"+ lstReadOnly.join("\n") + "\n"+ tr("are read only!"));
    }

	for (int i = 0; i < d_table->columnCount(); i++)
		normalizeCol(i);

	emit modifiedWindow(this);
}

void Table::normalizeCol(int col)
{
	if (col<0) col = selectedCol;
	if (d_table->isColumnReadOnly(col) || colTypes[col] == Table::Text)
	    return;

	int rows = d_table->rowCount();
	gsl_vector *data = gsl_vector_alloc(rows);
	for (int i=0; i<rows; i++)
		gsl_vector_set(data, i, cell(i, col));

	double max = gsl_vector_max(data);
	if (max == 1.0)
		return;

    int prec;
    char f;
    columnNumericFormat(col, &f, &prec);

	for (int i=0; i<rows; i++){
		if ( !d_table->text(i, col).isEmpty() )
			d_table->setText(i, col, locale().toString(gsl_vector_get(data, i)/max, f, prec));
	}

	gsl_vector_free(data);
	emit modifiedData(this, colName(col));
}

void Table::sortColumnsDialog()
{
	SortDialog *sortd = new SortDialog(applicationWindow());
	connect (sortd, SIGNAL(sort(int, int, const QString&)), this, SLOT(sortColumns(int, int, const QString&)));
	sortd->insertColumnsList(selectedColumns());
	sortd->exec();
}

void Table::sortTableDialog()
{
	SortDialog *sortd = new SortDialog(applicationWindow());
	connect (sortd, SIGNAL(sort(int, int, const QString&)), this, SLOT(sort(int, int, const QString&)));
	sortd->insertColumnsList(colNames());
	sortd->exec();
}

void Table::sort(int type, int order, const QString& leadCol)
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	sortColumns(col_label, type, order, leadCol);
    QApplication::restoreOverrideCursor();
}

void Table::sortColumns(int type, int order, const QString& leadCol)
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	sortColumns(selectedColumns(), type, order, leadCol);
    QApplication::restoreOverrideCursor();
}

int compare_qstrings (const void * a, const void * b)
{
	QString *bb = (QString *)b;
	return ((QString *)a)->compare(*bb);
}

void Table::sortColumns(const QStringList&s, int type, int order, const QString& leadCol)
{
	int cols = s.count();
	if(!type){//Sort columns separately
		for(int i = 0; i < cols; i++)
			sortColumn(colIndex(s[i]), order);
	} else {
		int leadcol = colIndex(leadCol);
		if (leadcol < 0){
			QApplication::restoreOverrideCursor();
			QMessageBox::critical(this, tr("QtiSAS - Error"),
			tr("Please indicate the name of the leading column!"));
			return;
		}

		int rows=d_table->rowCount();
		int non_empty_cells = 0;
		QVarLengthArray<int> valid_cell(rows);
		QVarLengthArray<double> data_double(rows);
		QVarLengthArray<QString> strings(rows);
		QString format = col_format[leadcol];
		for (int j = 0; j <rows; j++){
			if (!d_table->text(j, leadcol).isEmpty()){
				strings[non_empty_cells] = d_table->text(j, leadcol);				

				if (columnType(leadcol) == Table::Date || columnType(leadcol) == Table::Time)
					data_double[non_empty_cells] = fromDateTime(QDateTime::fromString (d_table->text(j, leadcol), format));
				else if (columnType(leadcol) == Table::Numeric)
					data_double[non_empty_cells] = cell(j, leadcol);

				valid_cell[non_empty_cells] = j;
				non_empty_cells++;
			}
		}

		if (!non_empty_cells){
			QApplication::restoreOverrideCursor();
			QMessageBox::critical(this, tr("QtiSAS - Error"),
			tr("The leading column is empty! Operation aborted!"));
			return;
		}

		data_double.resize(non_empty_cells);
		valid_cell.resize(non_empty_cells);
		strings.resize(non_empty_cells);

		// Find the permutation index for the lead col
		size_t *p = new size_t[non_empty_cells];
		if (columnType(leadcol) == Table::Text)
			gsl_heapsort_index(p, strings.data(), strings.count(), sizeof(QString), compare_qstrings);
		else
			gsl_sort_index(p, data_double.data(), 1, non_empty_cells);

		setAutoUpdateValues(false);

		for(int i = 0; i < cols; i++){// Since we have the permutation index, sort all the columns
			int col = colIndex(s[i]);
			if (d_table->isColumnReadOnly(col))
				continue;

			int type = columnType(col);
			if (type == Text){
				for (int j = 0; j<non_empty_cells; j++)
					strings[j] = text(valid_cell[j], col);
				if(!order)
					for (int j = 0; j < non_empty_cells; j++)
						d_table->setText(valid_cell[j], col, strings[p[j]]);
				else
					for (int j = 0; j < non_empty_cells; j++)
						d_table->setText(valid_cell[j], col, strings[p[non_empty_cells - j - 1]]);
			} else if (type == Date || type == Time){
				QString format = col_format[col];
				for (int j = 0; j<non_empty_cells; j++)
					data_double[j] = fromDateTime(QDateTime::fromString(d_table->text(valid_cell[j], col), format));
				if(!order)
					for (int j=0; j<non_empty_cells; j++)
						d_table->setText(valid_cell[j], col, dateTime(data_double[p[j]]).toString(format));
				else
					for (int j=0; j<non_empty_cells; j++)
						d_table->setText(valid_cell[j], col, dateTime(data_double[p[non_empty_cells - j - 1]]).toString(format));
			} else if (type == Numeric) {
				for (int j = 0; j<non_empty_cells; j++)
					data_double[j] = cell(valid_cell[j], col);
				int prec;
				char f;
				columnNumericFormat(col, &f, &prec);
				if(!order)
					for (int j=0; j<non_empty_cells; j++)
						d_table->setText(valid_cell[j], col, locale().toString(data_double[p[j]], f, prec));
				else
					for (int j=0; j<non_empty_cells; j++)
						d_table->setText(valid_cell[j], col, locale().toString(data_double[p[non_empty_cells-j-1]], f, prec));
			}
		}
		delete[] p;
	}

	for(int i = 0; i < cols; i++){// notify changes
		int col = colIndex(s[i]);
		if (d_table->isColumnReadOnly(col))
			continue;

		emit modifiedData(this, colName(col));
	}
	emit modifiedWindow(this);
	setAutoUpdateValues(applicationWindow()->autoUpdateTableValues());
}

void Table::sortColumn(int col, int order)
{
	if (col < 0)
		col = d_table->currentColumn();

	if (d_table->isColumnReadOnly(col))
		return;

	int rows=d_table->rowCount();
	int non_empty_cells = 0;
	QVarLengthArray<int> valid_cell(rows);
	QVarLengthArray<double> r(rows);
	QStringList text_cells;
	QString format = col_format[col];
	for (int i = 0; i <rows; i++){
		if (!d_table->text(i, col).isEmpty()){
			switch(columnType(col)){
				case Table::Text:
					text_cells << d_table->text(i, col);
				break;
				case Table::Date:
				case Table::Time:
					r[non_empty_cells] = fromDateTime(QDateTime::fromString(d_table->text(i, col), format));
				break;
				case Table::Numeric:
					r[non_empty_cells] = cell(i, col);
				break;
				default:
					break;
			}

			valid_cell[non_empty_cells] = i;
			non_empty_cells++;
		}
	}

	if (!non_empty_cells)
		return;

	int type = columnType(col);
	valid_cell.resize(non_empty_cells);
	if (type == Table::Text){
		r.clear();
		text_cells.sort();
	} else {
		r.resize(non_empty_cells);
		gsl_sort(r.data(), 1, non_empty_cells);
	}

	blockSignals(true);

	if (type == Table::Text){
		if (!order){
			for (int i=0; i<non_empty_cells; i++)
				d_table->setText(valid_cell[i], col, text_cells[i]);
		} else {
			for (int i=0; i<non_empty_cells; i++)
				d_table->setText(valid_cell[i], col, text_cells[non_empty_cells-i-1]);
		}
	} else if (type == Table::Date || type == Table::Time){
		if (!order) {
			for (int i=0; i<non_empty_cells; i++)
				d_table->setText(valid_cell[i], col, dateTime(r[i]).toString(format));
		 } else {
			 for (int i=0; i<non_empty_cells; i++)
				 d_table->setText(valid_cell[i], col, dateTime(r[non_empty_cells-i-1]).toString(format));
		 }
	} else if (type == Table::Numeric){
		int prec;
		char f;
		columnNumericFormat(col, &f, &prec);
		if (!order) {
			for (int i=0; i<non_empty_cells; i++)
				d_table->setText(valid_cell[i], col, locale().toString(r[i], f, prec));
		} else {
			for (int i=0; i<non_empty_cells; i++)
				d_table->setText(valid_cell[i], col, locale().toString(r[non_empty_cells-i-1], f, prec));
		}
	}

	blockSignals(false);

	emit modifiedData(this, colName(col));
	emit modifiedWindow(this);
}

void Table::sortColAsc()
{
	sortColumns(selectedColumns());
}

void Table::sortColDesc()
{
	sortColumns(selectedColumns(), 0, 1);
}

int Table::numRows()
{
	return d_table->rowCount();
}

int Table::numCols()
{
	return d_table->columnCount();
}

bool Table::isEmptyRow(int row)
{
	for (int i=0; i<d_table->columnCount(); i++)
	{
		QString text = d_table->text(row,i);
		if (!text.isEmpty())
			return false;
	}
	return true;
}

bool Table::isEmptyColumn(int col)
{
	if (col < 0 || col >= d_table->columnCount())
		return true;

	for (int i=0; i<d_table->rowCount(); i++)
	{
		QString text=d_table->text(i,col);
		if (!text.isEmpty())
			return false;
	}
	return true;
}

int Table::nonEmptyRows()
{
	int r=0;
	for (int i=0;i<d_table->rowCount();i++){
		if (!isEmptyRow(i))
			r++;
	}
	return r;
}

double Table::cell(int row, int col)
{
	int colType = colTypes[col];
	if (colType == Time)
		return fromTime(QTime::fromString(d_table->text(row, col).trimmed(), col_format[col].trimmed()));
	else if (colType == Date)
		return fromDateTime(QDateTime::fromString(d_table->text(row, col).trimmed(), col_format[col].trimmed()));

	return locale().toDouble(d_table->text(row, col));
}

void Table::setCell(int row, int col, double val)
{
	 if (col < 0 || col >= d_table->columnCount() ||
		 row < 0 || row >= d_table->rowCount())
		return;

    char format;
    int prec;
    columnNumericFormat(col, &format, &prec);
    d_table->setText(row, col, locale().toString(val, format, prec));
}

QString Table::text(int row, int col)
{
	return d_table->text(row, col);
}

void Table::setText (int row, int col, const QString & text )
{
	d_table->setText(row, col, text);
}

void Table::saveToMemory()
{
	int rows = d_table->rowCount();
	int cols = d_table->columnCount();

	d_saved_cells = new double* [cols];
	for ( int i = 0; i < cols; ++i)
		d_saved_cells[i] = new double [rows];

	for (int col = 0; col < cols; col++){// initialize the matrix to zero
		for (int row = 0; row < rows; row++)
			d_saved_cells[col][row] = 0.0;}

	for (int col = 0; col < cols; col++){
		int colType = colTypes[col];
		QString fmt = col_format[col].trimmed();
		if (colType == Time){
			for (int row = 0; row < rows; row++)
				d_saved_cells[col][row] = fromTime(QTime::fromString(d_table->text(row, col).trimmed(), fmt));
		} else if (colType == Date){
			for (int row = 0; row < rows; row++)
				d_saved_cells[col][row] = fromDateTime(QDateTime::fromString(d_table->text(row, col).trimmed(), fmt));
		}
	}

    //+++
    //QLocale l = locale();
    QLocale l = QLocale::c();
    l.setNumberOptions(QLocale::OmitGroupSeparator);
    //---
    for (int col = 0; col < cols; col++){
		if (colTypes[col] == Numeric){
			for (int row = 0; row < rows; row++){
				QString s = d_table->text(row, col);
				if (!s.isEmpty())
					d_saved_cells[col][row] = l.toDouble(s);
			}
		}
	}
}

void Table::freeMemory()
{
    for ( int i = 0; i < d_table->columnCount(); i++)
        delete[] d_saved_cells[i];

	delete[] d_saved_cells;
	d_saved_cells = 0;
}

void Table::setTextFormat(int col)
{
	if (col >= 0 && col < colTypes.count() && colTypes[col] != Text)
		colTypes[col] = Text;
}

void Table::setColNumericFormat(int col)
{
	if (colTypes[col] == Numeric || (col < 0 && col >= colTypes.count()))
		return;

	colTypes[col] = Numeric;
}

void Table::setColNumericFormat(int f, int prec, int col, bool updateCells)
{
	if (prec < 0)
		prec = 0;
	else if (prec > 14)
		prec = 14;

	if (colTypes[col] == Numeric){
		int old_f, old_prec;
		columnNumericFormat(col, &old_f, &old_prec);
		if (old_f == f && old_prec == prec)
			return;
	} else if (!updateCells)
		emit modifiedData(this, colName(col));

	colTypes[col] = Numeric;
	col_format[col] = QString::number(f) + "/" + QString::number(prec);

    if (!updateCells)
        return;

    char format = 'g';
	for (int i=0; i<d_table->rowCount(); i++) {
		QString t = text(i, col);
		if (!t.isEmpty()) {
			if (f == 1)
                format = 'f';
			else if (f == 2)
                format = 'e';
			else
				format = 'g';

			if (d_saved_cells)
				setText(i, col, locale().toString(d_saved_cells[col][i], format, prec));
			else
				setText(i, col, locale().toString(locale().toDouble(t), format, prec));
		}
	}
}

void Table::setColumnsFormat(const QStringList& lst)
{
	if (col_format == lst)
		return;

	col_format = lst;
}

QDateTime Table::dateTime(double val)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QDateTime d = QDateTime(QDate::fromJulianDay(static_cast<qint64>(val) + 1));
#else
    QDateTime d = QDate::fromJulianDay(static_cast<qint64>(val) + 1).startOfDay();
#endif
	double msecs = (val - floor(val))*864e5;
	d.setTime(d.time().addMSecs(qRound(msecs)));

	return d;
}

double Table::fromDateTime(const QDateTime& dt)
{
	return dt.date().toJulianDay() - 1 + (double)QTime(0, 0).msecsTo(dt.time())/864.0e5;
}

double Table::fromTime(const QTime& t)
{
	return (double)QTime(0, 0).msecsTo(t)/864.0e5;
}

bool Table::setDateFormat(const QString& format, int col, bool updateCells)
{
	if (colTypes[col] == Date && col_format[col] == format)
		return true;

	bool first_time = false;
	if (updateCells){
		for (int i=0; i<d_table->rowCount(); i++){
			QString s = d_table->text(i,col);
			if (!s.isEmpty()){
				QDateTime d = QDateTime::fromString (s, format);
				if (colTypes[col] != Date && d.isValid()){
				//This might be the first time the user assigns a date format.
				//If Qt understands the format we break the loop, assign it to the column and return true!
					first_time = true;
					break;
				}

				if (d_saved_cells){
					d = dateTime(d_saved_cells[col][i]);
					if (d.isValid())
						d_table->setText(i, col, d.toString(format));
				}
			}
		}
	}
	colTypes[col] = Date;
	col_format[col] = format;
	if (first_time){//update d_saved_cells in case the user changes the time format before pressing OK in the column dialog
		for (int i = 0; i < d_table->rowCount(); i++)
			d_saved_cells[col][i] = fromDateTime(QDateTime::fromString(d_table->text(i, col), format));
	}
	emit modifiedData(this, colName(col));
	return true;
}

bool Table::setTimeFormat(const QString& format, int col, bool updateCells)
{
	if (colTypes[col] == Time && col_format[col] == format)
		return true;

	bool first_time = false;
	if (updateCells){
		QTime ref = QTime(0, 0);
		for (int i = 0; i < d_table->rowCount(); i++){
			QString s = d_table->text(i, col);
			if (s.isEmpty())
				continue;

			QTime t = QTime::fromString(s, format);
			if (colTypes[col] != Time && t.isValid()){
			//This is the first time the user assigns a time format.
			//If Qt understands the format we break the loop, assign it to the column and return true!
				first_time = true;
				break;
			}

			if (d_saved_cells){
				t = ref.addMSecs(qRound(d_saved_cells[col][i]*864e5));
				if (t.isValid())
					d_table->setText(i, col, t.toString(format));
			}
		}
	}
	colTypes[col] = Time;
	col_format[col] = format;
	if (first_time)//update d_saved_cells in case the user changes the time format before pressing OK in the column dialog
		for (int i = 0; i < d_table->rowCount(); i++)
			d_saved_cells[col][i] = fromTime(QTime::fromString(d_table->text(i, col), format));
	emit modifiedData(this, colName(col));
	return true;
}

void Table::setMonthFormat(const QString& format, int col, bool updateCells)
{
    if (colTypes[col] == Month && col_format[col] == format)
        return;

	colTypes[col] = Month;
	col_format[col] = format;

	if (!updateCells)
        return;

	for (int i = 0; i < numRows(); i++){
        QString t = d_table->text(i,col);
        if (!t.isEmpty()){
            int day;
            if (d_saved_cells)
                day = int(d_saved_cells[col][i]) % 12;
            else
                day = t.toInt() % 12;
            if (!day)
                day = 12;

            if (format == "M")
                d_table->setText(i, col, QLocale().monthName(day, QLocale::ShortFormat).left(1));
            else if (format == "MMM")
                d_table->setText(i, col, QLocale().monthName(day, QLocale::ShortFormat));
            else if (format == "MMMM")
                d_table->setText(i, col, QLocale().monthName(day));
        }
    }
	emit modifiedData(this, colName(col));
}

void Table::setDayFormat(const QString& format, int col, bool updateCells)
{
    if (colTypes[col] == Day && col_format[col] == format)
        return;

	colTypes[col] = Day;
	col_format[col] = format;

	if (!updateCells)
        return;

	for (int i = 0; i < numRows(); i++){
        QString t = d_table->text(i,col);
        if (!t.isEmpty()){
            int day;
            if (d_saved_cells)
                day = int(d_saved_cells[col][i]) % 7;
            else
                day = t.toInt() % 7;
            if (!day)
                day = 7;

            if (format == "d")
                d_table->setText(i, col, QLocale().dayName(day, QLocale::ShortFormat).left(1));
            else if (format == "ddd")
                d_table->setText(i, col, QLocale().dayName(day, QLocale::ShortFormat));
            else if (format == "dddd")
                d_table->setText(i, col, QLocale().dayName(day));
        }
    }
	emit modifiedData(this, colName(col));
}

void Table::setRandomValues()
{
	QStringList list = writableSelectedColumns();
	if (list.isEmpty())
		return;

	MySelection selection = getSelection();
	for (int i = 0; i < list.count(); i++)
		setRandomValues(colIndex(list[i]), selection.topRow(), selection.bottomRow());

	emit modifiedWindow(this);
}

void Table::setRandomValues(int col, int startRow, int endRow)
{
	if (col < 0 || col >= d_table->columnCount() || d_table->isColumnReadOnly(col))
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	int rows = d_table->rowCount();
	if (startRow < 0 || startRow >= rows)
		startRow = 0;
	if (endRow < 0 || endRow >= rows)
		endRow = rows - 1;

	int prec;
	char f;
	columnNumericFormat(col, &f, &prec);

	srand(time(nullptr) + col);
	for (int i = startRow; i <= endRow; i++)
		d_table->setText(i, col, locale().toString(double(rand())/double(RAND_MAX), f, prec));

	emit modifiedData(this, colName(col));

	QApplication::restoreOverrideCursor();
}

void Table::setNormalRandomValues()
{
	QStringList list = writableSelectedColumns();
	if (list.isEmpty())
		return;

	MySelection selection = getSelection();
	for (int i = 0; i < list.count(); i++)
		setNormalRandomValues(colIndex(list[i]), selection.topRow(), selection.bottomRow());

	emit modifiedWindow(this);
}

void Table::setNormalRandomValues(int col, int startRow, int endRow, double sigma)
{
	if (col < 0 || col >= d_table->columnCount() || d_table->isColumnReadOnly(col))
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	int rows = d_table->rowCount();
	if (startRow < 0 || startRow >= rows)
		startRow = 0;
	if (endRow < 0 || endRow >= rows)
		endRow = rows - 1;

	gsl_rng_env_setup();
	const gsl_rng_type * T = gsl_rng_default;
	gsl_rng * r = gsl_rng_alloc (T);
	if (!r)
		return;

	int prec;
	char f;
	columnNumericFormat(col, &f, &prec);

	gsl_rng_set(r, time(nullptr) + col);
	for (int i = startRow; i <= endRow; i++)
		d_table->setText(i, col, locale().toString(gsl_ran_gaussian_ziggurat(r, sigma), f, prec));
	gsl_rng_free (r);

	emit modifiedData(this, colName(col));
	QApplication::restoreOverrideCursor();
}

void Table::loadHeader(QStringList header)
{
	col_label = QStringList();
	col_plot_type = QList <int>();
    
	for (int i=0; i<header.count();i++)
	{
		if (header[i].isEmpty())
			continue;

		QString s = header[i].replace("_","-");
		if (s.contains("[X]"))
		{
			col_label << s.remove("[X]");
			col_plot_type << X;
		}
		else if (s.contains("[Y]"))
		{
			col_label << s.remove("[Y]");
			col_plot_type << Y;
		}
		else if (s.contains("[Z]"))
		{
			col_label << s.remove("[Z]");
			col_plot_type << Z;
		}
		else if (s.contains("[xEr]"))
		{
			col_label << s.remove("[xEr]");
			col_plot_type << xErr;
		}
		else if (s.contains("[yEr]"))
		{
			col_label << s.remove("[yEr]");
			col_plot_type << yErr;
		}
		else if (s.contains("[L]"))
		{
			col_label << s.remove("[L]");
			col_plot_type << Label;
		}
		else
		{
			col_label << s;
			col_plot_type << None;
		}
	}
	if (col_label.count()>0) setHeaderColType();
}

void Table::setHeader(QStringList header)
{
	col_label = header;
	setHeaderColType();
}

int Table::colIndex(const QString& name)
{
	int pos = name.lastIndexOf("_");
	QString label = name.right(name.length() - pos - 1);
	return col_label.indexOf(label);
}

void Table::setHeaderColType()
{
	int xcols = 0;
	for (int j = 0; j < d_table->columnCount(); j++){
		if (col_plot_type[j] == X)
			xcols++;
	}
 
	if (xcols > 1){
		xcols = 0;
		for (int i = 0; i < d_table->columnCount(); i++){
			if (i >= col_label.size() || i >= col_plot_type.size())
				continue;
			int colType = col_plot_type[i];
			QString label = col_label[i];
			switch(colType){
				case X:
					setColumnHeader(i, label + "[X" + QString::number(++xcols) + "]");
				break;
				case Y:
					if (xcols > 0)
						setColumnHeader(i, label + "[Y" + QString::number(xcols) + "]");
					else
						setColumnHeader(i, label + "[Y]");
				break;
				case Z:
					if (xcols > 0)
						setColumnHeader(i, label + "[Z" + QString::number(xcols) + "]");
					else
						setColumnHeader(i, label + "[Z]");
				break;
				case xErr:
					setColumnHeader(i, label + "[xEr]");
				break;
				case yErr:
					setColumnHeader(i, label + "[yEr]");
				break;
				case Label:
					setColumnHeader(i, label + "[L]");
				break;
				default:
					setColumnHeader(i, label);
				break;
			}
		}
	} else {
		for (int i = 0; i < d_table->columnCount(); i++){
			if (i >= col_label.size() || i >= col_plot_type.size())
				continue;
			int colType = col_plot_type[i];
			QString label = col_label[i];
			switch(colType){
				case X:
					setColumnHeader(i, label + "[X]");
				break;
				case Y:
					setColumnHeader(i, label + "[Y]");
				break;
				case Z:
					setColumnHeader(i, label + "[Z]");
				break;
				case xErr:
					setColumnHeader(i, label + "[xEr]");
				break;
				case yErr:
					setColumnHeader(i, label + "[yEr]");
				break;
				case Label:
					setColumnHeader(i, label + "[L]");
				break;
				default:
					setColumnHeader(i, label);
				break;
			}
		}
	}
}

QStringList Table::writableSelectedColumns()
{
	MySelection selection = getSelection();
	QStringList list = selectedColumns();
	if (list.isEmpty()){
		for (int i = selection.leftColumn(); i <= selection.rightColumn(); i++)
			list << colName(i);
	}

	QStringList lstReadOnly;
	for (int i = 0; i < list.count(); i++){
		QString name = list[i];
		int col = colIndex(name);
		if (d_table->isColumnReadOnly(col))
			lstReadOnly << name;
	}

	if (!lstReadOnly.isEmpty()){
		QMessageBox::warning(this, tr("QtiSAS - Error"),
		tr("The folowing columns") + ":\n" + lstReadOnly.join("\n") + "\n" + tr("are read only!"));
	}
	return list;
}

void Table::setAscValues()
{
	QStringList list = writableSelectedColumns();
	if (list.isEmpty())
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	MySelection selection = getSelection();
	for (int j = 0; j < list.count(); j++){
		QString name = list[j];
		selectedCol = colIndex(name);

        if (d_table->isColumnReadOnly(selectedCol))
            continue;

		if (columnType(selectedCol) != Numeric){
			colTypes[selectedCol] = Numeric;
			col_format[selectedCol] = "0/6";
		}

		int prec;
		char f;
		columnNumericFormat(selectedCol, &f, &prec);

		for (int i = selection.topRow(); i <= selection.bottomRow(); i++)
			setText(i, selectedCol, QString::number(i + 1, f, prec));

		emit modifiedData(this, name);
	}
	emit modifiedWindow(this);
	QApplication::restoreOverrideCursor();
}

bool Table::noXColumn()
{
	bool notSet = true;
	for (int i=0; i<d_table->columnCount(); i++){
		if (col_plot_type[i] == X)
			return false;
	}
	return notSet;
}

bool Table::noYColumn()
{
	bool notSet = true;
	for (int i=0; i<d_table->columnCount(); i++){
		if (col_plot_type[i] == Y)
			return false;
	}
	return notSet;
}

void Table::importASCII(const QString &fname, const QString &sep, int ignoredLines, bool renameCols,
    bool stripSpaces, bool simplifySpaces, bool importComments, const QString& commentString,
	bool readOnly, ImportMode importAs, const QLocale& importLocale, int endLine, int maxRows,
	const QList<int>& newColTypes, const QStringList& colFormats)
{
	int rows;
	QString name = MdiSubWindow::parseAsciiFile(fname, commentString, endLine, ignoredLines, maxRows, rows);
	if (name.isEmpty())
		return;

	QFile f(name);
	if (!f.open(QIODevice::ReadOnly))
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QLocale locale = this->locale();
	bool updateDecimalSeparators = (importLocale != locale) ? true : false;

	QTextStream t(&f);
	QString s = t.readLine();//read first line
	if (simplifySpaces)
		s = s.simplified();
	else if (stripSpaces)
		s = s.trimmed();

	QStringList line = s.split(sep);
	int cols = line.size();

	bool allNumbers = true;
	for (int i=0; i<cols; i++)
	{//verify if the strings in the line used to rename the columns are not all numbers
		locale.toDouble(line[i], &allNumbers);
		if (!allNumbers)
			break;
	}
	if (renameCols && !allNumbers)
		rows--;
	if (importComments)
		rows--;

	QStringList oldHeader = col_label;
	int startRow = 0, startCol = 0;
	int c = d_table->columnCount();
	int r = d_table->rowCount();
	switch(importAs){
		case Overwrite:
			if (d_table->rowCount() != rows)
				setNumRows(rows);
                //d_table->setRowCount(rows);
			if (c != cols){
				if (c < cols)
					addColumns(cols - c);
				else {
					//d_table->setColumnCount(cols);
                    setNumCols(cols);
					for (int i = c-1; i>=cols; i--){
						emit removedCol(QString(objectName()) + "_" + oldHeader[i]);
						commands.removeLast();
						comments.removeLast();
						col_format.removeLast();
						col_label.removeLast();
						colTypes.removeLast();
						col_plot_type.removeLast();
					}
				}
			}
		break;
		case NewColumns:
			startCol = c;
			addColumns(cols);
			if (r < rows)
				setNumRows(rows);
				//d_table->setRowCount(rows);
		break;
		case NewRows:
			startRow = r;
			if (c < cols)
				addColumns(cols - c);
            //d_table->setRowCount(r + rows);
            setNumRows(r + rows);
		break;
	}

	if (!newColTypes.isEmpty()){
		int ncols = newColTypes.size();
		int fSize = colFormats.size();
		switch(importAs){
			case NewColumns:
				for (int i = c; i < c + cols && i < ncols && i < colTypes.size(); i++){
					int type = newColTypes[i];
					colTypes[i] = type;
					if (type != Numeric && type != Text && i < fSize)
						col_format[i] = colFormats[i];
				}
			break;
			default:
				for (int i = 0; i < cols && i < ncols; i++){
					int type = newColTypes[i];
					colTypes[i] = type;
					if (type != Numeric && type != Text && i < fSize){
						col_format[i] = colFormats[i];
					}
				}
			break;
		}
	}

	if (renameCols && !allNumbers){//use first line to set the table header
		for (int i = 0; i<cols; i++){
			int aux = i + startCol;
            col_label[aux] = QString();
			if (!importComments)
				comments[aux] = line[i];
			s = line[i].replace("-","_").remove(QRegExp("\\W")).replace("_","-");
			int n = col_label.count(s);
			if(n){//avoid identical col names
				while (col_label.contains(s+QString::number(n)))
					n++;
				s += QString::number(n);
			}
			col_label[aux] = s;
		}
	}

	if (importComments){//import comments
		if (renameCols && !allNumbers)
			s = t.readLine();//read 2nd line
		if (simplifySpaces)
			s = s.simplified();
		else if (stripSpaces)
			s = s.trimmed();
        line = s.split(sep);
		for (int i=0; i<line.size(); i++){
			int aux = startCol + i;
			if (aux < comments.size())
				comments[aux] = line[i];
		}
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
		showComments(true);
	}

	if ((!renameCols || allNumbers) && !importComments && rows > 0){
		//put values in the first line of the table
		for (int i = 0; i < cols; i++){
			QString cell = line[i];
			if (cell.isEmpty())
				continue;

			bool ok;
			double val = importLocale.toDouble(cell, &ok);
			if (colTypes[startCol + i] == Table::Numeric && (ok || updateDecimalSeparators)){
				char format;
				int prec;
				columnNumericFormat(startCol + i, &format, &prec);
				d_table->setText(startRow, startCol + i, locale.toString(val, format, prec));
			} else
				d_table->setText(startRow, startCol + i, cell);
		}
		startRow++;
	}

	d_table->blockSignals(true);
	setHeaderColType();

	int steps = rows/100 + 1;
	QProgressDialog progress((QWidget *)applicationWindow());
	progress.setWindowTitle(tr("QtiSAS") + " - " + tr("Reading file..."));
	progress.setLabelText(fname);
	progress.activateWindow();
	progress.setAutoClose(true);
	progress.setAutoReset(true);
	progress.setRange(0, steps);

	QApplication::restoreOverrideCursor();

	int l = 0;
	int row = startRow;
	rows = d_table->rowCount();
	while (!t.atEnd() && row < rows){
		if (progress.wasCanceled()){
			f.close();
			return;
		}
		s = t.readLine();
		if (simplifySpaces)
			s = s.simplified();
		else if (stripSpaces)
			s = s.trimmed();
		line = s.split(sep);
		int lc = line.size();
		if (lc > cols) {
			addColumns(lc - cols);
			cols = lc;
		}

		for (int j = 0; j<cols && j<lc; j++){
			QString cell = line[j];
			if (cell.isEmpty())
				continue;

			bool ok;
			double val = importLocale.toDouble(cell, &ok);
			if (colTypes[startCol + j] == Table::Numeric && (ok || updateDecimalSeparators)){
				char format;
				int prec;
				columnNumericFormat(startCol + j, &format, &prec);
				d_table->setText(row, startCol + j, locale.toString(val, format, prec));
			} else
				d_table->setText(row, startCol + j, cell);
		}

		l++;
		row++;
		if (l%100 == 0)
			progress.setValue(l/100);
	}

	d_table->blockSignals(false);
	f.remove();

	if (readOnly) {
		for (int i = 0; i<cols; i++)
			d_table->setColumnReadOnly(startCol + i, true);
	}

	if (importAs == Overwrite || importAs == NewRows){
		if (cols > c)
			cols = c;

		for (int i = 0; i < cols; i++){
			emit modifiedData(this, colName(i));
			if (colLabel(i) != oldHeader[i])
				emit changedColHeader(QString(objectName()) + "_" + oldHeader[i], QString(objectName()) + "_" + colLabel(i));
		}
	}
}

bool Table::exportASCII(const QString& fname, const QString& separator,
		bool withLabels, bool exportComments, bool exportSelection)
{
	if (!applicationWindow())
		return false;

	if (fname.endsWith(".tex")){
		ImportExportPlugin *ep = applicationWindow()->exportPlugin("tex");
		if (!ep)
			return false;
		return ep->exportTable(this, fname, withLabels, exportComments, exportSelection);
	} else if (fname.endsWith(".csv")){
		ImportExportPlugin *ep = applicationWindow()->exportPlugin("csv");
		if (!ep)
			return false;
		return ep->exportTable(this, fname, withLabels, exportComments, exportSelection);
	}

	QFile f(fname);
	if ( !f.open( QIODevice::WriteOnly ) ){
		QMessageBox::critical(0, tr("QtiSAS - ASCII Export Error"),
            tr(QString("Could not write to file: <br><h4>" + fname +
            "</h4><p>Please verify that you have the right to write to this location!").toLocal8Bit().constData()).arg(fname));
		return false;
	}

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QTextStream t( &f );
	QString eol = applicationWindow()->endOfLine();
	QString sep = separator;

	int rows = d_table->rowCount();
	int cols = d_table->columnCount();
	int selectedCols = 0;
	int topRow = 0, bottomRow = 0;
	int *sCols = 0;
	if (exportSelection){
		for (int i=0; i<cols; i++){
			if (d_table->isColumnSelected(i))
				selectedCols++;
		}

		sCols = new int[selectedCols];
		int aux = 0;
		for (int i=0; i<cols; i++){
			if (d_table->isColumnSelected(i)){
				sCols[aux] = i;
				aux++;
			}
		}

		for (int i=0; i<rows; i++){
			if (d_table->isRowSelected(i)){
				topRow = i;
				break;
			}
		}

		for (int i = rows - 1; i > 0; i--){
			if (d_table->isRowSelected(i)){
				bottomRow = i;
				break;
			}
		}
	}

	int aux = selectedCols - 1;
	if (!selectedCols)
		aux = cols - 1;

	if (withLabels){
		QStringList header = colNames();
		QStringList ls = header.filter( QRegExp ("\\D"));
		if (exportSelection && selectedCols){
			for (int i = 0; i < aux; i++){
				if (ls.count()>0)
					t << header[sCols[i]] + sep;
				else
					t << "C" + header[sCols[i]] + sep;
			}

			if (aux >= 0){
				if (ls.count()>0)
					t << header[sCols[aux]] + eol;
				else
					t << "C" + header[sCols[aux]] + eol;
			}
		} else {
			if (ls.count()>0){
				for (int j = 0; j < aux; j++)
					t << header[j] + sep;
				t << header[aux] + eol;
			} else {
				for (int j = 0; j < aux; j++)
					t << "C" + header[j] + sep;
				t << "C" + header[aux] + eol;
			}
		}
	}// finished writting labels

	if (exportComments){
		if (exportSelection && selectedCols){
			for (int i = 0; i < aux; i++)
				t << comments[sCols[i]] + sep;
			if (aux >= 0)
				t << comments[sCols[aux]] + eol;
		} else {
			for (int i = 0; i < aux; i++)
				t << comments[i] + sep;
			t << comments[aux] + eol;
		}
	}

	if (exportSelection && selectedCols){
		for (int i = topRow; i <= bottomRow; i++){
			for (int j = 0; j < aux; j++)
				t << d_table->text(i, sCols[j]) + sep;
			if (aux >= 0)
				t << d_table->text(i, sCols[aux]) + eol;
		}
		delete [] sCols;
	} else {
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < aux; j++)
				t << d_table->text(i, j) + sep;
			t << d_table->text(i, aux) + eol;
		}
	}

	f.close();
	QApplication::restoreOverrideCursor();
	return true;
}

void Table::moveCurrentCell()
{
	int cols=d_table->columnCount();
	int row=d_table->currentRow();
	int col=d_table->currentColumn();
	d_table->clearSelection();

	if (col+1<cols)
	{
		d_table->setCurrentCell(row, col + 1);
        MySelection range(row, col + 1, row, col + 1);
		d_table->setRangeSelected(range, true);
	}
	else
	{
        if(row+1 >= numRows())
            setNumRows(row + 11);
            //d_table->setRowCount(row + 11);
		d_table->setCurrentCell (row+1, 0);
        MySelection range(row + 1, 0, row + 1, 0);
		d_table->setRangeSelected(range, true);
	}
}

bool Table::eventFilter(QObject *object, QEvent *e)
{
	QHeaderView *hheader = d_table->horizontalHeader();
	QHeaderView *vheader = d_table->verticalHeader();

    const QMouseEvent *me = (const QMouseEvent *)e;
    if (e->type() == QEvent::MouseButtonDblClick && object == (QObject*)hheader) {
        selectedCol = hheader->logicalIndexAt(me->pos().x());
        QRect rect(hheader->sectionViewportPosition(selectedCol), 0,
                   hheader->sectionSize(selectedCol), hheader->height());
        if (rect.contains(me->pos()))
            emit optionsDialog();
    }
    if (e->type() == QEvent::ContextMenu && object == (QObject*)d_table) {
        const QContextMenuEvent *ce = (const QContextMenuEvent *) e;
        QRect r(hheader->sectionViewportPosition(d_table->columnCount() - 1), 0,
                hheader->sectionSize(d_table->columnCount() - 1), hheader->height());
        setFocus();
        if (ce->pos().x() > r.right() + d_table->verticalHeader()->width())
            emit showContextMenu(false);
        else if (d_table->columnCount() > 0 && d_table->rowCount() > 0)
            emit showContextMenu(true);
    }
	return MdiSubWindow::eventFilter(object, e);
}

void Table::customEvent(QEvent *e)
{
	if (e->type() == SCRIPTING_CHANGE_EVENT)
		scriptingChangeEvent((ScriptingChangeEvent*)e);
}

void Table::setNumRows(int rows)
{
    int old=d_table->rowCount();
	d_table->setRowCount(rows);
    if (rows<=old) return;
    updateVerticalHeaderByFont(d_table->font(),-1);
}

void Table::setNumCols(int c)
{
	int cols = d_table->columnCount();
	if (cols == c)
		return;

	if (cols > c){
		d_table->setColumnCount(c);
		for (int i = cols - 1; i >= c; i--){
			commands.removeLast();
			comments.removeLast();
			col_format.removeLast();
			col_label.removeLast();
			colTypes.removeLast();
			col_plot_type.removeLast();
		}
	} else {
		addColumns(c - cols);
		setHeaderColType();
	}
}

void Table::resizeRows(int r)
{
	int rows = d_table->rowCount();
	if (rows == r)
		return;

	if (rows > r){
		QString text= tr("Rows will be deleted from the table!");
		text+="<p>"+tr("Do you really want to continue?");
		int i,cols = d_table->columnCount();
		switch( QMessageBox::information(this,tr("QtiSAS"), text, tr("Yes"), tr("Cancel"), 0, 1 ) )
		{
			case 0:
				QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
				//d_table->setRowCount(r);
                setNumRows(r);
				for (i=0; i<cols; i++)
					emit modifiedData(this, colName(i));

				QApplication::restoreOverrideCursor();
				break;

			case 1:
				return;
				break;
		}
	} else {
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		//d_table->setRowCount(r);
        setNumRows(r);
//        for(int rr=rows;rr<r; rr++) d_table->setRowHeight(rr, rowHight);
        updateVerticalHeaderByFont(d_table->font(),-1);
		QApplication::restoreOverrideCursor();
	}

	emit modifiedWindow(this);
}

void Table::resizeCols(int c)
{
	int cols = d_table->columnCount();
	if (cols == c)
		return;

	if (cols > c){
		QString text= tr("Columns will be deleted from the table!");
		text+="<p>"+tr("Do you really want to continue?");
		switch( QMessageBox::information(this,tr("QtiSAS"), text, tr("Yes"), tr("Cancel"), 0, 1 ) ){
			case 0: {
				QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
				for (int i=cols-1; i>=c; i--){
					QString name = colName(i);
					emit removedCol(name);
					d_table->removeColumn(i);

					commands.removeLast();
					comments.removeLast();
					col_format.removeLast();
					col_label.removeLast();
					colTypes.removeLast();
					col_plot_type.removeLast();
				}
				QApplication::restoreOverrideCursor();
				break;
			}

			case 1:
				return;
				break;
		}
	}
	else{
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		addColumns(c-cols);
		setHeaderColType();
        for(int cc=cols;cc<c; cc++) d_table->setColumnWidth(cc, colWidth);
		QApplication::restoreOverrideCursor();
	}
	emit modifiedWindow(this);
}

void Table::copy(Table *m, bool values)
{
	int rows = d_table->rowCount();
	int cols = d_table->columnCount();

	if (values){
		for (int i=0; i<rows; i++){
			for (int j=0; j<cols; j++)
				d_table->setText(i, j, m->text(i, j));
		}
	}

	for (int i=0; i<cols; i++){
		d_table->setColumnReadOnly(i, m->isReadOnlyColumn(i));
		d_table->setColumnWidth(i, m->columnWidth(i));
		if (m->isColumnHidden(i))
			d_table->hideColumn(i);
	}

	col_label = m->colNames();
	col_plot_type = m->plotDesignations();
	d_show_comments = m->commentsEnabled();
    comments = m->colComments();
	setHeaderColType();

	commands = m->getCommands();
	setColumnTypes(m->columnTypes());
	col_format = m->getColumnsFormat();
}

void Table::restore(const QStringList& flist)
{
	int cols = numCols();
	QStringList::const_iterator line = flist.begin();
    for (; line != flist.end(); line++)
    {
		QStringList fields = (*line).split("\t");
		if (fields[0] == "geometry" || fields[0] == "tgeometry"){
			ApplicationWindow *app = applicationWindow();
			if (app)
				app->restoreWindowGeometry(app, this, *line);
		} else if (fields[0] == "header") {
			fields.pop_front();
			loadHeader(fields);
		} else if (fields[0] == "ColWidth") {
			fields.pop_front();
			setColWidths(fields);
		} else if (fields[0] == "com") { // legacy code
			fields.pop_front();
			setCommands(*line);
		} else if (fields[0] == "<com>") {
			for (line++; line != flist.end() && *line != "</com>"; line++){
				int col = (*line).mid(9, (*line).length() - 11).toInt();
				QString formula;
				for (line++; line != flist.end() && *line != "</col>"; line++)
					formula += *line + "\n";
				formula.truncate(formula.length() - 1);
				setCommand(col,formula);
			}
		} else if (fields[0] == "ColType") { // d_file_version > 65
			fields.pop_front();
			setColumnTypes(fields);
		} else if (fields[0] == "Comments") { // d_file_version > 71
			fields.pop_front();
			setColComments(fields);
			setHeaderColType();
		} else if (fields[0] == "WindowLabel" && fields.size() >= 3) { // d_file_version > 71
			setWindowLabel(fields[1]);
			setCaptionPolicy((MdiSubWindow::CaptionPolicy)fields[2].toInt());
		} else if (fields[0] == "ReadOnlyColumn") { // d_file_version > 91
			fields.pop_front();
			for (int i = 0; i < cols; i++)
				setReadOnlyColumn(i, fields[i].toShort());
		} else if (fields[0] == "HiddenColumn") { // d_file_version >= 93
			fields.pop_front();
			for (int i = 0; i < cols; i++)
				hideColumn(i, fields[i].toShort());
		} else // <data> or values
			break;
	}
}

void Table::notifyChanges()
{
	bool updateValues = applicationWindow()->autoUpdateTableValues();
	if (updateValues)
		setAutoUpdateValues(false);

	for(int i = 0; i < d_table->columnCount(); i++){
		if (d_table->isColumnReadOnly(i))
			continue;

		emit modifiedData(this, colName(i));
	}
	emit modifiedWindow(this);

	if (updateValues)
		setAutoUpdateValues(true);
}

void Table::notifyChanges(const QString& colName)
{
	emit modifiedData(this, colName);
	emit modifiedWindow(this);
}

void Table::clear()
{
	for (int i=0; i<d_table->columnCount(); i++)
	{
		for (int j=0; j<d_table->rowCount(); j++)
            d_table->setText(j, i, QString());

		emit modifiedData(this, colName(i));
	}

	emit modifiedWindow(this);
}

void Table::goToRow(int row)
{
	if( (row < 1) || (row > numRows()) ) return;

    QTableWidgetItem *it = d_table->item(row-1, 0);
    d_table->scrollToItem(it, QAbstractItemView::EnsureVisible);
	d_table->clearSelection();
	d_table->selectRow(row-1);
}

void Table::goToColumn(int col)
{
	if( (col < 1) || (col > numCols()) )
		return;

    QTableWidgetItem *it = d_table->item(0, col - 1);
    d_table->scrollToItem(it, QAbstractItemView::EnsureVisible);
	d_table->clearSelection();
	d_table->selectColumn(col - 1);
}

void Table::setColumnHeader(int index, const QString& label)
{
	QHeaderView *head = d_table->horizontalHeader();
    QTableWidgetItem *it = d_table->horizontalHeaderItem(index);
    if (!it) {
        it = new QTableWidgetItem();
        d_table->setHorizontalHeaderItem(index, it);
    }
    QString s = label;
	if (d_show_comments) {
		int lines = d_table->columnWidth(index)/head->fontMetrics().boundingRect("_").width();
		if (index >= 0 && index < comments.size())
            it->setText(s.remove("\n") + "\n" + QString(lines, '_') + "\n" + comments[index]);
	} else
        it->setText(s);
}

void Table::showComments(bool on)
{
	if (d_show_comments == on)
		return;

	d_show_comments = on;
	if (applicationWindow())
		applicationWindow()->d_show_table_comments = on;
	setHeaderColType();

    updateHorizontalHeaderByFont(d_table->horizontalHeader()->font());
}

void Table::setNumericPrecision(int prec)
{
	if (prec > 14)
		prec = 14;

	d_numeric_precision = prec;
	for (int i=0; i<d_table->columnCount(); i++){
		if (colTypes[i] == Numeric)
        	col_format[i] = "0/" + QString::number(prec);
	}
}

void Table::updateDecimalSeparators(const QLocale& oldSeparators)
{
    //+++
    //QLocale l = locale();
    QLocale l = QLocale::c();
    l.setNumberOptions(QLocale::OmitGroupSeparator);
    //---
    if (l == oldSeparators)
        return;

    int rows = d_table->rowCount();
	for (int i=0; i<d_table->columnCount(); i++){
	    if (colTypes[i] != Numeric)
            continue;

        char format;
        int prec;
        columnNumericFormat(i, &format, &prec);

        for (int j = 0; j<rows; j++){
            QString s = d_table->text(j, i);
            if (!s.isEmpty()){
				double val = oldSeparators.toDouble(s);
                d_table->setText(j, i, l.toString(val, format, prec));
			}
		}
	}
}

bool Table::isReadOnlyColumn(int col)
{
    if (col < 0 || col >= d_table->columnCount())
        return false;

    return d_table->isColumnReadOnly(col);
}

void Table::setReadOnlyColumn(int col, bool on)
{
    if (col < 0 || col >= d_table->columnCount())
        return;

	if (d_table->isColumnReadOnly(col) == on)
		return;

	d_table->setColumnReadOnly(col, on);
	emit modifiedWindow(this);
}

void Table::moveColumn(int, int fromIndex, int toIndex)
{
    int to = toIndex;
    if (fromIndex < toIndex)
        to = toIndex - 1;

    col_label.move(fromIndex, to);
    comments.move(fromIndex, to);
	commands.move(fromIndex, to);
	colTypes.move(fromIndex, to);
	col_format.move(fromIndex, to);
	col_plot_type.move(fromIndex, to);
	setHeaderColType();
}

void Table::swapColumns(int col1, int col2)
{
    if (col1 < 0 || col1 >= d_table->columnCount() || col2 < 0 || col2 >= d_table->columnCount())
        return;

    int width1 = d_table->columnWidth(col1);
    int width2 = d_table->columnWidth(col2);

    d_table->swapColumns(col1, col2);
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
    col_label.swap(col1, col2);
    comments.swap(col1, col2);
    commands.swap(col1, col2);
    colTypes.swap(col1, col2);
    col_format.swap(col1, col2);
    col_plot_type.swap(col1, col2);
#else
    col_label.swapItemsAt(col1, col2);
    comments.swapItemsAt(col1, col2);
    commands.swapItemsAt(col1, col2);
    colTypes.swapItemsAt(col1, col2);
    col_format.swapItemsAt(col1, col2);
    col_plot_type.swapItemsAt(col1, col2);
#endif
    d_table->setColumnWidth(col1, width2);
    d_table->setColumnWidth(col2, width1);
    setHeaderColType();
	emit modifiedWindow(this);
	emit colIndexChanged(col1, col2);
}

void Table::moveColumnBy(int cols)
{
	int oldPos = selectedCol;
	int newPos = oldPos + cols;
	if (newPos < 0)
		newPos = 0;
	else if	(newPos >= d_table->columnCount())
		newPos = d_table->columnCount() - 1;

	if (abs(cols) > 1) {
		if (cols < 0) {
			d_table->insertColumn(newPos);
			d_table->swapColumns(oldPos + 1, newPos);
			d_table->removeColumn(oldPos + 1);
		} else {
			d_table->insertColumn(newPos + 1);
			d_table->swapColumns(oldPos, newPos + 1);
			d_table->removeColumn(oldPos);
		}

		col_label.move(oldPos, newPos);
    	comments.move(oldPos, newPos);
		commands.move(oldPos, newPos);
		colTypes.move(oldPos, newPos);
		col_format.move(oldPos, newPos);
		col_plot_type.move(oldPos, newPos);
		emit colIndexChanged(oldPos, newPos);
	} else
		swapColumns(oldPos, newPos);

	setHeaderColType();

	setSelectedCol(newPos);
    d_table->clearSelection();
    d_table->selectColumn(newPos);
	emit modifiedWindow(this);
}

void Table::hideColumn(int col, bool hide)
{
	if (hide)
		d_table->hideColumn(col);
	else
		d_table->showColumn(col);

	emit modifiedWindow(this);
}

void Table::hideSelectedColumns()
{
	for(int i = 0; i<d_table->columnCount(); i++)
		if (d_table->isColumnSelected(i, true))
			d_table->hideColumn(i);
}

void Table::showAllColumns()
{
	for(int i = 0; i<d_table->columnCount(); i++) {
		if (d_table->isColumnHidden(i))
			d_table->showColumn(i);
		if (!d_table->columnWidth(i))
			d_table->resizeColumnToContents(i);
	}
	emit modifiedWindow(this);
}

QString Table::sizeToString()
{
	int size = d_table->rowCount() * d_table->columnCount();
	return QString::number((sizeof(Table) + size*sizeof(QTableWidgetItem))/1024.0, 'f', 1) + " " + tr("kB");
}

void Table::moveRow(bool up)
{
	int row = d_table->currentRow();
	int nextRow = row - 1;
	if (up) {
        if (!row)
            return;
	} else {
		if (row == d_table->rowCount() - 1)
			return;

		nextRow = row + 1;
	}

	d_table->clearSelection();
	d_table->swapRows(row, nextRow);
	d_table->selectRow(nextRow);
    QTableWidgetItem *it = d_table->item(nextRow, 0);
    d_table->scrollToItem(it, QAbstractItemView::EnsureVisible);
	d_table->update();

	emit modifiedWindow(this);
}

double Table::sum(int col, int startRow, int endRow)
{
	if (col < 0 || col >= d_table->columnCount())
		return 0.0;

	if (colTypes[col] != Numeric)
		return 0.0;

	int rows = d_table->rowCount();
	if (startRow < 0 || startRow >= rows)
		startRow = 0;
	if (endRow < 0 || endRow >= rows)
		endRow = d_table->rowCount() - 1;

    //+++
    //QLocale l = locale();
    QLocale l = QLocale::c();
    l.setNumberOptions(QLocale::OmitGroupSeparator);
    //---
	double sum = 0.0;
	for (int i = startRow; i <= endRow; i++){
		QString s = d_table->text(i, col);
		if (!s.isEmpty())
			sum += l.toDouble(s);
	}
	return sum;
}

double Table::avg(int col, int startRow, int endRow)
{
	if (col < 0 || col >= d_table->columnCount())
		return 0.0;

	if (colTypes[col] != Numeric)
		return 0.0;

	int rows = d_table->rowCount();
	if (startRow < 0 || startRow >= rows)
		startRow = 0;
	if (endRow < 0 || endRow >= rows)
		endRow = d_table->rowCount() - 1;

    //+++
    //QLocale l = locale();
    QLocale l = QLocale::c();
    l.setNumberOptions(QLocale::OmitGroupSeparator);
    //---
	double sum = 0.0;
	int count = 0;
	for (int i = startRow; i <= endRow; i++){
		QString s = d_table->text(i, col);
		if (!s.isEmpty()){
			sum += l.toDouble(s);
			count++;
		}
	}

	if (count)
		return sum/(double)count;

	return 0.0;
}

double Table::minColumnValue(int col, int startRow, int endRow)
{
	if (col < 0 || col >= d_table->columnCount())
		return 0.0;

	if (colTypes[col] != Numeric)
		return 0.0;

	int rows = d_table->rowCount();
	if (startRow < 0 || startRow >= rows)
		startRow = 0;
	if (endRow < 0 || endRow >= rows)
		endRow = d_table->rowCount() - 1;

    //+++
    //QLocale l = locale();
    QLocale l = QLocale::c();
    l.setNumberOptions(QLocale::OmitGroupSeparator);
    //---
	double min = this->cell(startRow, col);
	for (int i = startRow + 1; i <= endRow; i++){
		QString s = d_table->text(i, col);
		if (!s.isEmpty()){
			double val = l.toDouble(s);
			if (val < min)
				min = val;
		}
	}
	return min;
}

double Table::maxColumnValue(int col, int startRow, int endRow)
{
	if (col < 0 || col >= d_table->columnCount())
		return 0.0;

	if (colTypes[col] != Numeric)
		return 0.0;

	int rows = d_table->rowCount();
	if (startRow < 0 || startRow >= rows)
		startRow = 0;
	if (endRow < 0 || endRow >= rows)
		endRow = d_table->rowCount() - 1;

    //+++
    //QLocale l = locale();
    QLocale l = QLocale::c();
    l.setNumberOptions(QLocale::OmitGroupSeparator);
    //---
	double max = this->cell(startRow, col);
	for (int i = startRow + 1; i <= endRow; i++){
		QString s = d_table->text(i, col);
		if (!s.isEmpty()){
			double val = l.toDouble(s);
			if (val > max)
				max = val;
		}
	}
	return max;
}
