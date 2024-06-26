/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2006 Tilman Hoener zu Siederdissen <thzs@gmx.net>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: Title/axis label options dialog
 ******************************************************************************/

#include <QCheckBox>
#include <QComboBox>
#include <QFont>
#include <QFontDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTextCursor>
#include <QTextEdit>

#include <qwt/qwt_scale_widget.h>

#include "ApplicationWindow.h"
#include "ColorButton.h"
#include "FrameWidget.h"
#include "MultiLayer.h"
#include "TextDialog.h"
#include "TextFormatButtons.h"

TextDialog::TextDialog(TextType type, QWidget* parent, Qt::WindowFlags fl)
	: QDialog( parent, fl)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle( tr( "QtiSAS - Text options" ) );
	setSizeGripEnabled( true );

	d_graph = nullptr;
	d_scale = nullptr;

	d_text_type = type;

	groupBox1 = new QGroupBox();
	QGridLayout * topLayout = new QGridLayout(groupBox1);
	topLayout->addWidget(new QLabel(tr("Text Color")), 0, 0);

	colorBtn = new ColorButton();
	topLayout->addWidget(colorBtn, 0, 1);

	buttonFont = new QPushButton(tr( "&Font" ));
	buttonFont->setIcon(QIcon(":/font.png"));
	topLayout->addWidget(buttonFont, 0, 2);

	topLayout->addWidget(new QLabel(tr("Alignment")), 1, 0);
	alignmentBox = new QComboBox();
	alignmentBox->addItem( tr( "Center" ) );
	alignmentBox->addItem( tr( "Left" ) );
	alignmentBox->addItem( tr( "Right" ) );
	topLayout->addWidget(alignmentBox, 1, 1);

	if (type == AxisTitle){
		topLayout->addWidget(new QLabel(tr("Distance to axis")), 2, 0);
		distanceBox = new QSpinBox();
		distanceBox->setRange(0, 1000);
		connect(distanceBox, SIGNAL(valueChanged(int)), this, SLOT(apply()));

		topLayout->addWidget(distanceBox, 2, 1);
		invertTitleBox = new QCheckBox(tr("&Inverted"));
		invertTitleBox->hide();
		connect(invertTitleBox, SIGNAL(toggled(bool)), this, SLOT(apply()));
		topLayout->addWidget(invertTitleBox, 1, 2);
	}

	topLayout->setColumnStretch(3, 1);

    QVBoxLayout *vl = new QVBoxLayout();
    buttonApply = new QPushButton(tr( "&Apply to..." ));
	buttonApply->setDefault( true );
	vl->addWidget(buttonApply);

	formatApplyToBox = new QComboBox();
    formatApplyToBox->addItem(tr("Object"));
	formatApplyToBox->addItem(tr("Layer"));
    formatApplyToBox->addItem(tr("Window"));
    formatApplyToBox->addItem(tr("All Windows"));
	vl->addWidget(formatApplyToBox);

    buttonCancel = new QPushButton( tr( "&Close" ) );
	vl->addWidget(buttonCancel);
	vl->addStretch();

	QHBoxLayout *hl = new QHBoxLayout();
	hl->addWidget(groupBox1);
	hl->addLayout(vl);

	textEditBox = new QTextEdit();

	formatButtons = new TextFormatButtons(textEditBox, TextFormatButtons::AxisLabel);

	setFocusPolicy(Qt::StrongFocus);
	setFocusProxy(textEditBox);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(hl);
	mainLayout->addWidget(formatButtons);
	mainLayout->addWidget(textEditBox, 1);

	// signals and slots connections
	connect( buttonApply, SIGNAL( clicked() ), this, SLOT( apply() ) );
	connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
	connect( buttonFont, SIGNAL( clicked() ), this, SLOT(customFont() ) );
	connect( colorBtn, SIGNAL(colorChanged()), this, SLOT(apply()));
	connect( alignmentBox, SIGNAL(activated(int)), this, SLOT(apply()));
}

