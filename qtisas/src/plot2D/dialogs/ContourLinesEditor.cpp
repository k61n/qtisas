/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: A contour lines editor widget
 ******************************************************************************/

#include <QCheckBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QTableWidget>

#include "ColorButton.h"
#include "ContourLinesEditor.h"
#include "DoubleSpinBox.h"
#include "PenStyleBox.h"
#include "Spectrogram.h"

ContourLinesEditor::ContourLinesEditor(const QLocale& locale, int precision, QWidget* parent)
				: QWidget(parent),
				d_spectrogram(nullptr),
				d_locale(locale),
				d_precision(precision)
{
	table = new QTableWidget();
	table->setColumnCount(2);
	table->hideColumn(1);
	table->setSelectionMode(QAbstractItemView::SingleSelection);
	table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	table->horizontalHeader()->setSectionsClickable( false );
	table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	table->viewport()->setMouseTracking(true);
	table->viewport()->installEventFilter(this);
	table->setHorizontalHeaderLabels(QStringList() << tr("Level") << tr("Pen"));
	table->setMinimumHeight(6*table->horizontalHeader()->height() + 2);
	table->installEventFilter(this);

	connect(table, SIGNAL(cellClicked (int, int)), this, SLOT(showPenDialog(int, int)));

	insertBtn = new QPushButton(tr("&Insert"));
	insertBtn->setEnabled(false);
	connect(insertBtn, SIGNAL(clicked()), this, SLOT(insertLevel()));

	deleteBtn = new QPushButton(tr("&Delete"));
	deleteBtn->setEnabled(false);
	connect(deleteBtn, SIGNAL(clicked()), this, SLOT(deleteLevel()));

	QHBoxLayout* hb = new QHBoxLayout();
	hb->addWidget(insertBtn);
	hb->addWidget(deleteBtn);

	QVBoxLayout* vl = new QVBoxLayout(this);
	vl->setSpacing(0);
	vl->addWidget(table);
	vl->addLayout(hb);

	setFocusProxy(table);
	setMaximumWidth(200);

	penDialog = nullptr;
}

void ContourLinesEditor::updateContourLevels()
{
	if (!d_spectrogram)
		return;

	int rows = table->rowCount();
	QwtValueList levels;
	for (int i = 0; i < rows; i++)
		levels << ((DoubleSpinBox*)table->cellWidget(i, 0))->value();

	d_spectrogram->setContourLevels(levels);
}

void ContourLinesEditor::updateContourPens()
{
	if (!d_spectrogram)
		return;

	if (!table->isColumnHidden(1))
		d_spectrogram->setContourPenList(d_pen_list);
}

void ContourLinesEditor::setSpectrogram(Spectrogram *sp)
{
	if (!sp || d_spectrogram == sp)
		return;

	d_spectrogram = sp;
	updateContents();
}

void ContourLinesEditor::updateContents()
{
	if (!d_spectrogram)
		return;

	QwtValueList levels = d_spectrogram->contourLevels ();

	int rows = (int)levels.size();
	table->setRowCount(rows);
	table->blockSignals(true);

	QwtDoubleInterval range = d_spectrogram->data().range();
	for (int i = 0; i < rows; i++){
		DoubleSpinBox *sb = new DoubleSpinBox();
		sb->setLocale(d_locale);
		sb->setDecimals(d_precision);
		sb->setValue(levels[i]);
		sb->setRange(range.minValue (), range.maxValue ());
		connect(sb, SIGNAL(activated(DoubleSpinBox *)), this, SLOT(spinBoxActivated(DoubleSpinBox *)));
    	table->setCellWidget(i, 0, sb);

		QPen pen = d_spectrogram->defaultContourPen();
		if (pen.style() == Qt::NoPen)
			pen = d_spectrogram->contourPen (levels[i]);

		int width = 80;
		int height = 20;
    	QPixmap pix(width, height);
    	pix.fill(Qt::white);
    	QPainter paint(&pix);
    	paint.setRenderHint(QPainter::Antialiasing);
    	paint.setPen(pen);
    	paint.drawLine(0, height/2, width, height/2);
    	paint.end();

    	QLabel *lbl = new QLabel();
    	lbl->setPixmap(pix);

    	table->setCellWidget(i, 1, lbl);

    	d_pen_list << pen;
	}
	table->blockSignals(false);
}

