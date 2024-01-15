/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: A widget displaying images in 2D plots
 ******************************************************************************/

#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#include <QWidget>
#include "FrameWidget.h"

class ApplicationWindow;

class ImageWidget: public FrameWidget
{
	Q_OBJECT

public:
    //! Construct an image widget from a file name.
	ImageWidget(Graph *, const QString& fn = QString::null);
	//! Construct an image widget from a QImage.
	ImageWidget(Graph *, const QImage& image);

	//! Return the pixmap to be drawn.
	QPixmap pixmap() const {return d_pix;};
	void setPixmap(const QPixmap&);

    //! Import image from #d_file_name. Returns true if successful.
	bool load(const QString& fn, bool update = true);
	//! Return #d_file_name.
	QString fileName(){return d_file_name;};
	
	bool saveInternally(){return d_save_xpm;};
	void setSaveInternally(bool save = true){d_save_xpm = save;};

	void print(QPainter *p, const QwtScaleMap map[QwtPlot::axisCnt]);
	virtual QString saveToString();

	void clone(ImageWidget* t);
	static void restore(Graph *g, const QStringList& lst);

	//! Return d_window_name.
	QString windowName(){return d_window_name;};
	void setWindowName(const QString& name){d_window_name = name;};

	static QPixmap windowPixmap(ApplicationWindow *mw, const QString& name, const QSize& size);
        void paintEvent(QPaintEvent *e);

private:
	void draw(QPainter *painter, const QRect& r);
//	void paintEvent(QPaintEvent *e);
	QPixmap d_pix;
	//! The file from which the image was loaded.
	QString d_file_name;
	//! Flag telling if the pixmap must be saved in the .qti project as XPM
	bool d_save_xpm;
	//! The window whos image is drawn.
	QString d_window_name;
};

#endif
