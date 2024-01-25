/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
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
