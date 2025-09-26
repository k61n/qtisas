/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Set column values dialog
 ******************************************************************************/

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCompleter>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QPushButton>
#include <QSpinBox>
#include <QTextCursor>
#include <QTextEdit>

#include "ApplicationWindow.h"
#include "muParserScripting.h"
#include "ScriptEdit.h"
#include "SetColValuesDialog.h"
#include "Table.h"

SetColValuesDialog::SetColValuesDialog( ScriptingEnv *env, QWidget* parent, Qt::WindowFlags fl )
    : QDialog( parent, fl ), scripted(env)
{
    setObjectName( "SetColValuesDialog" );
	setWindowTitle( tr( "QtiSAS - Set column values" ) );
	setSizeGripEnabled(true);
    setAttribute(Qt::WA_DeleteOnClose);

	QHBoxLayout *hbox1 = new QHBoxLayout();
	hbox1->addWidget(new QLabel(tr("For row (i)")));
	start = new QSpinBox();
	start->setMinimum(1);
    start->setMaximum(INT_MAX);
	hbox1->addWidget(start);

	hbox1->addWidget(new QLabel(tr("to")));

	end = new QSpinBox();
	end->setMinimum(1);
    end->setMaximum(INT_MAX);
	hbox1->addWidget(end);

    hbox1->addWidget(new QLabel(tr("step")));

    step = new QSpinBox();
    step->setMinimum(1);
    step->setMaximum(INT_MAX - 1);
    hbox1->addWidget(step);

	QGridLayout *gl1 = new QGridLayout();
	functions = new QComboBox();
	functions->addItems(muParserScripting::functionsList(true));
	gl1->addWidget(functions, 0, 0);
	btnAddFunction = new QPushButton(tr( "Add function" ));
	gl1->addWidget(btnAddFunction, 0, 1);
	boxColumn = new QComboBox();
	gl1->addWidget(boxColumn, 1, 0);
	btnAddCol = new QPushButton(tr( "Add column" ));
	gl1->addWidget(btnAddCol, 1, 1);

	QHBoxLayout *hbox3 = new QHBoxLayout();
	hbox3->addStretch();
	buttonPrev = new QPushButton("&<<");
	hbox3->addWidget(buttonPrev);
	buttonNext = new QPushButton("&>>");
	hbox3->addWidget(buttonNext);
	gl1->addLayout(hbox3, 2, 0);
	addCellButton = new QPushButton(tr( "Add cell" ));
	gl1->addWidget(addCellButton, 2, 1);

	QGroupBox *gb = new QGroupBox();
	QVBoxLayout *vbox1 = new QVBoxLayout();
	vbox1->addLayout(hbox1);
	vbox1->addLayout(gl1);
	gb->setLayout(vbox1);
	gb->setSizePolicy(QSizePolicy (QSizePolicy::Preferred, QSizePolicy::Preferred));

	explain = new QTextEdit();
	explain->setReadOnly (true);
	explain->setSizePolicy(QSizePolicy (QSizePolicy::Preferred, QSizePolicy::Preferred));
	QPalette palette = explain->palette();
	palette.setColor(QPalette::Active, QPalette::Base, Qt::lightGray);
	explain->setPalette(palette);

	QHBoxLayout *hbox2 = new QHBoxLayout();
	hbox2->addWidget(explain);
	hbox2->addWidget(gb);

	commands = new ScriptEdit(scriptEnv);
    commands->setTabStopDistance(dynamic_cast<ApplicationWindow *>(parent)->d_notes_tab_length);
    commands->setFont(((ApplicationWindow *)parent)->d_notes_font);

	QVBoxLayout *vbox2 = new QVBoxLayout();
	btnApply = new QPushButton(tr( "&Apply" ));
	vbox2->addWidget(btnApply);
	buttonClearFormulas = new QPushButton(tr("Clear &Formulas" ));
	vbox2->addWidget(buttonClearFormulas);
	btnCancel = new QPushButton(tr( "&Close" ));
	vbox2->addWidget(btnCancel);
	vbox2->addStretch();

	QHBoxLayout *hbox4 = new QHBoxLayout();
	hbox4->addWidget(commands);
	hbox4->addLayout(vbox2);

	QVBoxLayout* vbox3 = new QVBoxLayout();
	vbox3->addLayout(hbox2);
#ifdef SCRIPTING_PYTHON
	boxMuParser = nullptr;
	if (env->objectName() != QString("muParser")){
		boxMuParser = new QCheckBox(tr("Use built-in muParser (much faster)"));
		boxMuParser->setChecked(((ApplicationWindow *)parent)->d_force_muParser);
		connect(boxMuParser, SIGNAL(toggled(bool)), this, SLOT(updateFunctionsList(bool)));
		updateFunctionsList(boxMuParser->isChecked());
		vbox3->addWidget(boxMuParser);
	}
#endif

	colNameLabel = new QLabel();
	vbox3->addWidget(colNameLabel);
	vbox3->addLayout(hbox4);

	setLayout(vbox3);
	setFocusProxy (commands);
	commands->setFocus();

	if (functions->count() > 0)
		insertExplain(0);

	connect(btnAddFunction, SIGNAL(clicked()),this, SLOT(insertFunction()));
	connect(btnAddCol, SIGNAL(clicked()),this, SLOT(insertCol()));
	connect(addCellButton, SIGNAL(clicked()),this, SLOT(insertCell()));
	connect(btnApply, SIGNAL(clicked()),this, SLOT(apply()));
	connect(btnCancel, SIGNAL(clicked()),this, SLOT(close()));
	connect(functions, SIGNAL(activated(int)),this, SLOT(insertExplain(int)));
	connect(buttonPrev, SIGNAL(clicked()), this, SLOT(prevColumn()));
	connect(buttonNext, SIGNAL(clicked()), this, SLOT(nextColumn()));
	connect(buttonClearFormulas, SIGNAL(clicked()), this, SLOT(clearFormulas()));
}

