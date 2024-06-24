/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Tool for adding enrichments to a plot
 ******************************************************************************/

#ifndef ADDWIDGETTOOL_H
#define ADDWIDGETTOOL_H

#include <QAction>
#include <QObject>
#include <QPoint>

#include "FrameWidget.h"
#include "PlotToolInterface.h"

/*!Plot tool for adding enrichements.
 *
 * Provides selection of points on a Graph.
 */
class AddWidgetTool : public QObject, public PlotToolInterface
{
	Q_OBJECT
	public:
		enum WidgetType{
        	Text = 0,
        	TexEquation,
        	Rectangle,
			Ellipse,
        	UserWidget = 1000
    	};

		AddWidgetTool(WidgetType type, Graph *graph, QAction *d_action, const QObject *status_target = nullptr, const char *status_slot = "");
		virtual ~AddWidgetTool();

		virtual int rtti() const {return PlotToolInterface::Rtti_AddWidgetTool;};
		//! Returns the type of widget to be added
		WidgetType widgetType(){return d_widget_type;};

	signals:
		/*! Emitted whenever a new message should be presented to the user.
		 *
		 * You don't have to connect to this signal if you alreay specified a reciever during initialization.
		 */
		void statusText(const QString&);

	protected:
		void addEllipse(const QPoint& point);
        void addRectangle(const QPoint& point);
		void addEquation(const QPoint& point);
		void addText(const QPoint& point);
		void addWidget(const QPoint& point);

        virtual bool eventFilter(QObject *obj, QEvent *event);
		QAction *d_action;
		WidgetType d_widget_type;
		FrameWidget *d_fw;
};

#endif
