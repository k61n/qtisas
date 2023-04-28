/***************************************************************************
File                   : compile-info.cpp
Project                : QtiSAS
--------------------------------------------------------------------
Copyright              : (C) 2017-2021 by Vitaliy Pipich
Email (use @ for *)    : v.pipich*gmail.com
Description            : compile interface: info tab functions

***************************************************************************/

/***************************************************************************
 * *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or  *
 *  (at your option) any later version.*
 * *
 *  This program is distributed in the hope that it will be useful,*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  *
 *  GNU General Public License for more details.   *
 * *
 *   You should have received a copy of the GNU General Public License *
 *   along with this program; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,*
 *   Boston, MA  02110-1301  USA   *
 * *
 ***************************************************************************/
#include "compile18.h"
#include <QClipboard>

void compile18::deleteFormat()
{
    textEditDescription->selectAll();
    textLeft();
    textEditDescription->setAcceptRichText(false);
    textEditDescription->setAcceptRichText(false);
    textEditDescription->selectAll();
    textEditDescription->cut();
    textEditDescription->paste();
    textEditDescription->setAcceptRichText(true);
}

void compile18::textFamily( const QString &f )
{
    
    textEditDescription->setFontFamily(f);
    textEditDescription->viewport()->setFocus();
}


void compile18::textSize( const QString &p )
{
    textEditDescription->setFontPointSize(p.toInt());
    textEditDescription->viewport()->setFocus();
}


void compile18::textBold()
{
    textEditDescription->setFontWeight(textEditDescription->fontWeight() < QFont::Bold);
}


void compile18::textUnderline()
{
    textEditDescription->setFontUnderline(!textEditDescription->fontUnderline());
}

void compile18::textItalic()
{
    textEditDescription->setFontItalic(!textEditDescription->fontItalic());
}

void compile18::textLeft()
{
    textEditDescription->setAlignment( Qt::AlignLeft );
}


void compile18::textRight()
{
    textEditDescription->setAlignment( Qt::AlignRight );
}


void compile18::textCenter()
{
    textEditDescription->setAlignment( Qt::AlignCenter );
}


void compile18::textJust()
{
    textEditDescription->setAlignment( Qt::AlignJustify);
}


void compile18::textEXP()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->clear();
    textEditDescription->cut();
    if (pushButtonEXP->isChecked() )
    {
        textEditDescription->insertHtml("<SUP>"+clipboard->text()+"</SUP>");
    }
    else
    {
     textEditDescription->insertHtml(clipboard->text());
    }
}


void compile18::textIndex()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->clear();
    textEditDescription->cut();
    
    if (pushButtonSub->isChecked() )
    {
        textEditDescription->insertHtml("<SUB>"+clipboard->text()+"</SUB>");
    }
    else
    {
        textEditDescription->insertHtml(clipboard->text());
    }
}

void compile18::textGreek()
{
    QString selected = textEditDescription->textCursor().selectedText();
    textEditDescription->cut();
    
    QString pattern="<*>";
    QRegExp rx(pattern);
    rx.setPatternSyntax(QRegExp::Wildcard);
    
    selected.remove(rx);
    
    int length=selected.length();
    
    for (int i=0; i<length;i++)
    {
        if (selected[i].unicode() >= 0x0061 && selected[i].unicode() <= 0x007A) selected[i]=QChar(selected[i].unicode()-0x0061+0x03B1);
        else if (selected[i].unicode() >= 0x03B1 && selected[i].unicode() <= 0x03C9) selected[i]=QChar(selected[i].unicode()-0x03B1+0x0061);
    }

    textEditDescription->insertPlainText(selected);
}


void compile18::readTextFormatting()
{
    
    if (textEditDescription->fontWeight() >= QFont::Bold) pushButtonBold->setChecked(true);
    else pushButtonBold->setChecked(false);
    
    if (textEditDescription->fontItalic()) pushButtonItal->setChecked(true);
    else pushButtonItal->setChecked(false);
    
    if (textEditDescription->fontUnderline()) pushButtonUnder->setChecked(true);
    else pushButtonUnder->setChecked(false);
    
    if (textEditDescription->fontUnderline()) pushButtonUnder->setChecked(true);
    else pushButtonUnder->setChecked(false);
    
    //+++
    pushButtonLeft->setChecked(false);
    pushButtonCenter->setChecked(false);
    pushButtonRight->setChecked(false);
    pushButtonJust->setChecked(false);
    
    //	QMessageBox::warning(this,tr("QtKws"), QString::number(textEditDescription->alignment()));
    
    
    switch (textEditDescription->alignment())
    {
        case Qt::AlignLeft: pushButtonLeft->setChecked(true); break;
        case Qt::AlignRight: pushButtonRight->setChecked(true); break;
        case Qt::AlignJustify: pushButtonJust->setChecked(true); break;
        case Qt::AlignHCenter: pushButtonCenter->setChecked(true); break;
    }
    
    //+++ font
    comboBoxFont->setItemText(comboBoxFont->currentIndex(), textEditDescription->fontFamily());
    comboBoxFontSize->setItemText(comboBoxFontSize->currentIndex(), QString::number(int(textEditDescription->fontPointSize() + 0.5)));
}


