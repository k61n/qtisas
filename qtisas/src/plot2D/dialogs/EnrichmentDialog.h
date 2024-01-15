/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: A general properties dialog for the FrameWidget, using article
             "Using a Simple Web Service with Qt" in Qt Quaterly, Issue 23, Q3 2007
 ******************************************************************************/

#ifndef TEXWIDGETDIALOG_H
#define TEXWIDGETDIALOG_H

#include <QDialog>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class QComboBox;
class QGroupBox;
class QHttp;
class QLabel;
class QPushButton;
class QTextEdit;
class QTabWidget;
class QCheckBox;
class QLineEdit;
class QCheckBox;
class QSpinBox;
class QSlider;
class QNetworkProxy;

class Graph;
class FrameWidget;
class LegendWidget;
class ColorButton;
class DoubleSpinBox;
class PatternBox;
class PenStyleBox;
class RectangleWidget;
class TextFormatButtons;
class ApplicationWindow;

class EnrichmentDialog : public QDialog
{
    Q_OBJECT

public:
	enum WidgetType{Frame, Text, Image, Tex, MDIWindow, Ellipse};

	EnrichmentDialog(WidgetType wt, Graph *g, ApplicationWindow *app, QWidget *parent = 0);
	~EnrichmentDialog();

	void setWidget(QWidget *w);
	void accept (){return apply();};

private slots:
    void clearForm();
    void fetchImage();
    void updateForm();
	void apply();
	void customButtons(int);
	void chooseImageFile(const QString& fn = QString::null);
	void displayCoordinates(int unit);
	void adjustHeight(double width);
	void adjustWidth(double height);
	void saveImagesInternally(bool save);
	void setBestSize();
	void frameApplyTo();
	void patternApplyTo();
	void textFormatApplyTo();
	void customFont();
	void updateTransparency(int alpha);
	void setTextDefaultValues();
	void setFrameDefaultValues();
	void setRectangleDefaultValues();
	void finishedCompiling(int, QProcess::ExitStatus);
	void displayCompileError(QProcess::ProcessError error);
	void createImage();
	void updateCompilerInterface(int);
	void updateButtons();

private:
	void initTextPage();
	void initEditorPage();
	void initImagePage();
	void initFramePage();
	void initGeometryPage();
	void initPatternPage();
	void setCoordinates(int unit);
	void setFrameTo(FrameWidget *fw);
	void setPatternTo(FrameWidget *r);
	void setTextFormatTo(LegendWidget *l);
	void setText(QTextEdit *editor, const QString & t);
	QString createTempTexFile();

	ApplicationWindow *d_app;
    QNetworkAccessManager nam;
    QNetworkReply *http;
    QProcess *compileProcess, *dvipngProcess;

    QLabel *outputLabel;
    QPushButton *clearButton;
    QPushButton *updateButton;
	QPushButton *cancelButton;
	QPushButton *bestSizeButton;
    QTextEdit *equationEditor, *textEditBox;
	QComboBox *frameBox;
	QTabWidget* tabWidget;
	QWidget *editPage, *framePage, *geometryPage, *imagePage, *patternPage, *textPage;
	ColorButton *frameColorBtn, *backgroundColorBtn, *patternColorBtn;
	QCheckBox *boxSaveImagesInternally;
	QLineEdit *imagePathBox;
	DoubleSpinBox *xBox, *yBox, *widthBox, *heightBox, *boxFrameWidth;
	QComboBox *unitBox;
	PenStyleBox *boxFrameLineStyle;
	QCheckBox *keepAspectBox, *useFrameColorBox;
	QSpinBox *boxTransparency, *boxTextAngle;
	PatternBox *patternBox;
	QPushButton *textApplyToBtn;
	QComboBox *frameApplyToBox, *patternApplyToBox, *textApplyToBox;
	ColorButton *textColorBtn, *textBackgroundBtn;
	QPushButton *textFontBtn;
	QSpinBox *boxBackgroundTransparency;
	QSlider *transparencySlider, *fillTransparencySlider;
	TextFormatButtons *formatButtons, *texFormatButtons;
	QPushButton *textDefaultBtn, *frameDefaultBtn, *rectangleDefaultBtn;
	QCheckBox *autoUpdateTextBox, *texOutputBox;
    QComboBox *texCompilerBox;
    QComboBox* attachToBox;

	QFont textFont;

	Graph *d_plot;
	QWidget *d_widget;
	WidgetType d_widget_type;
	double aspect_ratio;
};

#endif
