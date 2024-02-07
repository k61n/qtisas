/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2007 Knut Franke <knut.franke@gmx.de>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: QFileDialog plus generic extension support
 ******************************************************************************/

#ifndef EXTENSIBLE_FILE_DIALOG_H
#define EXTENSIBLE_FILE_DIALOG_H

#include <QFileDialog>
#include <QPushButton>

//! QFileDialog plus generic extension support.
/**
 * This is a simple hack on top of QFileDialog that allows a custom extension widget to be added to
 * the bottom of the dialog. A button is provided for toggling display of this widget on/off.
 *
 * For the placement of button and extension widget, it is assumed that QFileDialog uses a
 * QGridLayout as its top-level layout. Other layouts will probably lead to a strange outlook,
 * although the functionality should stay intact.
 */
class ExtensibleFileDialog : public QFileDialog
{
	Q_OBJECT
	
	public:
		//! Constructor.
		/**
		 * \param parent parent widget (only affects placement of the dialog)
		 * \param extended flag: show/hide the advanced options on start-up
		 * \param flags window flags
		 */
    explicit ExtensibleFileDialog(QWidget *parent = nullptr, bool extended = true,
                                  Qt::WindowFlags flags = Qt::WindowFlags());
		//! Set the extension widget to be displayed when the user presses the toggle button.
		void setExtensionWidget(QWidget *extension);
	
		//! Tells weather the dialog has a valid extension widget
		bool isExtendable(){return d_extension != nullptr;};
		bool isExtended(){return d_extension_toggle->isChecked();};
		//! Toggle extension widget on/off
		void setExtended(bool extended);
		//! Sets the text to be displayed in the toggle button
		void setExtentionToggleButtonText(const QString& text){d_extension_toggle->setText(text);};
		//! Enables/Disables editing of the file type filter
		void setEditableFilter(bool on = true);
		
	private slots:
		void updateToggleButtonText(bool);

	protected:
		//! Button for toggling display of extension on/off.
		QPushButton *d_extension_toggle;

	private:
		//! The extension widget
		QWidget *d_extension;
		//! The layout row (of the assumed QGridLayout) used for extensions
		int d_extension_row;
};

#endif // ifndef EXTENSIBLE_FILE_DIALOG_H
