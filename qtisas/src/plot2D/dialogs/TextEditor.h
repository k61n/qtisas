/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A QwtText editor
 ******************************************************************************/

#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <QTextEdit>

class Graph;

class TextEditor: public QTextEdit
{
	Q_OBJECT

public:
    TextEditor(Graph *g);
    void formatText(const QString & prefix, const QString & postfix);

public slots:
    void addSymbol(const QString & letter);

private:
	void closeEvent(QCloseEvent *e);
	QString d_initial_text;
	QWidget *d_target;
	Graph *d_graph;
};

#endif
