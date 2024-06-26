/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2007 Knut Franke <knut.franke@gmx.de>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Dialog for opening project files
 ******************************************************************************/

#ifndef OPENPROJECTDIALOG_H
#define OPENPROJECTDIALOG_H

#include <QComboBox>

#include "ExtensibleFileDialog.h"

class OpenProjectDialog : public ExtensibleFileDialog
{
	Q_OBJECT
	public:
		enum OpenMode { NewProject, NewFolder };
    explicit OpenProjectDialog(QWidget *parent = nullptr, bool extended = true,
                               Qt::WindowFlags flags = Qt::WindowFlags());
		OpenMode openMode() const { return (OpenMode) d_open_mode->currentIndex(); }

	private:
		QComboBox *d_open_mode;

    protected slots:
		void closeEvent(QCloseEvent* );
        //! Update which options are visible and enabled based on the output format.
        void updateAdvancedOptions (const QString &filter);
};

#endif
