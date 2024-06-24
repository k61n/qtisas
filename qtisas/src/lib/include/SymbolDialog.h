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

#ifndef SYMBOLDIALOG_H
#define SYMBOLDIALOG_H

#include <QButtonGroup>
#include <QDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QShortcut>
#include <QSizePolicy>
#include <QVBoxLayout>

//! Tools window to select special text characters
class SymbolDialog : public QDialog
{
    Q_OBJECT

public:
	//! Character set
	enum CharSet{
		lowerGreek = 0, /*!< lower case Greek letters */
		upperGreek = 1,  /*!< upper case Greek letters */
		mathSymbols = 2, /*!< mathematical symbols */
		arrowSymbols = 3, /*!< arrow symbols */
		numberSymbols = 4, /*!< number symbols (e.g. 1/2, vi)*/
		latexArrowSymbols = 5, /*!< default LaTeX arrow symbols */
		latexMathSymbols = 6 /*!< default LaTeX math symbols */
	};

	//! Constructor
	/**
	 * \param charSet character set (lower- or uppercase)
	 * \param parent parent widget
	 * \param fl window flags
	 */
    explicit SymbolDialog(CharSet charSet, QWidget *parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());

private:
	//! Show lowercase Greek characters
	void initLowerGreekChars();
	//! Show uppercase Greek characters
	void initUpperGreekChars();
	//! Show mathematical symbols
	void initMathSymbols();
	//! Show arrow symbols
	void initArrowSymbols();
	//! Show number symbols
	void initNumberSymbols();
	//! Show default LaTeX arrow symbols
	void initLatexArrowSymbols();
	//! Show default LaTeX math symbols
	void initLatexMathSymbols();

    QButtonGroup * buttons;
	QPushButton * closeButton;
	int numButtons;
	QVBoxLayout * mainLayout;
	QGridLayout * gridLayout;

protected:
	//! Event handler: When the dialog gets the focus the first button is set to react on [return]
	void focusInEvent( QFocusEvent * event );

public slots:
	//! Change language (reset title)
    virtual void languageChange();
	//! Find and emit char from pressed button
	void getChar(int btnIndex);
	//! Find and emit char from current button ([return] pressed)
	void addCurrentChar();

signals:
	//! Emitted when a letter is to be added
	void addLetter(const QString&);
};

#endif
