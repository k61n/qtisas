/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
Description: Widget with text format buttons (connected to a QTextEdit)
 ******************************************************************************/

#ifndef TEXTFORMATBUTTONS_H
#define TEXTFORMATBUTTONS_H

#include <QWidget>
class QTextEdit;
class QPushButton;

//! Widget with text format buttons (connected to a QTextEdit)
class TextFormatButtons : public QWidget
{
	Q_OBJECT

public:
	enum Buttons{Plot3D, AxisLabel, Legend, Equation, TexLegend};

	//! Constructor
	/**
	 * \param textEdit the QTextEdit that the buttons shall affect
	 * \param parent parent widget
	 */
	TextFormatButtons(QTextEdit * textEdit, Buttons buttons = Plot3D, QWidget * parent=0);
	void setButtons(Buttons btns);

private:
	QTextEdit *connectedTextEdit;
	Buttons d_buttons;
	//! Internal function: format selected text with prefix and postfix
	void formatText(const QString & prefix, const QString & postfix);
	void init(Buttons btns);

private slots:
	//! Format seleted text to fraction
	void addFraction();
	//! Format seleted text to square root
	void addSquareRoot();
	//! Format seleted text to subscript
	void addSubscript();
	//! Format seleted text to superscript
	void addSuperscript();
	//! Format seleted text to underlined
	void addUnderline();
	//! Format seleted text to italics
	void addItalics();
	//! Format seleted text to bold
	void addBold();
	//! Insert curve marker into the text
	void addCurve();

	//! Let the user insert lower case greek letters
	void showLowerGreek();
	//! Let the user insert capital greek letters
	void showUpperGreek();
	//! Let the user insert mathematical symbols
	void showMathSymbols();
	//! Let the user insert arrow symbols
	void showArrowSymbols();
	//! Insert 'letter' into the text
	void addSymbol(const QString& letter);
    //! Let the user insert Unicode symbols
    void showUnicodeSymbols();
};

#endif // TEXTFORMATBUTTONS_H