void ContourLinesEditor::insertLevel()
{
	if (!d_spectrogram)
		return;

	int row = table->currentRow();
	DoubleSpinBox *sb = (DoubleSpinBox*)table->cellWidget(row, 0);
	if (!sb)
		return;

	QwtDoubleInterval range = d_spectrogram->data().range();
	double current_value = sb->value();
	double previous_value = range.minValue ();
	sb = (DoubleSpinBox*)table->cellWidget(row - 1, 0);
	if (sb)
		previous_value = sb->value();

	double val = 0.5*(current_value + previous_value);

	table->blockSignals(true);
	table->insertRow(row);

	sb = new DoubleSpinBox();
	sb->setLocale(d_locale);
	sb->setDecimals(d_precision);
	sb->setValue(val);
	sb->setRange(range.minValue (), range.maxValue ());
	connect(sb, SIGNAL(activated(DoubleSpinBox *)), this, SLOT(spinBoxActivated(DoubleSpinBox *)));
    table->setCellWidget(row, 0, sb);

	QPen pen = d_spectrogram->defaultContourPen();
	if (pen.style() == Qt::NoPen)
		pen = d_spectrogram->contourPen (val);

	int width = 80;
	int height = 20;
	QPixmap pix(width, height);
	pix.fill(Qt::white);
	QPainter paint(&pix);
	paint.setRenderHint(QPainter::Antialiasing);
	paint.setPen(pen);
	paint.drawLine(0, height/2, width, height/2);
	paint.end();

	QLabel *lbl = new QLabel();
	lbl->setPixmap(pix);

	table->setCellWidget(row, 1, lbl);
	table->blockSignals(false);

	enableButtons(table->currentRow());
	d_pen_list.insert(row, pen);
}

void ContourLinesEditor::deleteLevel()
{
	int index = table->currentRow();
	table->removeRow (index);

	if (index >=0 && index < d_pen_list.size())
		d_pen_list.removeAt(index);
}

void ContourLinesEditor::showPenDialog(int row, int col)
{
	if (!d_spectrogram || col != 1)
		return;

	enableButtons(row);

	QPen pen = d_pen_list[row];

	if (!penDialog){
		penDialog = new QDialog(this);
		penDialog->setWindowTitle(tr("QtiSAS - Edit pen"));

		QGroupBox *gb1 = new QGroupBox();
		QGridLayout *hl1 = new QGridLayout(gb1);

		hl1->addWidget(new QLabel(tr("Color")), 0, 0);
		penColorBox = new ColorButton();
		penColorBox->setColor(pen.color());
		hl1->addWidget(penColorBox, 0, 1);

		applyAllColorBox = new QCheckBox(tr("Apply to all"));
		hl1->addWidget(applyAllColorBox, 0, 2);

		hl1->addWidget(new QLabel(tr("Style")), 1, 0);
		penStyleBox = new PenStyleBox;
		penStyleBox->setStyle(pen.style());
		hl1->addWidget(penStyleBox, 1, 1);

		applyAllStyleBox = new QCheckBox(tr("Apply to all"));
		hl1->addWidget(applyAllStyleBox, 1, 2);

		hl1->addWidget(new QLabel(tr("Width")), 2, 0);
		penWidthBox = new DoubleSpinBox();
		penWidthBox->setValue(pen.widthF());
		hl1->addWidget(penWidthBox, 2, 1);
		hl1->setRowStretch(3, 1);

		applyAllWidthBox = new QCheckBox(tr("Apply to all"));
		hl1->addWidget(applyAllWidthBox, 2, 2);

		QPushButton *acceptPenBtn = new QPushButton(tr("&Ok"));
		connect(acceptPenBtn, SIGNAL(clicked()), this, SLOT(updatePen()));

		QPushButton *closeBtn = new QPushButton(tr("&Close"));
		connect(closeBtn, SIGNAL(clicked()), penDialog, SLOT(reject()));

		QHBoxLayout *hl2 = new QHBoxLayout();
		hl2->addStretch();
		hl2->addWidget(acceptPenBtn);
		hl2->addWidget(closeBtn);

		QVBoxLayout *vl = new QVBoxLayout(penDialog);
		vl->addWidget(gb1);
		vl->addLayout(hl2);
	} else {
		penColorBox->setColor(pen.color());
		penStyleBox->setStyle(pen.style());
		penWidthBox->setValue(pen.widthF());
	}

	d_pen_index = row;
	penDialog->exec();
}

