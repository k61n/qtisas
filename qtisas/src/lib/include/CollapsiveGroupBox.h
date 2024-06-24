/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A collapsive QGroupBox
 ******************************************************************************/

#ifndef COLLAPSIVEGROUPBOX_H
#define COLLAPSIVEGROUPBOX_H

#include <QGroupBox>

//! A collapsive QGroupBox.
/**
 *
 */
class CollapsiveGroupBox : public QGroupBox
{
	Q_OBJECT

public:
	//! Constructor.
	/**
	* \param parent parent widget (only affects placement of the widget)
	*/
	CollapsiveGroupBox(const QString & title = QString(), QWidget * parent = 0);

public slots:
	void setCollapsed (bool collapsed = true);
	void setExpanded (bool expanded = true);
};

#endif
