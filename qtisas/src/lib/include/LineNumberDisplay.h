/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: A widget displaying line numbers for a QTextEdit
 ******************************************************************************/

#ifndef LineNumberDisplay_H
#define LineNumberDisplay_H

#include <QTextEdit>

//! A QTextEdit displaying line numbers.
/**
 * It must be used in connection with another "source" QTextEdit.
 */
class LineNumberDisplay: public QTextEdit
{
    Q_OBJECT

public:
	//! Constructor
	/**
	* \param te the "source" QTextEdit for which we want to display the line numbers
	* \param parent parent widget (only affects placement of the dialog)
	*/
	LineNumberDisplay(QTextEdit *te, QWidget *parent = 0);
    int firstLineIncrement;
    bool addZeros;
public slots:
	void updateLineNumbers(bool force = false);
	void updateDocumentSelection();

private slots:
	void changeCharFormat (const QTextCharFormat &);

private:
	void showEvent(QShowEvent *);
	QTextEdit *d_text_edit;
};
#endif