void TextDialog::setGraph(Graph *g)
{
	if (!g)
		return;

	d_graph = g;
	QwtText l;
	int axis = -1;
	if (d_text_type == LayerTitle)
		l = d_graph->title();
	else if (d_text_type == AxisTitle){
		d_scale = g->currentScale();
		if (!d_scale)
			return;

		l =	d_scale->title();
		switch(d_scale->alignment()){
			case QwtScaleDraw::BottomScale:
				setWindowTitle(tr("QtiSAS") + " - " + tr("X Axis Title"));
				axis = QwtPlot::xBottom;
			break;
			case QwtScaleDraw::LeftScale:
				setWindowTitle(tr("QtiSAS") + " - " + tr("Y Axis Title"));
				invertTitleBox->blockSignals(true);
				invertTitleBox->setChecked(d_scale->testLayoutFlag(QwtScaleWidget::TitleInverted));
				invertTitleBox->show();
				invertTitleBox->blockSignals(false);
				axis = QwtPlot::yLeft;
			break;
			case QwtScaleDraw::TopScale:
				setWindowTitle(tr("QtiSAS") + " - " + tr("Top Axis Title"));
				axis = QwtPlot::xTop;
			break;
			case QwtScaleDraw::RightScale:
				setWindowTitle(tr("QtiSAS") + " - " + tr("Right Axis Title"));
				invertTitleBox->blockSignals(true);
				invertTitleBox->setChecked(d_scale->testLayoutFlag(QwtScaleWidget::TitleInverted));
				invertTitleBox->show();
				invertTitleBox->blockSignals(false);
				axis = QwtPlot::yRight;
			break;
		}

		distanceBox->blockSignals(true);
		distanceBox->setValue(d_scale->spacing());
		distanceBox->blockSignals(false);
	}

	setAlignment(l.renderFlags());
	if (axis >= 0)
		setText(g->axisTitleString(axis));
	else
		setText(l.text());
	selectedFont = l.font();

	colorBtn->blockSignals(true);
	colorBtn->setColor(l.color());
	colorBtn->blockSignals(false);

	QFont fnt = selectedFont;
	fnt.setPointSize(QFont().pointSize() + 2);
	textEditBox->setFont(fnt);
}

void TextDialog::apply()
{
	if (!d_graph)
		return;

	if (d_text_type == AxisTitle){
		if (!d_scale)
			return;

		QwtText t =	d_scale->title();
		t.setRenderFlags(alignment());
		t.setText(textEditBox->toPlainText().replace("\n", "<br>"));
		t.setFont(selectedFont);
		t.setColor(colorBtn->color());
		d_scale->setTitle(t);
		d_scale->setSpacing(distanceBox->value());

		int axis = -1;
		switch(d_scale->alignment()){
			case QwtScaleDraw::BottomScale:
				axis = QwtPlot::xBottom;
			break;
			case QwtScaleDraw::LeftScale:
				d_scale->setLayoutFlag(QwtScaleWidget::TitleInverted, invertTitleBox->isChecked());
				d_scale->repaint();
				axis = QwtPlot::yLeft;
			break;
			case QwtScaleDraw::TopScale:
				axis = QwtPlot::xTop;
			break;
			case QwtScaleDraw::RightScale:
				d_scale->setLayoutFlag(QwtScaleWidget::TitleInverted, invertTitleBox->isChecked());
				d_scale->repaint();
				axis = QwtPlot::yRight;
			break;
		}
		d_graph->setAxisTitle(axis, t.text());
	} else if (d_text_type == LayerTitle){
		QwtText t =	d_graph->title();
		t.setRenderFlags(alignment());
		t.setText(textEditBox->toPlainText().replace("\n", "<br>"));
		t.setFont(selectedFont);
		t.setColor(colorBtn->color());
		d_graph->setTitle(t);
	}

	d_graph->notifyChanges();

	switch(formatApplyToBox->currentIndex()){
		case 0:
			d_graph->replot();
			d_graph->updateMarkersBoundingRect();
		break;

		case 1://this layer
			formatLayerLabels(d_graph);
		break;

		case 2://this window
		{
            QList<Graph *> layersLst = d_graph->multiLayer()->layersList();
			foreach(Graph *g, layersLst)
                formatLayerLabels(g);
		}
		break;

		case 3://all windows
		{
            ApplicationWindow *app = (ApplicationWindow *)this->parent();
		    QList<MdiSubWindow *> windows = app->windowsList();
			foreach(MdiSubWindow *w, windows){
				MultiLayer *ml = qobject_cast<MultiLayer *>(w);
				if (!ml)
					continue;

				QList<Graph *> layersLst = ml->layersList();
				foreach(Graph *g, layersLst)
                    formatLayerLabels(g);
			}
		}
		break;
	}

	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	if (app)
		app->setFormatBarFont(selectedFont);
}

