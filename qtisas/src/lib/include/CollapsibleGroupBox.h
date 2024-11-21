/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A collapsible QGroupBox
 ******************************************************************************/

#ifndef COLLAPSIBLEGROUPBOX_H
#define COLLAPSIBLEGROUPBOX_H

#include <QGroupBox>

//! A collapsible QGroupBox.
/**
 *
 */
class CollapsibleGroupBox : public QGroupBox
{
	Q_OBJECT

public:
	//! Constructor.
	/**
	* \param parent parent widget (only affects placement of the widget)
	*/
    explicit CollapsibleGroupBox(const QString &title = QString(), QWidget *parent = nullptr);

public slots:
	void setCollapsed (bool collapsed = true);
	void setExpanded (bool expanded = true);
};

#endif
