/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Knut Franke <knut.franke@gmx.de>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Line options dialog
 ******************************************************************************/

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>

#include "ApplicationWindow.h"
#include "ArrowMarker.h"
#include "ColorButton.h"
#include "DoubleSpinBox.h"
#include "Graph.h"
#include "LineDialog.h"
#include "PenStyleBox.h"

LineDialog::LineDialog( ArrowMarker *line, QWidget* parent,  Qt::WindowFlags fl )
    : QDialog( parent, fl )
{
    setWindowTitle( tr( "QtiSAS - Line options" ) );
	setAttribute(Qt::WA_DeleteOnClose);
	setSizeGripEnabled( true );

	lm = line;

	QGroupBox *gb1 = new QGroupBox();
    QGridLayout *gl1 = new QGridLayout();

    gl1->addWidget(new QLabel(tr("Color")), 0, 0);
	colorBox = new ColorButton();
	colorBox->setColor(lm->color());
	gl1->addWidget(colorBox, 0, 1);

	gl1->addWidget(new QLabel(tr("Type")), 1, 0);
    styleBox = new PenStyleBox();
	styleBox->setStyle(lm->style());
	gl1->addWidget(styleBox, 1, 1);

	gl1->addWidget(new QLabel(tr("Width")), 2, 0);
    widthBox = new DoubleSpinBox('f');
	widthBox->setLocale(((ApplicationWindow *)parent)->locale());
	widthBox->setSingleStep(0.1);
    widthBox->setRange(0, 100);
	widthBox->setValue(lm->width());
	gl1->addWidget(widthBox, 2, 1);

	startBox = new QCheckBox();
    startBox->setText( tr( "Arrow at &start" ) );
	startBox->setChecked(lm->hasStartArrow());
	gl1->addWidget(startBox, 3, 0);

	endBox = new QCheckBox();
    endBox->setText( tr( "Arrow at &end" ) );
	endBox->setChecked(lm->hasEndArrow());
	gl1->addWidget(endBox, 3, 1);
	gl1->setRowStretch(4, 1);

	gb1->setLayout(gl1);

	QHBoxLayout* hl1 = new QHBoxLayout();
    hl1->addWidget(gb1);

	options = new QWidget();
    options->setLayout(hl1);

	tw = new QTabWidget();
	tw->addTab(options, tr( "&Line" ) );

    QGroupBox *gb2 = new QGroupBox();
    QGridLayout *gl2 = new QGridLayout();

    gl2->addWidget(new QLabel(tr("Length")), 0, 0);
	boxHeadLength = new QSpinBox();
	boxHeadLength->setValue(lm->headLength());
	gl2->addWidget(boxHeadLength, 0, 1);

	gl2->addWidget(new QLabel(tr( "Angle" )), 1, 0 );
	boxHeadAngle = new QSpinBox();
	boxHeadAngle->setRange(0, 90);
	boxHeadAngle->setSingleStep(5);
	boxHeadAngle->setValue(lm->headAngle());
	gl2->addWidget(boxHeadAngle, 1, 1);

	filledBox = new QCheckBox();
    filledBox->setText( tr( "&Filled" ) );
	filledBox->setChecked(lm->filledArrowHead());
	gl2->addWidget(filledBox, 2, 1);
	gl2->setRowStretch(3, 1);

	gb2->setLayout(gl2);

	QHBoxLayout* hl2 = new QHBoxLayout();
    hl2->addWidget(gb2);

    head = new QWidget();
    head->setLayout(hl2);
	tw->addTab(head, tr("Arrow &Head"));

	initGeometryTab();

	buttonDefault = new QPushButton( tr( "Set &Default" ) );
	btnApply = new QPushButton( tr( "&Apply" ) );
	btnOk = new QPushButton(tr( "&Ok" ) );
    btnOk->setDefault(true);

    QBoxLayout *bl1 = new QBoxLayout (QBoxLayout::LeftToRight);
    bl1->addStretch();
	bl1->addWidget(buttonDefault);
	bl1->addWidget(btnApply);
	bl1->addWidget(btnOk);

	QVBoxLayout* vl = new QVBoxLayout();
    vl->addWidget(tw);
	vl->addLayout(bl1);
	setLayout(vl);

	enableHeadTab();

	connect(colorBox, SIGNAL(colorChanged()), this, SLOT(apply()));
	connect(styleBox, SIGNAL(activated(int)), this, SLOT(apply()));
	connect(widthBox, SIGNAL(valueChanged(double)), this, SLOT(apply()));
	connect(startBox, SIGNAL(toggled(bool)), this, SLOT(apply()));
	connect(endBox, SIGNAL(toggled(bool)), this, SLOT(apply()));

	connect(boxHeadLength, SIGNAL(valueChanged(int)), this, SLOT(apply()));
	connect(boxHeadAngle, SIGNAL(valueChanged(int)), this, SLOT(apply()));
	connect(filledBox, SIGNAL(toggled(bool)), this, SLOT(apply()));

	connect(btnOk, SIGNAL(clicked()), this, SLOT(accept()));
	connect(btnApply, SIGNAL(clicked()), this, SLOT(apply()));
    connect(tw, SIGNAL(currentChanged(int)), this, SLOT(enableButtonDefault(int)));
	connect(buttonDefault, SIGNAL(clicked()), this, SLOT(setDefaultValues()));
}