void TextDialog::formatLayerLabels(Graph *g)
{
	if (!g)
		return;

	QColor tc = colorBtn->color();
	int align = alignment();

	for (int i = 0; i < QwtPlot::axisCnt; i++){
		QwtScaleWidget *scale = (QwtScaleWidget *)g->axisWidget(i);
		if (scale){
			QwtText t = scale->title();
			t.setColor(tc);
			t.setFont(selectedFont);
			t.setRenderFlags(align);
			scale->setTitle(t);
			if (d_text_type == AxisTitle){
				scale->setSpacing(distanceBox->value());
				if (i == QwtPlot::yRight || i == QwtPlot::yLeft){
					scale->setLayoutFlag(QwtScaleWidget::TitleInverted, invertTitleBox->isChecked());
					scale->repaint();
				}
			}
		}
	}

	QwtText t = g->title();
	t.setColor(tc);
	t.setFont(selectedFont);
	t.setRenderFlags(align);
	g->setTitle(t);
	g->replot();
	g->updateMarkersBoundingRect();
}

int TextDialog::alignment()
{
	int align = -1;
	switch (alignmentBox->currentIndex())
	{
		case 0:
			align = Qt::AlignHCenter;
			break;

		case 1:
			align = Qt::AlignLeft;
			break;

		case 2:
			align = Qt::AlignRight;
			break;
	}
	return align;
}

void TextDialog::setAlignment(int align)
{
	alignmentBox->blockSignals(true);
	switch(align){
		case Qt::AlignHCenter:
			alignmentBox->setCurrentIndex(0);
			break;
		case Qt::AlignLeft:
			alignmentBox->setCurrentIndex(1);
			break;
		case Qt::AlignRight:
			alignmentBox->setCurrentIndex(2);
			break;
	}
	alignmentBox->blockSignals(false);
}

void TextDialog::customFont()
{
	bool okF;
	QFont fnt = QFontDialog::getFont( &okF, selectedFont, this);
	if (okF && fnt != selectedFont){
		selectedFont = fnt;

		fnt.setPointSize(QFont().pointSize() + 2);
		textEditBox->setFont(fnt);
		apply();
	}
}

void TextDialog::setText(const QString & t)
{
	QTextCursor cursor = textEditBox->textCursor();
	// select the whole (old) text
	cursor.movePosition(QTextCursor::Start);
	cursor.movePosition(QTextCursor::End,QTextCursor::KeepAnchor);
	// replace old text
	cursor.insertText(t);
	// select the whole (new) text
	cursor.movePosition(QTextCursor::Start);
	cursor.movePosition(QTextCursor::End,QTextCursor::KeepAnchor);
	// this line makes the selection visible to the user
	// (the 2 lines above only change the selection in the
	// underlying QTextDocument)
	textEditBox->setTextCursor(cursor);
	// give focus back to text edit
	textEditBox->setFocus();
}
