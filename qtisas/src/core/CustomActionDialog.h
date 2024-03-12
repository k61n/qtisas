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


class CustomXMLParser : public QXmlDefaultHandler
{
  public:
    CustomXMLParser();
    bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName,
                      const QXmlAttributes &attributes) override;
    bool characters(const QString &str) override;
    bool fatalError(const QXmlParseException &) override;
    QString errorString() const override;

  protected:
    bool metFitTag;
    QString currentText, errorStr, handlerType;
};


class CustomActionHandler : public CustomXMLParser
{
  public:
    explicit CustomActionHandler(QAction *action);
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) override;
    QString parentName();

  private:
    QString filePath, d_widget_name;
    QAction *d_action;
};


class CustomMenuHandler : public CustomXMLParser
{
  public:
    CustomMenuHandler();
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) override;
    QString location();
    QString title();

  private:
    QString d_location, d_title;
};
#endif
