/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Custom action dialog
 ******************************************************************************/

#ifndef CustomActionDialog_H
#define CustomActionDialog_H

#include <QDialog>
#include <QXmlDefaultHandler>

class QGroupBox;
class QPushButton;
class QRadioButton;
class QComboBox;
class QListWidget;
class QLineEdit;
class QMenu;
class QToolBar;

class CustomActionDialog : public QDialog
{
    Q_OBJECT

public:
	//! Constructor
	/**
	 * \param parent parent widget (must be the application window!=
	 * \param fl window flags
	 */
    explicit CustomActionDialog(QWidget *parent, Qt::WindowFlags fl = Qt::WindowFlags());

private slots:
	void chooseIcon();
	void chooseFile();
	void chooseFolder();
	QAction* addAction();
	void removeAction();
	void setCurrentAction(int);
	void saveCurrentAction();
	void addMenu();
	void removeMenu();
	void enableDeleteMenuBtn(const QString &);

private:
	void init();
	void updateDisplayList();
	QAction* actionAt(int row);
	void saveAction(QAction *action);
	void customizeAction(QAction *action);
	bool validUserInput();
	void saveMenu(QMenu *menu);

	QStringList d_app_shortcut_keys;

	QList<QMenu *> d_menus;
	QList<QToolBar *> d_app_toolbars;

    QListWidget *itemsList;
    QPushButton *buttonCancel, *buttonAdd, *buttonRemove, *buttonSave;
    QPushButton *folderBtn, *fileBtn, *iconBtn;
    QLineEdit *folderBox, *fileBox, *iconBox, *textBox, *toolTipBox, *shortcutBox;
    QRadioButton *menuBtn, *toolBarBtn;
    QComboBox *menuBox, *toolBarBox;
    QPushButton *newMenuBtn, *removeMenuBtn;
};

class CustomActionHandler : public QXmlDefaultHandler
{
public:
    CustomActionHandler(QAction *action);

    bool startElement(const QString &namespaceURI, const QString &localName,
                       const QString &qName, const QXmlAttributes &attributes);
    bool endElement(const QString &namespaceURI, const QString &localName,
                     const QString &qName);
    bool characters(const QString &str){currentText += str; return true;};
    bool fatalError(const QXmlParseException &){return false;};
    QString errorString() const {return errorStr;};
	QString parentName(){return d_widget_name;};

private:
    bool metFitTag;
    QString currentText;
    QString errorStr;
    QString filePath;
	QString d_widget_name;
    QAction *d_action;
};

class CustomMenuHandler : public QXmlDefaultHandler
{
public:
    CustomMenuHandler();

    bool startElement(const QString &namespaceURI, const QString &localName,
                       const QString &qName, const QXmlAttributes &attributes);
    bool endElement(const QString &namespaceURI, const QString &localName,
                     const QString &qName);
    bool characters(const QString &str){currentText += str; return true;};
    bool fatalError(const QXmlParseException &){return false;};
    QString errorString() const {return errorStr;};
	QString location(){return d_location;};
	QString title(){return d_title;};

private:
    bool metFitTag;
    QString currentText;
    QString errorStr;
	QString d_location, d_title;
};
#endif
