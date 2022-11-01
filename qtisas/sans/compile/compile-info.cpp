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
    
    textEditDescription->setFamily( f );
    textEditDescription->viewport()->setFocus();
}


void compile18::textSize( const QString &p )
{
    textEditDescription->setPointSize( p.toInt() );
    textEditDescription->viewport()->setFocus();
}


void compile18::textBold()
{
    textEditDescription->setBold( !textEditDescription->bold());
}


void compile18::textUnderline()
{
    textEditDescription->setUnderline( !textEditDescription->underline());
}

void compile18::textItalic()
{
    textEditDescription->setItalic(!textEditDescription->italic());
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
    QString selected=textEditDescription->selectedText();
    textEditDescription->cut();
    
    QString pattern="<*>";
    QRegExp rx(pattern);
    rx.setWildcard( TRUE );
    
    selected.remove(rx);
    
    int length=selected.length();
    
    for (int i=0; i<length;i++)
    {
        if (selected[i].unicode() >= 0x0061 && selected[i].unicode() <= 0x007A) selected[i]=QChar(selected[i].unicode()-0x0061+0x03B1);
        else if (selected[i].unicode() >= 0x03B1 && selected[i].unicode() <= 0x03C9) selected[i]=QChar(selected[i].unicode()-0x03B1+0x0061);
    }

    textEditDescription->insert(selected);
}


void compile18::readTextFormatting()
{
    
    if (textEditDescription->bold()) pushButtonBold->setOn(true);
    else pushButtonBold->setOn(false);
    
    if (textEditDescription->italic()) pushButtonItal->setOn(true);
    else pushButtonItal->setOn(false);
    
    if (textEditDescription->underline()) pushButtonUnder->setOn(true);
    else pushButtonUnder->setOn(false);
    
    if (textEditDescription->underline()) pushButtonUnder->setOn(true);
    else pushButtonUnder->setOn(false);
    
    //+++
    pushButtonLeft->setOn(false);
    pushButtonCenter->setOn(false);
    pushButtonRight->setOn(false);
    pushButtonJust->setOn(false);
    
    //	QMessageBox::warning(this,tr("QtKws"), QString::number(textEditDescription->alignment()));
    
    
    switch (textEditDescription->alignment())
    {
        case Qt::AlignLeft: pushButtonLeft->setOn(true); break;
        case Qt::AlignRight: pushButtonRight->setOn(true);break;
        case -8: pushButtonJust->setOn(true);break;
        case 4:     pushButtonCenter->setOn(true);break;
    }
    
    //+++ font
    comboBoxFont->setCurrentText(textEditDescription->family());
    comboBoxFontSize->setCurrentText(QString::number(textEditDescription->pointSize ()));
}