void ContourLinesEditor::updatePenColumn()
{
	table->blockSignals(true);
	for (int i = 0; i < table->rowCount(); i++){
		int width = 80;
		int height = 20;
    	QPixmap pix(width, height);
    	pix.fill(Qt::white);
    	QPainter paint(&pix);
    	paint.setRenderHint(QPainter::Antialiasing);
    	paint.setPen(d_pen_list[i]);
    	paint.drawLine(0, height/2, width, height/2);
    	paint.end();

    	QLabel *lbl = new QLabel();
    	lbl->setPixmap(pix);

    	table->setCellWidget(i, 1, lbl);
	}
	table->blockSignals(false);
}

void ContourLinesEditor::updatePen()
{
	QPen pen = QPen(penColorBox->color(), penWidthBox->value(), penStyleBox->style());

	d_pen_list[d_pen_index] = pen;

	if (applyAllColorBox->isChecked()){
		for (int i = 0; i < d_pen_list.size(); i++){
			QPen p = d_pen_list[i];
			p.setColor(penColorBox->color());
			d_pen_list[i] = p;
		}
	}

	if (applyAllStyleBox->isChecked()){
		for (int i = 0; i < d_pen_list.size(); i++){
			QPen p = d_pen_list[i];
			p.setStyle(penStyleBox->style());
			d_pen_list[i] = p;
		}
	}

	if (applyAllWidthBox->isChecked()){
		for (int i = 0; i < d_pen_list.size(); i++){
			QPen p = d_pen_list[i];
			p.setWidthF(penWidthBox->value());
			d_pen_list[i] = p;
		}
	}

	updatePenColumn();
	penDialog->close();
}

bool ContourLinesEditor::eventFilter(QObject *object, QEvent *e)
{
	if (e->type() == QEvent::MouseMove && object == table->viewport()){
        const QMouseEvent *me = (const QMouseEvent *)e;
        QPoint pos = table->viewport()->mapToParent(me->pos());
        int row = table->rowAt(pos.y() - table->horizontalHeader()->height());
        if (table->columnAt(pos.x()) == 1 && row >= 0 && row < table->rowCount())
            setCursor(QCursor(Qt::PointingHandCursor));
        else
            setCursor(QCursor(Qt::ArrowCursor));
        return true;
	} else if (e->type() == QEvent::Leave && object == table->viewport()){
		setCursor(QCursor(Qt::ArrowCursor));
		return true;
	} else if (e->type() == QEvent::KeyPress && object == table){
		QKeyEvent *ke = (QKeyEvent *)e;
		if (ke->key() == Qt::Key_Return && table->currentColumn() == 1){
			showPenDialog(table->currentRow(), 1);
			return true;
		}
	return false;
	}
	return QObject::eventFilter(object, e);
}

void ContourLinesEditor::enableButtons(int row)
{
	if (row < 0)
		return;

	deleteBtn->setEnabled(true);
	insertBtn->setEnabled(true);
}

void ContourLinesEditor::spinBoxActivated(DoubleSpinBox *sb)
{
	if (!sb)
		return;

	int rows = table->rowCount();
	for (int i = 0; i < rows; i++){
		DoubleSpinBox *box = (DoubleSpinBox*)table->cellWidget(i, 0);
		if (box && box == sb){
			table->setCurrentCell(i, 0);
			enableButtons(i);
			return;
		}
	}
}

void ContourLinesEditor::showPenColumn(bool on)
{
	if (on)
		table->showColumn(1);
	else
		table->hideColumn(1);
}

ContourLinesEditor::~ContourLinesEditor()
{
	if(penDialog)
        delete penDialog;
}