void LineDialog::initGeometryTab()
{
	QGridLayout *bl1 = new QGridLayout();

	attachToBox = new QComboBox();
	attachToBox->addItem(tr("Page"));
	attachToBox->addItem(tr("Layer Scales"));
	attachToBox->setCurrentIndex((int)lm->attachPolicy());

	bl1->addWidget(new QLabel(tr( "Attach to" )), 0, 0);
	bl1->addWidget(attachToBox, 0, 1);

	unitBox = new QComboBox();
	unitBox->addItem(tr("Scale Coordinates"));
	unitBox->addItem(tr("Pixels"));

	bl1->addWidget(new QLabel(tr( "Unit" )), 1, 0);
	bl1->addWidget(unitBox, 1, 1);

	ApplicationWindow *app = (ApplicationWindow *)parent();
	QLocale locale = QLocale();
	if (app)
		locale = app->locale();

    QGroupBox *gb1 = new QGroupBox(tr("Start Point"));
	xStartBox = new DoubleSpinBox();
	xStartBox->setLocale(locale);
	xStartBox->setDecimals(6);
	yStartBox = new DoubleSpinBox();
	yStartBox->setLocale(locale);
	yStartBox->setDecimals(6);

	xStartPixelBox = new QSpinBox();
	xStartPixelBox->setRange(-INT_MAX, INT_MAX);
	yStartPixelBox = new QSpinBox();
	yStartPixelBox->setRange(-INT_MAX, INT_MAX);

    QGridLayout *gl1 = new QGridLayout();
    gl1->addWidget(new QLabel( tr("X")), 0, 0);
    gl1->addWidget(xStartBox, 0, 1);
	gl1->addWidget(xStartPixelBox, 0, 1);
    gl1->addWidget(new QLabel(tr("Y")), 1, 0);
    gl1->addWidget(yStartBox, 1, 1);
	gl1->addWidget(yStartPixelBox, 1, 1);
	gl1->setColumnStretch(1, 10);
	gl1->setRowStretch(2, 1);
    gb1->setLayout(gl1);

    QGroupBox *gb2 = new QGroupBox(tr("End Point"));
    xEndBox = new DoubleSpinBox();
	xEndBox->setLocale(locale);
	xEndBox->setDecimals(6);
	yEndBox = new DoubleSpinBox();
	yEndBox->setLocale(locale);
	yEndBox->setDecimals(6);

	xEndPixelBox = new QSpinBox();
	xEndPixelBox->setRange(-INT_MAX, INT_MAX);
	yEndPixelBox = new QSpinBox();
	yEndPixelBox->setRange(-INT_MAX, INT_MAX);

    QGridLayout *gl2 = new QGridLayout();
    gl2->addWidget(new QLabel( tr("X")), 0, 0);
    gl2->addWidget(xEndBox, 0, 1);
	gl2->addWidget(xEndPixelBox, 0, 1);

    gl2->addWidget(new QLabel(tr("Y")), 1, 0);
    gl2->addWidget(yEndBox, 1, 1);
	gl2->addWidget(yEndPixelBox, 1, 1);
	gl2->setColumnStretch(1, 10);
	gl2->setRowStretch(2, 1);
    gb2->setLayout(gl2);

    QBoxLayout *bl2 = new QBoxLayout (QBoxLayout::LeftToRight);
	bl2->addWidget(gb1);
	bl2->addWidget(gb2);

	QVBoxLayout* vl = new QVBoxLayout();
    vl->addLayout(bl1);
    vl->addLayout(bl2);
    vl->addStretch();

    geometry = new QWidget();
    geometry->setLayout(vl);
	tw->addTab(geometry, tr( "&Geometry" ) );

	connect(unitBox, SIGNAL(activated(int)), this, SLOT(displayCoordinates(int)));
	displayCoordinates(0);
}

