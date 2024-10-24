/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Tool window to select special text characters
 ******************************************************************************/

#include <QButtonGroup>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QPushButton>
#include <QShortcut>
#include <QSizePolicy>

#include "SymbolDialog.h"

SymbolDialog::SymbolDialog(CharSet charSet, QWidget* parent, Qt::WindowFlags fl )
: QDialog( parent, fl )
{
    setAttribute(Qt::WA_DeleteOnClose);
	setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	setSizeGripEnabled( false );

	buttons = new QButtonGroup(this);
	mainLayout = new QVBoxLayout(this);
	gridLayout = new QGridLayout();

	switch(charSet){
		case lowerGreek:
			initLowerGreekChars();
		break;
		case upperGreek:
			initUpperGreekChars();
		break;
		case mathSymbols:
			initMathSymbols();
		break;
		case arrowSymbols:
			initArrowSymbols();
		break;
		case numberSymbols:
			initNumberSymbols();
		break;
		case latexArrowSymbols:
			initLatexArrowSymbols();
		break;
		case latexMathSymbols:
			initLatexMathSymbols();
		break;
	}

	closeButton = new QPushButton(tr("&Close"), this);

	mainLayout->addLayout( gridLayout );
	mainLayout->addStretch();
	mainLayout->addWidget( closeButton );

	languageChange();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(buttons, SIGNAL(buttonClicked(int)), this, SLOT(getChar(int)));
#else
    connect(buttons, &QButtonGroup::idClicked, this, &SymbolDialog::getChar);
#endif
	connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
	QShortcut *shortcut = new QShortcut(Qt::Key_Return, this);
	connect( shortcut , SIGNAL(activated()), this, SLOT(addCurrentChar()) );
}

void SymbolDialog::initLowerGreekChars()
{
	int counter = 0;
	for (int i=0 ; i <= (0x3C9-0x3B1) ; i++,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x3B1)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/5,counter%5);
	}
}

void SymbolDialog::initUpperGreekChars()
{
	int i, counter = 0;
	for ( i=0; i <= (0x394-0x393); i++,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x393)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn, counter + 1);
		gridLayout->addWidget(btn, counter/5,counter%5);
	}

	for ( i=0; i <= (0x3A0-0x398); i+=3,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x398)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn, counter + 1);
		gridLayout->addWidget(btn, counter/5,counter%5);
	}

	for ( i=0; i <= (0x3A6-0x3A0); i+=3,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x3A0)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn, counter + 1);
		gridLayout->addWidget(btn, counter/5,counter%5);
	}


	for ( i=0 ; i <= (0x3A9-0x3A8) ; i++,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x3A8)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/5,counter%5);
	}

	numButtons = counter;
}

void SymbolDialog::initNumberSymbols()
{
	int i, counter = 0;
	for ( i=0 ; i <= (0x216B-0x2153) ; i++,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2153)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}
	for ( i=0 ; i <= (0x217B-0x2170) ; i++,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2170)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}
	numButtons = counter;
}

