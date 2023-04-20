/***************************************************************************
	File                 : ColorMapEditor.cpp
	Project              : QtiSAS
--------------------------------------------------------------------
    Copyright /QtiSAS/   : (C) 2012-2021 by Vitaliy Pipich
    Copyright /QtiPlot/  : (C) 2006-2012 by Ion Vasilief

	Email (use @ for *)  : v.pipich*gmail.com, ion_vasilief*yahoo.fr
	Description          : A QwtLinearColorMap Editor Widget
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include "ColorMapEditor.h"
#include <DoubleSpinBox.h>

#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QCheckBox>
#include <QColorDialog>
#include <QLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QComboBox>
#include <QString>
#include <QTextStream>
#include <QDir>
#include <iostream>

#include <math.h>
ColorMapEditor::ColorMapEditor(QStringList mapLst, int initCurrentMap, bool initCurrentLog, QString initMapPath, const QLocale& locale, int precision, QWidget* parent, Matrix *m0)
				: QWidget(parent),
				color_map(LinearColorMap()),
				min_val(0),
				max_val(1),
				d_locale(locale),
				d_precision(precision)
{
    
    //+++
    colorMaps=new QComboBox();
    colorMaps->addItems(mapLst);
    colorMaps->setCurrentIndex(initCurrentMap);
    connect(colorMaps, SIGNAL( activated(int) ), this, SLOT( colorMapsSelected(int) ) );
    mapPath=initMapPath;
    
    if (m0) m=m0; else m = NULL;
    //---
     
	table = new QTableWidget();
	table->setColumnCount(2);
	table->setSelectionMode(QAbstractItemView::SingleSelection);
	table->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	table->verticalHeader()->hide();
	table->horizontalHeader()->setClickable(false);
	table->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
	table->horizontalHeader()->setResizeMode(1, QHeaderView::Fixed);
	table->horizontalHeader()->setDefaultSectionSize(80);
	table->viewport()->setMouseTracking(true);
	table->viewport()->installEventFilter(this);
	table->setHorizontalHeaderLabels(QStringList() << tr("Level") << tr("Color"));
	table->setMinimumHeight(6*table->horizontalHeader()->height() + 2);
	table->installEventFilter(this);

	connect(table, SIGNAL(cellClicked (int, int)), this, SLOT(showColorDialog(int, int)));

	insertBtn = new QPushButton(tr("&Insert"));
	insertBtn->setEnabled(false);
	connect(insertBtn, SIGNAL(clicked()), this, SLOT(insertLevel()));

	deleteBtn = new QPushButton(tr("&Delete"));
	deleteBtn->setEnabled(false);
	connect(deleteBtn, SIGNAL(clicked()), this, SLOT(deleteLevel()));
    
//+++
    updateBtn = new QPushButton(tr("&Update"));
    updateBtn->setEnabled(true);
    connect(updateBtn, SIGNAL(clicked()), this, SLOT(updateScale()));
//---
    
	QHBoxLayout* hb = new QHBoxLayout();
	hb->addWidget(insertBtn);
	hb->addWidget(deleteBtn);
    hb->addWidget(updateBtn);
    hb->addStretch();

	scaleColorsBox = new QCheckBox(tr("&Scale Colors"));
	scaleColorsBox->setChecked(true);
	connect(scaleColorsBox, SIGNAL(toggled(bool)), this, SLOT(setScaledColors(bool)));
    
//+++
    scaleColorsBoxLog = new QCheckBox(tr("Log Scale of Colors"));
    if (m)
    {
    scaleColorsBoxLog->setChecked(initCurrentLog);
    connect(scaleColorsBoxLog, SIGNAL(toggled(bool)), this, SLOT(setScaledColorsLog(bool)));
    }
    else scaleColorsBoxLog->hide();
    
//---
    
    QHBoxLayout* hbCheck = new QHBoxLayout();
    hbCheck->addWidget(scaleColorsBox);
    hbCheck->addWidget(scaleColorsBoxLog);
    hbCheck->setSpacing(5);
    hbCheck->addStretch();
    
	QVBoxLayout* vl = new QVBoxLayout(this);
	vl->setSpacing(2);
    vl->addWidget(colorMaps);
	vl->addWidget(table);
	vl->addLayout(hb);
    vl->addLayout(hbCheck);
	setFocusProxy(table);
}

void ColorMapEditor::updateColorMap()
{
    int rows = table->rowCount();
    //
    QColor c_min = QColor(table->item(0, 1)->text());
    QColor c_max = QColor(table->item(rows - 1, 1)->text());
    LinearColorMap map(c_min, c_max);
    
    //+++
    double min_val_local=((DoubleSpinBox*)table->cellWidget(0, 0))->value();
    double max_val_local=((DoubleSpinBox*)table->cellWidget(rows-1, 0))->value();

    QwtDoubleInterval range = QwtDoubleInterval(0,1);
    map.setIntensityRange(range);
    
    for (int i = 1; i < rows - 1; i++)
    {
        double val= (((DoubleSpinBox*)table->cellWidget(i, 0))->value()- min_val_local)/(max_val_local-min_val_local);
        map.addColorStop (val, QColor(table->item(i, 1)->text()));
    }
    color_map = map;
    
    setScaledColors(scaleColorsBox->isChecked());
    //+++2020-05-28 modified
}

void ColorMapEditor::setColorMap(const LinearColorMap& map0)
{
    LinearColorMap map;
    map=map0;

    if (map0.colorStops().size() ==2 )
    {
        map.addColorStop (0, map0.color(0));
        map.addColorStop (1, map0.color(1));
    }
    
    QwtArray <double> colors = map.colorStops();

	bool scaledColors = (map.mode() == QwtLinearColorMap::ScaledColors);
	scaleColorsBox->setChecked(scaledColors);

	color_map = map;

	int rows = (int)colors.size();
    table->blockSignals(true);

    table->setRowCount(rows);
    
    if (m) m->range(&min_val,&max_val,false);
    else {min_val=0.0;max_val=1.0;};
    

	QwtDoubleInterval range = QwtDoubleInterval(min_val, max_val);
    double width = range.width();

    for (int i = 0; i < rows; i++)
    {
        DoubleSpinBox *sb = new DoubleSpinBox();
        sb->setLocale(d_locale);
        sb->setDecimals(d_precision);
        sb->setValue(min_val+colors[i]*width);
        
        if (i == 0)
        {
            sb->setPrefix("<= ");
            //sb->setValue(-1e308);
            sb->setDisabled(true);
        }else if (i == 1)
        {
            sb->setMaximum(max_val);
            connect(sb, SIGNAL(valueChanged(double)), this, SLOT(updateLowerRangeLimit(double)));
        }
        else if (i == rows - 2)
        {
            sb->setMinimum(min_val);
            connect(sb, SIGNAL(valueChanged(double)), this, SLOT(updateUpperRangeLimit(double)));
        }
        else if (i == rows - 1)
        {
            sb->setPrefix("> ");
            sb->setDisabled(true);
            //sb->setMinimum(min_val);
            connect(sb, SIGNAL(valueChanged(double)), this, SLOT(updateUpperRangeLimit(double)));//+++updateScale()));
        }
        else
        {
            sb->setMinimum(min_val);
            sb->setMaximum(max_val);
            //sb->setSuffix(" > " + QString::number(max_val*width,'f',1));
            //sb->setRange(min_val, max_val);
        }
        sb->setValue(min_val+colors[i]*width);

        connect(sb, SIGNAL(activated(DoubleSpinBox *)), this, SLOT(spinBoxActivated(DoubleSpinBox *)));

        table->setCellWidget(i, 0, sb);
        
        QColor c = color_map.color(i);
        
        QTableWidgetItem *it = new QTableWidgetItem(c.name());
        it->setFlags(it->flags() & ~Qt::ItemIsEditable);
        it->setBackground(QBrush(c));
        it->setForeground(QBrush(c));
        table->setItem(i, 1, it);
    }
    table->blockSignals(false);
}

void ColorMapEditor::updateLowerRangeLimit(double val)
{
    DoubleSpinBox *sb = (DoubleSpinBox*)table->cellWidget(1, 0);

    bool scale=scaleColorsBoxLog->isChecked();
    setScaledColorsLog(scale,false,true,false);
}

void ColorMapEditor::updateUpperRangeLimit(double val)
{
    int row = table->currentRow();
    DoubleSpinBox *sb = (DoubleSpinBox*)table->cellWidget(row-2, 0);
    
    bool scale=scaleColorsBoxLog->isChecked();
    setScaledColorsLog(scale,false,false,true);
}

void ColorMapEditor::setRange(double min, double max)
{
	min_val = qMin(min, max);
	max_val = qMax(min, max);
}

void ColorMapEditor::insertLevel()
{
	int row = table->currentRow();
    if (row == table->rowCount()-1) row = table->rowCount()-1;
    if (row <= 1) row = 2;

    DoubleSpinBox *sb = (DoubleSpinBox*)table->cellWidget(row, 0);
    if (!sb) return;
    double current_value = sb->value();
    double previous_value = min_val;
    sb = (DoubleSpinBox*)table->cellWidget(row - 1, 0);
    if (sb) previous_value = sb->value();
    
    double val = 0.5*(current_value + previous_value);
    QwtDoubleInterval range = QwtDoubleInterval(min_val, max_val);
    double mapped_val = (val - min_val)/range.width();
    
    QColor c = QColor(color_map.rgb(QwtDoubleInterval(0, 1), mapped_val));

    insertLevel(row, val, c);
    updateColorMap();
}

void ColorMapEditor::insertLevel(int row, double val, QColor c)
{
	table->blockSignals(true);
	table->insertRow(row);

	DoubleSpinBox *sb = new DoubleSpinBox();
	sb->setLocale(d_locale);
    sb->setDecimals(d_precision);
	sb->setRange(min_val, max_val);
    sb->setValue(val);
	connect(sb, SIGNAL(activated(DoubleSpinBox *)), this, SLOT(spinBoxActivated(DoubleSpinBox *)));
    table->setCellWidget(row, 0, sb);

	QTableWidgetItem *it = new QTableWidgetItem(c.name());
	it->setFlags(it->flags() & ~Qt::ItemIsEditable);
	it->setBackground(QBrush(c));
	it->setForeground(QBrush(c));
	table->setItem(row, 1, it);
	table->blockSignals(false);

	enableButtons(table->currentRow());
}

void ColorMapEditor::deleteLevel()
{
    int row=table->currentRow();
    deleteLevel(row);
    updateColorMap();
}

void ColorMapEditor::deleteLevel(int row)
{
    table->removeRow (row);
    enableButtons(row);
}

void ColorMapEditor::showColorDialog(int row, int col)
{
	if (col != 1)
		return;

	enableButtons(row);

	QColor c = QColor(table->item(row, 1)->text());
	QColor color = QColorDialog::getColor(c, this);
	if (!color.isValid() || color == c)
		return;

	table->item(row, 1)->setText(color.name());
	table->item(row, 1)->setForeground(QBrush(color));
	table->item(row, 1)->setBackground(QBrush(color));

	updateColorMap();
}

bool ColorMapEditor::eventFilter(QObject *object, QEvent *e)
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
			showColorDialog(table->currentRow(), 1);
			return true;
		}
	return false;
	}
	return QObject::eventFilter(object, e);
}

void ColorMapEditor::enableButtons(int row)
{
	if (row < 0)
		return;

	if (row < 2 || row >= table->rowCount()-2)
		deleteBtn->setEnabled(false);
	else
		deleteBtn->setEnabled(true);

	if (!row)
		insertBtn->setEnabled(false);
	else
		insertBtn->setEnabled(true);
    
    if (row < 2 || row >= table->rowCount()-1) insertBtn->setEnabled(false);
}

void ColorMapEditor::setScaledColors(bool scale)
{
	if (scale)
		color_map.setMode(QwtLinearColorMap::ScaledColors);
	else
		color_map.setMode(QwtLinearColorMap::FixedColors);

	scalingChanged();
}

//+++
void ColorMapEditor::setScaledColorsLog(bool scale)
{
    setScaledColorsLog(scale,true);
}

void ColorMapEditor::setScaledColorsLog(bool scale, bool fromCheckBox)
{
    setScaledColorsLog(scale,fromCheckBox,false,false);
}

void ColorMapEditor::setScaledColorsLog(bool scale, bool fromCheckBox, bool lowLimit, bool upperLimit)
{
    int rows=table->rowCount();
    int levels=rows-3;
    
    table->blockSignals(true);
    for (int i = 0; i < rows; i++) ((DoubleSpinBox*)table->cellWidget(i, 0))->blockSignals(true);

    double initMinAbs = ((DoubleSpinBox*)table->cellWidget(0, 0))->value();
    double initMaxAbs = ((DoubleSpinBox*)table->cellWidget(rows-1, 0))->value();
    double initMin = ((DoubleSpinBox*)table->cellWidget(1, 0))->value();
    double initMax = ((DoubleSpinBox*)table->cellWidget(rows-2, 0))->value();
    
    if (!lowLimit && !upperLimit)
    {
        double minvalue0, minvalue1,maxvalue;
        if (m!=NULL)
        {
            m->range(&minvalue0,&maxvalue,false);
            if (scale) m->range(&minvalue1,&maxvalue,true);
            else minvalue1=minvalue0;
            
            if (scale && maxvalue<=0 )
            {
                minvalue0=0;
                minvalue1=0.1;
                maxvalue=1.0;
            }
            else if (scale && minvalue1<=0 ) minvalue1=0.1*maxvalue;
        }
        else
        {
            minvalue0=0;
            minvalue1=0;
            if (scale) minvalue1=0.1;
            maxvalue=1.0;
        }
        
        if (!fromCheckBox || initMin<initMinAbs||initMax>initMaxAbs||(scale&&initMax<=0.0)||(scale&&initMin<=0.0)||initMin>initMax|| ( initMinAbs!=minvalue0||initMaxAbs!=maxvalue||(scale&&initMin<=0) ) )
        {
            initMinAbs=minvalue0;
            initMin=minvalue1;
            
            initMaxAbs=maxvalue;
            initMax=maxvalue;
        }
        
     }

    ((DoubleSpinBox*)table->cellWidget(1, 0))->setRange(initMinAbs, initMax);
    ((DoubleSpinBox*)table->cellWidget(rows-2, 0))->setRange(initMin, initMaxAbs);

    ((DoubleSpinBox*)table->cellWidget(0, 0))->setValue(initMinAbs);
    ((DoubleSpinBox*)table->cellWidget(rows-1, 0))->setValue(initMaxAbs);
    ((DoubleSpinBox*)table->cellWidget(1, 0))->setValue(initMin);
    ((DoubleSpinBox*)table->cellWidget(rows-2, 0))->setValue(initMax);
		
    
    min_val=initMinAbs;
    max_val=initMaxAbs;
    
    double delta= (initMax-initMin)/levels; // !log
    
    for (int i = 2; i < rows-2; i++)
    {
        DoubleSpinBox *sb = (DoubleSpinBox*)table->cellWidget(i, 0);
        sb->setRange(initMin, initMax);
        
        if (scale)
        {
            if (initMin==initMinAbs) sb->setValue(initMax*pow(10.0, ( -log10(initMax) + log10(initMin) ) * double(levels+1.0-i) / double(levels)));
            else
            {
            double scaledInitMin=(initMin-initMinAbs)/(initMaxAbs-initMinAbs);
            double scaledInitMax=(initMax-initMinAbs)/(initMaxAbs-initMinAbs);
            sb->setValue(initMinAbs+(initMaxAbs-initMinAbs)*scaledInitMax*pow(10.0, ( -log10(scaledInitMax) + log10(scaledInitMin) ) * double(levels+1.0-i) / double(levels)));
            }
        }
        else sb->setValue(initMin+delta*(i-1));
    }
    
    //+
    for (int i = 0; i < rows; i++) ((DoubleSpinBox*)table->cellWidget(i, 0))->blockSignals(false);
    table->blockSignals(false);
    //
    updateColorMap();
}
//---

void ColorMapEditor::spinBoxActivated(DoubleSpinBox *sb)
{
	if (!sb) return;

	int rows = table->rowCount();
	for (int i = 0; i < rows; i++)
    {
		DoubleSpinBox *box = (DoubleSpinBox*)table->cellWidget(i, 0);
		if (box && box == sb)
        {
			table->setCurrentCell(i, 0);
			enableButtons(i);
			return;
		}
	}
}


void ColorMapEditor::colorMapsSelected(int selected)
{

    QList<int> lR, lG, lB;
    int numberColors=0;
    
    if (colorMaps->currentIndex() > 6)
    {
        //+++
        QDir dd;
        QString colorPath=mapPath+"/colorMaps";
        colorPath=colorPath.replace("//","/");
        if (!dd.cd(colorPath))
        {
            colorPath=QDir::homePath()+"/colorMaps";
            colorPath=colorPath.replace("//","/");
            
            if (!dd.cd(colorPath)) return;
        };
        
        colorPath = dd.absolutePath();
        
        QFile f(colorPath+"/"+colorMaps->currentText()+".MAP");
        if ( !f.open( QIODevice::ReadOnly ) ) return;
        
        QTextStream t( &f );
        bool realColor;
        
        QString s;
        
        QRegExp rx("(\\d+)");
        int pos, NN, colorRGB;
        
        int nR, nG, nB;
        while(!t.atEnd())
        {
            s=t.readLine().trimmed();
            
            realColor=true;
            pos = 0;
            NN=0;
            
            while ( pos >= 0  && NN<3 && realColor)
            {
                pos = rx.indexIn( s, pos );
                
                colorRGB=rx.cap( 1 ).toInt();
                if (NN==0) nR=colorRGB;
                if (NN==1) nG=colorRGB;
                if (NN==2) nB=colorRGB;
                
                if ( pos > -1 && colorRGB>=0 && colorRGB<=255 )
                {
                    pos  += rx.matchedLength();
                    NN++;
                }
                else realColor=false;
            }
            if (NN==3 && realColor)
            {
                numberColors++;
                lR<<nR;lG<<nG;lB<<nB;
            }
        }
        numberColors=numberColors-2;
        f.close();
    }
    else if (colorMaps->currentIndex() == 1)
    {
        // white/red
        lR<<0; lG<<0;lB<<255;//"white"
        lR<<0; lG<<255;lB<<255;
        lR<<0; lG<<255;lB<<0; //"green"
        lR<<255; lG<<255;lB<<0; //"yellow"
        lR<<255; lG<<0;lB<<0; //"red"
        numberColors=3;
    }
    else if (colorMaps->currentIndex() == 2)
    {
        // black/white
        lR<<0; lG<<0;lB<<0; //"black"
        lR<<255; lG<<255;lB<<255; //"white"
        numberColors=0;
    }
    else if (colorMaps->currentIndex() == 3)
    {
        // black/white
        lR<<0; lG<<0;lB<<0; //"black"
        lR<<0; lG<<0;lB<<255; //"blue"
        lR<<0; lG<<255;lB<<0; //"green"
        lR<<255; lG<<255;lB<<0; //"yellow"
        lR<<255; lG<<0;lB<<255; //"magenta"
        lR<<255; lG<<0;lB<<0; //"red"
        lR<<255; lG<<255;lB<<255; //"white"
        numberColors=5;
    }
    else if (colorMaps->currentIndex() == 4)
    {
        // jet
        lR<<0; lG<<0; lB<<143;
        lR<<0; lG<<0; lB<<159;
        lR<<0; lG<<0; lB<<175;
        lR<<0; lG<<0; lB<<191;
        lR<<0; lG<<0; lB<<207;
        lR<<0; lG<<0; lB<<223;
        lR<<0; lG<<0; lB<<239;
        lR<<0; lG<<0; lB<<255;
        lR<<0; lG<<15; lB<<255;
        lR<<0; lG<<31; lB<<255;
        lR<<0; lG<<47; lB<<255;
        lR<<0; lG<<63; lB<<255;
        lR<<0; lG<<79; lB<<255;
        lR<<0; lG<<95; lB<<255;
        lR<<0; lG<<111; lB<<255;
        lR<<0; lG<<127; lB<<255;
        lR<<0; lG<<143; lB<<255;
        lR<<0; lG<<159; lB<<255;
        lR<<0; lG<<175; lB<<255;
        lR<<0; lG<<191; lB<<255;
        lR<<0; lG<<207; lB<<255;
        lR<<0; lG<<223; lB<<255;
        lR<<0; lG<<239; lB<<255;
        lR<<0; lG<<255; lB<<255;
        lR<<15; lG<<255; lB<<239;
        lR<<31; lG<<255; lB<<223;
        lR<<47; lG<<255; lB<<207;
        lR<<63; lG<<255; lB<<191;
        lR<<79; lG<<255; lB<<175;
        lR<<95; lG<<255; lB<<159;
        lR<<111; lG<<255; lB<<143;
        lR<<127; lG<<255; lB<<127;
        lR<<143; lG<<255; lB<<111;
        lR<<159; lG<<255; lB<<95;
        lR<<175; lG<<255; lB<<79;
        lR<<191; lG<<255; lB<<63;
        lR<<207; lG<<255; lB<<47;
        lR<<223; lG<<255; lB<<31;
        lR<<239; lG<<255; lB<<15;
        lR<<255; lG<<255; lB<<0;
        lR<<255; lG<<239; lB<<0;
        lR<<255; lG<<223; lB<<0;
        lR<<255; lG<<207; lB<<0;
        lR<<255; lG<<191; lB<<0;
        lR<<255; lG<<175; lB<<0;
        lR<<255; lG<<159; lB<<0;
        lR<<255; lG<<143; lB<<0;
        lR<<255; lG<<127; lB<<0;
        lR<<255; lG<<111; lB<<0;
        lR<<255; lG<<95; lB<<0;
        lR<<255; lG<<79; lB<<0;
        lR<<255; lG<<63; lB<<0;
        lR<<255; lG<<47; lB<<0;
        lR<<255; lG<<31; lB<<0;
        lR<<255; lG<<15; lB<<0;
        lR<<255; lG<<0; lB<<0;
        lR<<239; lG<<0; lB<<0;
        lR<<223; lG<<0; lB<<0;
        lR<<207; lG<<0; lB<<0;
        lR<<191; lG<<0; lB<<0;
        lR<<175; lG<<0; lB<<0;
        lR<<159; lG<<0; lB<<0;
        lR<<143; lG<<0; lB<<0;
        lR<<127; lG<<0; lB<<0;
        numberColors=63;
    }
    else if (colorMaps->currentIndex() == 5)
    {
        // jet-white
        lR<<255; lG<<255;lB<<255; //"white"
        lR<<0; lG<<0; lB<<143;
        lR<<0; lG<<0; lB<<159;
        lR<<0; lG<<0; lB<<175;
        lR<<0; lG<<0; lB<<191;
        lR<<0; lG<<0; lB<<207;
        lR<<0; lG<<0; lB<<223;
        lR<<0; lG<<0; lB<<239;
        lR<<0; lG<<0; lB<<255;
        lR<<0; lG<<15; lB<<255;
        lR<<0; lG<<31; lB<<255;
        lR<<0; lG<<47; lB<<255;
        lR<<0; lG<<63; lB<<255;
        lR<<0; lG<<79; lB<<255;
        lR<<0; lG<<95; lB<<255;
        lR<<0; lG<<111; lB<<255;
        lR<<0; lG<<127; lB<<255;
        lR<<0; lG<<143; lB<<255;
        lR<<0; lG<<159; lB<<255;
        lR<<0; lG<<175; lB<<255;
        lR<<0; lG<<191; lB<<255;
        lR<<0; lG<<207; lB<<255;
        lR<<0; lG<<223; lB<<255;
        lR<<0; lG<<239; lB<<255;
        lR<<0; lG<<255; lB<<255;
        lR<<15; lG<<255; lB<<239;
        lR<<31; lG<<255; lB<<223;
        lR<<47; lG<<255; lB<<207;
        lR<<63; lG<<255; lB<<191;
        lR<<79; lG<<255; lB<<175;
        lR<<95; lG<<255; lB<<159;
        lR<<111; lG<<255; lB<<143;
        lR<<127; lG<<255; lB<<127;
        lR<<143; lG<<255; lB<<111;
        lR<<159; lG<<255; lB<<95;
        lR<<175; lG<<255; lB<<79;
        lR<<191; lG<<255; lB<<63;
        lR<<207; lG<<255; lB<<47;
        lR<<223; lG<<255; lB<<31;
        lR<<239; lG<<255; lB<<15;
        lR<<255; lG<<255; lB<<0;
        lR<<255; lG<<239; lB<<0;
        lR<<255; lG<<223; lB<<0;
        lR<<255; lG<<207; lB<<0;
        lR<<255; lG<<191; lB<<0;
        lR<<255; lG<<175; lB<<0;
        lR<<255; lG<<159; lB<<0;
        lR<<255; lG<<143; lB<<0;
        lR<<255; lG<<127; lB<<0;
        lR<<255; lG<<111; lB<<0;
        lR<<255; lG<<95; lB<<0;
        lR<<255; lG<<79; lB<<0;
        lR<<255; lG<<63; lB<<0;
        lR<<255; lG<<47; lB<<0;
        lR<<255; lG<<31; lB<<0;
        lR<<255; lG<<15; lB<<0;
        lR<<255; lG<<0; lB<<0;
        lR<<239; lG<<0; lB<<0;
        lR<<223; lG<<0; lB<<0;
        lR<<207; lG<<0; lB<<0;
        lR<<191; lG<<0; lB<<0;
        lR<<175; lG<<0; lB<<0;
        lR<<159; lG<<0; lB<<0;
        lR<<143; lG<<0; lB<<0;
        lR<<127; lG<<0; lB<<0;
        numberColors=63;
    }
    else if (colorMaps->currentIndex() == 6)
    {
        lR<<60; lG<<0; lB<<80;
        lR<<60; lG<<0; lB<<80;
        lR<<60; lG<<0; lB<<84;
        lR<<64; lG<<0; lB<<84;
        lR<<64; lG<<0; lB<<84;
        lR<<64; lG<<0; lB<<88;
        lR<<64; lG<<0; lB<<88;
        lR<<68; lG<<0; lB<<88;
        lR<<68; lG<<0; lB<<92;
        lR<<68; lG<<0; lB<<92;
        lR<<68; lG<<0; lB<<92;
        lR<<72; lG<<0; lB<<96;
        lR<<72; lG<<0; lB<<96;
        lR<<72; lG<<0; lB<<96;
        lR<<72; lG<<0; lB<<100;
        lR<<76; lG<<0; lB<<100;
        lR<<76; lG<<0; lB<<100;
        lR<<76; lG<<0; lB<<104;
        lR<<76; lG<<0; lB<<104;
        lR<<76; lG<<0; lB<<104;
        lR<<80; lG<<0; lB<<104;
        lR<<80; lG<<0; lB<<108;
        lR<<80; lG<<0; lB<<108;
        lR<<80; lG<<0; lB<<108;
        lR<<84; lG<<0; lB<<112;
        lR<<84; lG<<0; lB<<112;
        lR<<84; lG<<0; lB<<112;
        lR<<84; lG<<0; lB<<116;
        lR<<88; lG<<0; lB<<116;
        lR<<88; lG<<0; lB<<116;
        lR<<88; lG<<0; lB<<120;
        lR<<88; lG<<0; lB<<120;
        lR<<92; lG<<0; lB<<120;
        lR<<92; lG<<0; lB<<124;
        lR<<92; lG<<0; lB<<124;
        lR<<92; lG<<0; lB<<124;
        lR<<96; lG<<0; lB<<128;
        lR<<96; lG<<0; lB<<128;
        lR<<96; lG<<0; lB<<128;
        lR<<96; lG<<0; lB<<132;
        lR<<96; lG<<0; lB<<132;
        lR<<100; lG<<0; lB<<132;
        lR<<100; lG<<0; lB<<132;
        lR<<100; lG<<0; lB<<136;
        lR<<100; lG<<0; lB<<136;
        lR<<104; lG<<0; lB<<136;
        lR<<104; lG<<0; lB<<140;
        lR<<104; lG<<0; lB<<140;
        lR<<104; lG<<0; lB<<140;
        lR<<108; lG<<0; lB<<144;
        lR<<108; lG<<0; lB<<144;
        lR<<108; lG<<0; lB<<144;
        lR<<108; lG<<0; lB<<148;
        lR<<112; lG<<0; lB<<148;
        lR<<112; lG<<0; lB<<148;
        lR<<112; lG<<0; lB<<152;
        lR<<112; lG<<0; lB<<152;
        lR<<116; lG<<0; lB<<152;
        lR<<116; lG<<0; lB<<156;
        lR<<116; lG<<0; lB<<156;
        lR<<116; lG<<0; lB<<156;
        lR<<120; lG<<0; lB<<160;
        lR<<120; lG<<0; lB<<160;
        lR<<124; lG<<4; lB<<160;
        lR<<124; lG<<8; lB<<164;
        lR<<128; lG<<12; lB<<164;
        lR<<128; lG<<16; lB<<164;
        lR<<132; lG<<20; lB<<168;
        lR<<132; lG<<24; lB<<168;
        lR<<136; lG<<28; lB<<168;
        lR<<136; lG<<32; lB<<172;
        lR<<140; lG<<36; lB<<172;
        lR<<140; lG<<40; lB<<172;
        lR<<144; lG<<44; lB<<176;
        lR<<144; lG<<48; lB<<176;
        lR<<148; lG<<52; lB<<180;
        lR<<148; lG<<56; lB<<180;
        lR<<152; lG<<60; lB<<180;
        lR<<152; lG<<64; lB<<184;
        lR<<156; lG<<68; lB<<184;
        lR<<156; lG<<72; lB<<184;
        lR<<160; lG<<76; lB<<188;
        lR<<160; lG<<80; lB<<188;
        lR<<164; lG<<84; lB<<188;
        lR<<164; lG<<88; lB<<192;
        lR<<168; lG<<92; lB<<192;
        lR<<168; lG<<96; lB<<192;
        lR<<172; lG<<100; lB<<196;
        lR<<172; lG<<104; lB<<196;
        lR<<176; lG<<108; lB<<200;
        lR<<176; lG<<112; lB<<200;
        lR<<180; lG<<116; lB<<200;
        lR<<180; lG<<120; lB<<204;
        lR<<184; lG<<124; lB<<204;
        lR<<188; lG<<128; lB<<204;
        lR<<188; lG<<132; lB<<208;
        lR<<192; lG<<136; lB<<208;
        lR<<192; lG<<140; lB<<208;
        lR<<196; lG<<144; lB<<212;
        lR<<196; lG<<148; lB<<212;
        lR<<200; lG<<152; lB<<216;
        lR<<200; lG<<156; lB<<216;
        lR<<204; lG<<160; lB<<216;
        lR<<204; lG<<164; lB<<220;
        lR<<208; lG<<168; lB<<220;
        lR<<208; lG<<172; lB<<220;
        lR<<212; lG<<176; lB<<224;
        lR<<212; lG<<180; lB<<224;
        lR<<216; lG<<184; lB<<224;
        lR<<216; lG<<188; lB<<228;
        lR<<220; lG<<192; lB<<228;
        lR<<220; lG<<196; lB<<228;
        lR<<224; lG<<200; lB<<232;
        lR<<224; lG<<204; lB<<232;
        lR<<228; lG<<208; lB<<236;
        lR<<228; lG<<212; lB<<236;
        lR<<232; lG<<216; lB<<236;
        lR<<232; lG<<220; lB<<240;
        lR<<236; lG<<224; lB<<240;
        lR<<236; lG<<228; lB<<240;
        lR<<240; lG<<232; lB<<244;
        lR<<240; lG<<236; lB<<244;
        lR<<244; lG<<240; lB<<244;
        lR<<244; lG<<244; lB<<248;
        lR<<248; lG<<248; lB<<248;
        lR<<252; lG<<252; lB<<252;
        lR<<252; lG<<252; lB<<252;
        lR<<252; lG<<252; lB<<248;
        lR<<252; lG<<252; lB<<244;
        lR<<252; lG<<252; lB<<240;
        lR<<252; lG<<252; lB<<236;
        lR<<252; lG<<252; lB<<232;
        lR<<252; lG<<252; lB<<228;
        lR<<252; lG<<252; lB<<224;
        lR<<252; lG<<252; lB<<220;
        lR<<252; lG<<252; lB<<216;
        lR<<252; lG<<252; lB<<212;
        lR<<252; lG<<252; lB<<208;
        lR<<252; lG<<252; lB<<204;
        lR<<252; lG<<252; lB<<200;
        lR<<252; lG<<252; lB<<196;
        lR<<252; lG<<252; lB<<192;
        lR<<252; lG<<252; lB<<188;
        lR<<252; lG<<252; lB<<184;
        lR<<252; lG<<252; lB<<180;
        lR<<252; lG<<252; lB<<176;
        lR<<252; lG<<252; lB<<172;
        lR<<252; lG<<252; lB<<168;
        lR<<252; lG<<252; lB<<164;
        lR<<252; lG<<252; lB<<160;
        lR<<252; lG<<252; lB<<156;
        lR<<252; lG<<252; lB<<152;
        lR<<252; lG<<252; lB<<148;
        lR<<252; lG<<252; lB<<144;
        lR<<252; lG<<252; lB<<140;
        lR<<252; lG<<252; lB<<136;
        lR<<252; lG<<252; lB<<132;
        lR<<252; lG<<252; lB<<128;
        lR<<252; lG<<252; lB<<124;
        lR<<252; lG<<252; lB<<120;
        lR<<252; lG<<252; lB<<116;
        lR<<252; lG<<252; lB<<112;
        lR<<252; lG<<252; lB<<108;
        lR<<252; lG<<252; lB<<104;
        lR<<252; lG<<252; lB<<100;
        lR<<252; lG<<252; lB<<96;
        lR<<252; lG<<252; lB<<92;
        lR<<252; lG<<252; lB<<88;
        lR<<252; lG<<252; lB<<84;
        lR<<252; lG<<252; lB<<80;
        lR<<252; lG<<252; lB<<76;
        lR<<252; lG<<252; lB<<72;
        lR<<252; lG<<252; lB<<68;
        lR<<252; lG<<252; lB<<64;
        lR<<252; lG<<252; lB<<60;
        lR<<252; lG<<252; lB<<56;
        lR<<252; lG<<252; lB<<52;
        lR<<252; lG<<252; lB<<48;
        lR<<252; lG<<252; lB<<44;
        lR<<252; lG<<252; lB<<40;
        lR<<252; lG<<252; lB<<36;
        lR<<252; lG<<252; lB<<32;
        lR<<252; lG<<252; lB<<28;
        lR<<252; lG<<252; lB<<24;
        lR<<252; lG<<252; lB<<20;
        lR<<252; lG<<252; lB<<16;
        lR<<252; lG<<252; lB<<12;
        lR<<252; lG<<252; lB<<8;
        lR<<252; lG<<252; lB<<4;
        lR<<252; lG<<252; lB<<0;
        lR<<252; lG<<248; lB<<0;
        lR<<248; lG<<244; lB<<0;
        lR<<244; lG<<240; lB<<0;
        lR<<240; lG<<236; lB<<4;
        lR<<240; lG<<232; lB<<4;
        lR<<236; lG<<228; lB<<4;
        lR<<232; lG<<224; lB<<8;
        lR<<228; lG<<220; lB<<8;
        lR<<228; lG<<216; lB<<8;
        lR<<224; lG<<212; lB<<12;
        lR<<220; lG<<208; lB<<12;
        lR<<216; lG<<204; lB<<12;
        lR<<212; lG<<200; lB<<16;
        lR<<212; lG<<196; lB<<16;
        lR<<208; lG<<192; lB<<16;
        lR<<204; lG<<188; lB<<20;
        lR<<200; lG<<184; lB<<20;
        lR<<200; lG<<180; lB<<20;
        lR<<196; lG<<176; lB<<24;
        lR<<192; lG<<172; lB<<24;
        lR<<188; lG<<168; lB<<24;
        lR<<184; lG<<164; lB<<28;
        lR<<184; lG<<160; lB<<28;
        lR<<180; lG<<156; lB<<28;
        lR<<176; lG<<152; lB<<32;
        lR<<172; lG<<148; lB<<32;
        lR<<172; lG<<144; lB<<32;
        lR<<168; lG<<140; lB<<36;
        lR<<164; lG<<136; lB<<36;
        lR<<160; lG<<132; lB<<36;
        lR<<160; lG<<128; lB<<36;
        lR<<156; lG<<124; lB<<40;
        lR<<152; lG<<120; lB<<40;
        lR<<148; lG<<116; lB<<40;
        lR<<144; lG<<112; lB<<44;
        lR<<144; lG<<108; lB<<44;
        lR<<140; lG<<104; lB<<44;
        lR<<136; lG<<100; lB<<48;
        lR<<132; lG<<96; lB<<48;
        lR<<132; lG<<92; lB<<48;
        lR<<128; lG<<88; lB<<52;
        lR<<124; lG<<84; lB<<52;
        lR<<120; lG<<80; lB<<52;
        lR<<116; lG<<76; lB<<56;
        lR<<116; lG<<72; lB<<56;
        lR<<112; lG<<68; lB<<56;
        lR<<108; lG<<64; lB<<60;
        lR<<104; lG<<60; lB<<60;
        lR<<104; lG<<56; lB<<60;
        lR<<100; lG<<52; lB<<64;
        lR<<96; lG<<48; lB<<64;
        lR<<92; lG<<44; lB<<64;
        lR<<88; lG<<40; lB<<68;
        lR<<88; lG<<36; lB<<68;
        lR<<84; lG<<32; lB<<68;
        lR<<80; lG<<28; lB<<72;
        lR<<76; lG<<24; lB<<72;
        lR<<76; lG<<20; lB<<72;
        lR<<72; lG<<16; lB<<76;
        lR<<68; lG<<12; lB<<76;
        lR<<64; lG<<8; lB<<76;
        lR<<60; lG<<0; lB<<80;
        lR<<60; lG<<0; lB<<80;
        lR<<60; lG<<0; lB<<80;
        lR<<60; lG<<0; lB<<80;
        numberColors=222;
    }
    else
    {
        // default
        lR<<0; lG<<0;lB<<255;
        lR<<255; lG<<0;lB<<0; //"red"
        numberColors=0;
    }
    
    //+++ first and last color
    QColor colF, colL;
    colF.setRgb(lR[0],lG[0],lB[0]);
    colL.setRgb(lR[numberColors+1],lG[numberColors+1],lB[numberColors+1]);
    
    colorMapToTable(lR,lG,lB);
    updateColorMap();
}

void ColorMapEditor::colorMapToTable(QList<int> lR, QList<int> lG, QList<int> lB)
{

    int levels=lR.size();
    int currentRowNumber=table->rowCount();
    
    double min_val_local=((DoubleSpinBox*)table->cellWidget(1, 0))->value();
    double max_val_local=((DoubleSpinBox*)table->cellWidget(currentRowNumber-2, 0))->value();
    
    if (currentRowNumber>4) for(int i=2;i<currentRowNumber-2;i++) deleteLevel(2);
    
    ((DoubleSpinBox*)table->cellWidget(1,0))->setValue(min_val_local);
    ((DoubleSpinBox*)table->cellWidget(2,0))->setValue(max_val_local);
    
    QColor c;
    c.setRgb(lR[0],lG[0],lB[0]);

    table->item(0, 1)->setText(c.name());
    table->item(0, 1)->setForeground(QBrush(c));
    table->item(0, 1)->setBackground(QBrush(c));
    
    table->item(1, 1)->setText(c.name());
    table->item(1, 1)->setForeground(QBrush(c));
    table->item(1, 1)->setBackground(QBrush(c));
    
    c.setRgb(lR[levels-1],lG[levels-1],lB[levels-1]);

    table->item(2, 1)->setText(c.name());
    table->item(2, 1)->setForeground(QBrush(c));
    table->item(2, 1)->setBackground(QBrush(c));

    table->item(3, 1)->setText(c.name());
    table->item(3, 1)->setForeground(QBrush(c));
    table->item(3, 1)->setBackground(QBrush(c));

    
    for(int i=2;i<levels;i++)
    {
        c.setRgb(lR[i-1],lG[i-1],lB[i-1]);
        insertLevel(i, min_val+(max_val_local-min_val_local)/(levels-1)*(i-1), c);
    }
    
    setScaledColorsLog(scaleColorsBoxLog->isChecked(),true,false,false);
}

void ColorMapEditor::updateScale()
{
    setScaledColorsLog(scaleColorsBoxLog->isChecked(),false,false,false);
}
//---//