void LineDialog::displayCoordinates(int unit)
{
	if (unit == ScaleCoordinates){
		QwtDoublePoint sp = lm->startPointCoord();
		xStartBox->setValue(sp.x());
		xStartBox->show();
		xStartPixelBox->hide();
		yStartBox->setValue(sp.y());
		yStartBox->show();
		yStartPixelBox->hide();

		QwtDoublePoint ep = lm->endPointCoord();
		xEndBox->setValue(ep.x());
		xEndBox->show();
		xEndPixelBox->hide();
		yEndBox->setValue(ep.y());
		yEndBox->show();
		yEndPixelBox->hide();
	} else {
		QPoint startPoint = lm->startPoint();
		QPoint endPoint = lm->endPoint();

		xStartBox->hide();
		xStartPixelBox->setValue(startPoint.x());
		xStartPixelBox->show();

		yStartBox->hide();
		yStartPixelBox->setValue(startPoint.y());
		yStartPixelBox->show();

		xEndBox->hide();
		xEndPixelBox->setValue(endPoint.x());
		xEndPixelBox->show();

		yEndBox->hide();
		yEndPixelBox->setValue(endPoint.y());
		yEndPixelBox->show();
	}
}

void LineDialog::setCoordinates(int unit)
{
	if (unit == ScaleCoordinates){
		lm->setStartPoint(xStartBox->value(), yStartBox->value());
		lm->setEndPoint(xEndBox->value(), yEndBox->value());
	} else {
		lm->setStartPoint(QPoint(xStartPixelBox->value(), yStartPixelBox->value()));
		lm->setEndPoint(QPoint(xEndPixelBox->value(), yEndPixelBox->value()));
	}
}

void LineDialog::apply()
{
    if (tw->currentWidget() == (QWidget *)options){
        lm->setStyle(styleBox->style());
        lm->setColor(colorBox->color());
        lm->setWidth(widthBox->value());
        lm->drawEndArrow(endBox->isChecked());
        lm->drawStartArrow(startBox->isChecked());
	} else if (tw->currentWidget() == (QWidget *)head){
        if (lm->headLength() != boxHeadLength->value())
            lm->setHeadLength( boxHeadLength->value() );

        if (lm->headAngle() != boxHeadAngle->value())
            lm->setHeadAngle( boxHeadAngle->value() );

        if (lm->filledArrowHead() != filledBox->isChecked())
            lm->fillArrowHead( filledBox->isChecked() );
	} else if (tw->currentWidget() == (QWidget *)geometry){
		lm->setAttachPolicy((ArrowMarker::AttachPolicy)attachToBox->currentIndex());
        setCoordinates(unitBox->currentIndex());
	}

	Graph *g = (Graph *)lm->plot();
	g->replot();
	g->multiLayer()->notifyChanges();

	enableHeadTab();
}

void LineDialog::accept()
{
	apply();
	close();
}

void LineDialog::enableHeadTab()
{
    if (startBox->isChecked() || endBox->isChecked())
        tw->setTabEnabled(tw->indexOf(head), true);
    else
        tw->setTabEnabled(tw->indexOf(head), false);
}

void LineDialog::setDefaultValues()
{
ApplicationWindow *app = (ApplicationWindow *)this->parent();
if (!app)
	return;

app->setArrowDefaultSettings(widthBox->value(), colorBox->color(), styleBox->style(),
							boxHeadLength->value(), boxHeadAngle->value(), filledBox->isChecked());
}

void LineDialog::enableButtonDefault(int)
{
    QWidget *w = tw->currentWidget();

    if (w == geometry)
        buttonDefault->setEnabled(false);
    else
        buttonDefault->setEnabled(true);
}