void SymbolDialog::initMathSymbols()
{
	int i, counter = 0;
	for ( i=0 ; i <= (0x220D-0x2200) ; i++,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2200)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}
	for ( i=0 ; i <= (0x2211-0x220F) ; i++,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x220F)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}
	for ( i=0 ; i <= (0x00B1-0x00B1) ; i++,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x00B1)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}
	for ( i=0 ; i <= (0x2213-0x2213) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2213)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}
	// MULTIPLICATION SIGN
	for ( i=0; i <= (0x00D7-0x00D7) ; i++,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x00D7)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}

	for ( i=0 ; i <= (0x221E - 0x2217) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2217)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}
	for ( i=0 ; i <= (0x2222-0x2222) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2222)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}
	for ( i=0 ; i <= (0x2230-0x2227) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2227)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}
	for ( i=0 ; i <= (0x223F-0x223F) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x223F)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}

	for ( i=0 ; i <= (0x2245-0x2245) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2245)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}
	for ( i=0 ; i <= (0x2248-0x2248) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2248)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}
	for ( i=0 ; i <= (0x2259-0x2259) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2259)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}

	for ( i=0 ; i <= (0x2255-0x2254) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2254)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}

	for ( i=0 ; i <= (0x2267-0x225F) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x225F)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}

	// < SIGN
	for ( i=0; i < 1 ; i++,counter++ ){
		QPushButton *btn = new QPushButton(QString("<"));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}

	for ( i=0 ; i <= (0x226B-0x226A) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x226A)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}

	for ( i=0 ; i <= (0x2289-0x2282) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2282)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}

	// h bar
	for ( i=0 ; i <= (0x210F-0x210F) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x210F)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}

	// angstrom
	for ( i=0 ; i <= (0x212B-0x212B) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x212B)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}
	// per mille and per ten thousand (thanks to Knut Franke)
	for ( i=0; i <= (0x2031-0x2030) ; i++,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2030)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/8,counter%8);
	}
	numButtons = counter;
}

void SymbolDialog::initLatexMathSymbols()
{
	int i, counter = 0;
	for ( i=0 ; i <= (0x2200-0x2200) ; i++,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2200)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/7,counter%7);
	}
	for ( i=0 ; i <= (0x2209-0x2202) ; i++,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2202)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/7,counter%7);
	}
	for ( i=0 ; i <= (0x220C-0x220B) ; i++,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x220B)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/7,counter%7);
	}
	for ( i=0 ; i <= (0x2211-0x220F) ; i++,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x220F)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/7,counter%7);
	}
	for ( i=0 ; i <= (0x00B1-0x00B1) ; i++,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x00B1)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/7,counter%7);
	}
	for ( i=0 ; i <= (0x2213-0x2213) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2213)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/7,counter%7);
	}
	// MULTIPLICATION SIGN
	for ( i=0; i <= (0x00D7-0x00D7) ; i++,counter++ ){
		QPushButton *btn = new QPushButton(QString(QChar(i+0x00D7)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/7,counter%7);
	}

	for ( i=0 ; i <= (0x221E - 0x2217) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2217)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/7,counter%7);
	}
	for ( i=0 ; i < (0x222F-0x2227) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2227)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/7,counter%7);
	}
	for ( i=0 ; i <= (0x223F-0x223F) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x223F)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/7,counter%7);
	}
	for ( i=0 ; i <= (0x2245-0x2245) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2245)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/7,counter%7);
	}
	for ( i=0 ; i <= (0x2248-0x2248) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2248)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/7,counter%7);
	}
	for ( i=0 ; i <= (0x2265-0x2260) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2260)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/7,counter%7);
	}
	for ( i=0 ; i <= (0x226B-0x226A) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x226A)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/7,counter%7);
	}
	for ( i=0 ; i <= (0x2289-0x2282) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2282)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/7,counter%7);
	}

	// h bar
	for ( i=0 ; i <= (0x210F-0x210F) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x210F)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/7,counter%7);
	}

	// angstrom
	for ( i=0 ; i <= (0x212B-0x212B) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x212B)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/7,counter%7);
	}
	numButtons = counter;

	QFont font = this->font();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QStringList families = QFontDatabase().families();
#else
    QStringList families = QFontDatabase::families();
#endif
	foreach(QString f, families){
		if (f.contains("Unicode")){
			font.setFamily(f);

			for(int i = 0; i < numButtons; i++){
				QWidget *w = gridLayout->itemAt(i)->widget();
				if (w)
					w->setFont(font);
			}
			break;
		}
	}
}

