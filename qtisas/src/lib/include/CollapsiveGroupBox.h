/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2010 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: A collapsive QGroupBox
 ******************************************************************************/

#ifndef COLLAPSIVE_GROUP_BOX__H
#define COLLAPSIVE_GROUP_BOX__H

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
