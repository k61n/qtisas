/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Set matrix values dialog
 ******************************************************************************/

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>

#include "ApplicationWindow.h"
#include "MatrixCommand.h"
#include "MatrixValuesDialog.h"
#include "muParserScripting.h"

MatrixValuesDialog::MatrixValuesDialog( ScriptingEnv *env, QWidget* parent, Qt::WindowFlags fl )
: QDialog( parent, fl ), scripted(env)
{
    setObjectName("MatrixValuesDialog");
	setWindowTitle( tr( "QtiSAS - Set Matrix Values" ) );
	setSizeGripEnabled(true);
    setAttribute(Qt::WA_DeleteOnClose);

	QGridLayout *gl1 = new QGridLayout();
    gl1->addWidget(new QLabel(tr("For row (i)")), 0, 0);
	startRow = new QSpinBox();
	startRow->setRange(1, INT_MAX);
    gl1->addWidget(startRow, 0, 1);
	gl1->addWidget(new QLabel(tr("to")), 0, 2);
	endRow =  new QSpinBox();
	endRow->setRange(1, INT_MAX);
	gl1->addWidget(endRow, 0, 3);
	gl1->addWidget(new QLabel(tr("For col (j)")), 1, 0);
	startCol = new QSpinBox();
	startCol->setRange(1, INT_MAX);
	gl1->addWidget(startCol, 1, 1);
	gl1->addWidget(new QLabel(tr("to")), 1, 2);
	endCol = new QSpinBox();
	endCol->setRange(1, INT_MAX);
	gl1->addWidget(endCol, 1, 3);

	functions = new QComboBox();
	functions->addItems(scriptEnv->mathFunctions());
	btnAddFunction = new QPushButton(tr( "Add &Function" ));
	btnAddCell = new QPushButton(tr( "Add Ce&ll" ));

	QHBoxLayout *hbox1 = new QHBoxLayout();
	hbox1->addWidget(functions);
	hbox1->addWidget(btnAddFunction);
	hbox1->addWidget(btnAddCell);

	QVBoxLayout *vbox1 = new QVBoxLayout();
    vbox1->addLayout(gl1);
	vbox1->addLayout(hbox1);
	QGroupBox *gb = new QGroupBox();
    gb->setLayout(vbox1);
    gb->setSizePolicy(QSizePolicy (QSizePolicy::Preferred, QSizePolicy::Preferred));

	explain = new QTextEdit();
	explain->setReadOnly(true);
	explain->setSizePolicy(QSizePolicy (QSizePolicy::Preferred, QSizePolicy::Preferred));
    QPalette palette = explain->palette();
    palette.setColor(QPalette::Active, QPalette::Base, Qt::lightGray);
    explain->setPalette(palette);

	QHBoxLayout *hbox2 = new QHBoxLayout();
	hbox2->addWidget(explain);
	hbox2->addWidget(gb);

	QHBoxLayout *hbox3 = new QHBoxLayout();

	commands = new ScriptEdit( scriptEnv);
    commands->setTabStopDistance(dynamic_cast<ApplicationWindow *>(parent)->d_notes_tab_length);
    commands->setFont(((ApplicationWindow *)parent)->d_notes_font);
	commands->setFocus();
	hbox3->addWidget(commands);

	QVBoxLayout *vbox2 = new QVBoxLayout();
	btnApply = new QPushButton(tr( "&Apply" ));
    vbox2->addWidget(btnApply);
	btnCancel = new QPushButton(tr( "&Close" ));
    vbox2->addWidget(btnCancel);
    vbox2->addStretch();

	hbox3->addLayout(vbox2);

	QVBoxLayout* vbox3 = new QVBoxLayout(this);
	vbox3->addLayout(hbox2);
#ifdef SCRIPTING_PYTHON
	boxMuParser = nullptr;
	if (scriptEnv->objectName() != QString("muParser")){
		boxMuParser = new QCheckBox(tr("Use built-in muParser (much faster)"));
		boxMuParser->setChecked(((ApplicationWindow *)parent)->d_force_muParser);
		connect(boxMuParser, SIGNAL(toggled(bool)), this, SLOT(updateFunctionsList(bool)));
		updateFunctionsList(boxMuParser->isChecked());
		vbox3->addWidget(boxMuParser);
	}
#endif
	vbox3->addWidget(new QLabel(tr( "Cell(i,j)=" )));
	vbox3->addLayout(hbox3);

	insertExplain(0);

	connect(btnAddCell, SIGNAL(clicked()), this, SLOT(addCell()));
	connect(btnAddFunction, SIGNAL(clicked()), this, SLOT(insertFunction()));
	connect(btnApply, SIGNAL(clicked()), this, SLOT(apply()));
	connect(btnCancel, SIGNAL(clicked()), this, SLOT(close()));
	connect(functions, SIGNAL(activated(int)), this, SLOT(insertExplain(int)));
}

QSize MatrixValuesDialog::sizeHint() const
{
	return QSize( 400, 190 );
}

void MatrixValuesDialog::customEvent(QEvent *e)
{
	if (e->type() == SCRIPTING_CHANGE_EVENT)
		scriptingChangeEvent((ScriptingChangeEvent*)e);
}

bool MatrixValuesDialog::apply()
{
	QString formula = commands->toPlainText();
	QString oldFormula = matrix->formula();

	matrix->setFormula(formula);

	bool useMuParser = true;
#ifdef SCRIPTING_PYTHON
	if (boxMuParser)
		useMuParser = boxMuParser->isChecked();
#endif

	if (matrix->canCalculate(useMuParser))
    {
		matrix->undoStack()->push(new MatrixSetFormulaCommand(matrix, oldFormula, formula,
							tr("Set New Formula") + " \"" + formula + "\""));

		if (matrix->calculate(startRow->value()-1, endRow->value()-1,
			startCol->value()-1, endCol->value()-1, useMuParser))
			return true;
	}

	matrix->setFormula(oldFormula);
	return false;
}

void MatrixValuesDialog::setMatrix(Matrix* m)
{
    if (!m)
        return;

	matrix = m;
	commands->setText(m->formula());
	commands->setContext(m);

    endCol->setValue(m->numCols());
    endRow->setValue(m->numRows());

    if (m->viewType() == Matrix::TableView){
        QItemSelectionModel *selModel = m->selectionModel();
        if (selModel->hasSelection()){
            QItemSelectionRange selection = selModel->selection().first();
            if (selection.width() > 1 || selection.height() > 1){
                startCol->setValue(selection.left()+1);
                startRow->setValue(selection.top()+1);
                endCol->setValue(selection.right()+1);
                endRow->setValue(selection.bottom()+1);
            }
        }
    }
}

void MatrixValuesDialog::insertExplain(int index)
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

void MatrixValuesDialog::insertFunction()
{
	commands->insertFunction(functions->currentText());
}

void MatrixValuesDialog::addCell()
{
	commands->insertPlainText("cell(i, j)");
}

void MatrixValuesDialog::setCompleter(QCompleter *completer)
{
    if (!completer)
        return;

    commands->setCompleter(completer);
}

void MatrixValuesDialog::closeEvent(QCloseEvent* e)
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
void MatrixValuesDialog::updateFunctionsList(bool muParser)
{
	functions->clear();
	if (muParser)
		functions->addItems(muParserScripting::functionsList());
	else
		functions->addItems(scriptingEnv()->mathFunctions());

	if (functions->count() > 0)
		insertExplain(0);
}
#endif