void SymbolDialog::initArrowSymbols()
{
	int i, counter = 0;
	for ( i=0 ; i <= (0x219B-0x2190) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2190)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/6,counter%6);
	}
	for ( i=0 ; i <= (0x21A7-0x21A4) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x21A4)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/6,counter%6);
	}
	for ( i=0 ; i <= (0x21D5-0x21CD) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x21CD)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/6,counter%6);
	}
	for ( i=0 ; i <= (0x21E9-0x21E6) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x21E6)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/6,counter%6);
	}
    for ( i=0 ; i <= (0x27E9-0x27E8) ; i++,counter++ )
    {
        QPushButton *btn = new QPushButton(QString(QChar(i+0x27E8)));
        btn->setMaximumWidth(40);
        btn->setFlat ( true );
        btn->setAutoDefault (false);
        buttons->addButton(btn,counter+1);
        gridLayout->addWidget(btn,counter/6,counter%6);
    }
	numButtons = counter;
}

void SymbolDialog::initLatexArrowSymbols()
{
	QFont font = this->font();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QStringList families = QFontDatabase().families();
#else
    QStringList families = QFontDatabase::families();
#endif
	foreach(QString f, families){
		if (f.contains("Unicode")){
			font.setFamily(f);
			break;
		}
	}

	int i, counter = 0;
	for ( i=0 ; i <= (0x2199-0x2190) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x2190)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setFont(font);
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/6,counter%6);
	}

	for ( i=0 ; i <= (0x21AA-0x21A9) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x21A9)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setFont(font);
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/6,counter%6);
	}

	for ( i=0 ; i <= (0x21D5-0x21D0) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x21D0)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setFont(font);
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/6,counter%6);
	}

	for ( i=0 ; i <= (0x21BD-0x21BC) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x21BC)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setFont(font);
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/6,counter%6);
	}

	for ( i=0 ; i <= (0x21C1-0x21C0) ; i++,counter++ )
	{
		QPushButton *btn = new QPushButton(QString(QChar(i+0x21C0)));
		btn->setMaximumWidth(40);
		btn->setFlat ( true );
		btn->setFont(font);
		btn->setAutoDefault (false);
		buttons->addButton(btn,counter+1);
		gridLayout->addWidget(btn,counter/6,counter%6);
	}

	QPushButton *btn = new QPushButton(QString(QChar(0x21CC)));
	btn->setMaximumWidth(40);
	btn->setFlat ( true );
	btn->setFont(font);
	btn->setAutoDefault (false);
	buttons->addButton(btn,counter+1);
	gridLayout->addWidget(btn,counter/6,counter%6);

	counter++;

	btn = new QPushButton(QString(QChar(0x21A6)));
	btn->setMaximumWidth(40);
	btn->setFlat ( true );
	btn->setFont(font);
	btn->setAutoDefault (false);
	buttons->addButton(btn,counter+1);
	gridLayout->addWidget(btn,counter/6,counter%6);

    counter++;
    
    for ( i=0 ; i <= (0x27E9-0x27E8) ; i++,counter++ )
    {
        QPushButton *btn = new QPushButton(QString(QChar(i+0x27E8)));
        btn->setMaximumWidth(40);
        btn->setFlat ( true );
        btn->setAutoDefault (false);
        buttons->addButton(btn,counter+1);
        gridLayout->addWidget(btn,counter/6,counter%6);
    }
    
	numButtons = counter;
}

void SymbolDialog::addCurrentChar()
{
	for (int i=1; i < numButtons; i++)
	{
		QPushButton *btn = (QPushButton *) buttons->button(i);
		if (btn && btn->hasFocus())
			emit addLetter(btn->text());
	}
}

void SymbolDialog::getChar(int btnIndex)
{
	QPushButton * btn = (QPushButton *)buttons->button( btnIndex );
	if(btn)
		emit addLetter(btn->text().replace("<", "&lt;"));
}


void SymbolDialog::languageChange()
{
	setWindowTitle( tr( "QtiSAS - Choose Symbol" ) );
}


void SymbolDialog::focusInEvent( QFocusEvent * event )
{
	Q_UNUSED(event)
	// select the first button as default (in case [return] is pressed)
	((QPushButton *)buttons->button(1))->setFocus(Qt::TabFocusReason);
}
