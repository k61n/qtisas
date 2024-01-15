/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Title picker
 ******************************************************************************/

#include <QObject>

class Graph;
class QwtTextLabel;

class TitlePicker: public QObject
{
    Q_OBJECT

public:
    TitlePicker(Graph *plot);
    void setSelected(bool select = true);
	bool selected(){return d_selected;};

signals:
	void clicked();
	void doubleClicked();
	void removeTitle();
	void showTitleMenu();

private:
	bool eventFilter(QObject *, QEvent *);
	QwtTextLabel *title;
	bool d_selected;
};