void SetColValuesDialog::prevColumn()
{
	int sc = table->selectedColumn();
	updateColumn(--sc);
}

void SetColValuesDialog::nextColumn()
{
	int sc = table->selectedColumn();
	updateColumn(++sc);
}

void SetColValuesDialog::updateColumn(int sc)
{
    if (sc < 0 || sc > table->numCols() - 1)
        return;

	if (sc == 0)
		buttonPrev->setEnabled(false);
	else
		buttonPrev->setEnabled(true);

	if (sc == table->numCols() - 1)
		buttonNext->setEnabled(false);
	else
		buttonNext->setEnabled(true);

	table->setSelectedCol(sc);
	table->table()->clearSelection();
	table->table()->selectColumn(sc);
	colNameLabel->setText("col(\""+table->colLabel(sc)+"\")= ");

	QStringList com = table->getCommands();
	commands->setText(com[sc]);
	QTextCursor cursor = commands->textCursor();
	cursor.movePosition(QTextCursor::End,QTextCursor::KeepAnchor);
}

QSize SetColValuesDialog::sizeHint() const
{
	return QSize( 400, 190 );
}

void SetColValuesDialog::customEvent(QEvent *e)
{
	if (e->type() == SCRIPTING_CHANGE_EVENT)
		scriptingChangeEvent((ScriptingChangeEvent*)e);
}

bool SetColValuesDialog::apply()
{
	int col = table->selectedColumn();
    if (col < 0 || col > table->numCols() - 1)
        return false;

	QString formula = commands->toPlainText();
	QString oldFormula = table->getCommands()[col];

	table->setCommand(col,formula);
	bool useMuParser = true;
#ifdef SCRIPTING_PYTHON
	if(boxMuParser)
		useMuParser = boxMuParser->isChecked();
#endif
    if (table->calculate(col, start->value() - 1, end->value() - 1, step->value(), useMuParser))
        return true;

	table->setCommand(col, oldFormula);
	return false;
}

void SetColValuesDialog::insertExplain(int index)
{
#ifdef SCRIPTING_PYTHON
	if (boxMuParser && boxMuParser->isChecked())
		explain->setText(muParserScripting::explainFunction(functions->itemText(index)));
	else
		explain->setText(scriptEnv->mathFunctionDoc(functions->itemText(index)));
#else
	explain->setText(scriptEnv->mathFunctionDoc(functions->itemText(index)));
#endif
}

void SetColValuesDialog::insertFunction()
{
	commands->insertFunction(functions->currentText());
}

void SetColValuesDialog::insertCol()
{
	commands->insertPlainText(boxColumn->currentText());
}

void SetColValuesDialog::insertCell()
{
	commands->insertPlainText(boxColumn->currentText().remove(")")+", i)");
}

void SetColValuesDialog::setTable(Table* w)
{
	table = w;
	QStringList colNames = w->colNames();
	int cols = w->numCols();
	for (int i=0; i<cols; i++)
		boxColumn->addItem("col(\""+colNames[i]+"\")", i);

	MySelection sel = w->table()->currentSelection();
    if (!sel.isEmpty()) {
        w->setSelectedCol(sel.leftColumn());

		start->setValue(sel.topRow() + 1);
		end->setValue(sel.bottomRow() + 1);
	} else {
		start->setValue(1);
		end->setValue(w->numRows());
	}

	updateColumn(w->selectedColumn());
	commands->setContext(w);

}

void SetColValuesDialog::setCompleter(QCompleter *completer)
{
    if (!completer)
        return;

    commands->setCompleter(completer);
}

void SetColValuesDialog::clearFormulas()
{
	if (!table)
		return;

	table->clearCommands();
	commands->clear();
}

void SetColValuesDialog::closeEvent(QCloseEvent* e)
{
#ifdef SCRIPTING_PYTHON
	if (boxMuParser){
		ApplicationWindow *app = (ApplicationWindow *)this->parent();
		if (app)
			app->d_force_muParser = boxMuParser->isChecked();
	}
#endif
	e->accept();
}

#ifdef SCRIPTING_PYTHON
void SetColValuesDialog::updateFunctionsList(bool muParser)
{
	functions->clear();
	if (muParser)
		functions->addItems(muParserScripting::functionsList(true));
	else
		functions->addItems(scriptingEnv()->mathFunctions());

	if (functions->count() > 0)
		insertExplain(0);
}
#endif
