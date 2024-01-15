/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2008 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: Pen style combobox
 ******************************************************************************/

#include "PenStyleBox.h"

#include <algorithm>

const Qt::PenStyle PenStyleBox::patterns[] = {
	Qt::SolidLine,
	Qt::DashLine,
	Qt::DotLine,
	Qt::DashDotLine,
	Qt::DashDotDotLine
};

PenStyleBox::PenStyleBox(QWidget *parent) : QComboBox(parent)
{
  	setEditable(false);
	addItem("_____");
	addItem("_ _ _");
	addItem(".....");
	addItem("_._._");
	addItem("_.._..");
}

void PenStyleBox::setStyle(const Qt::PenStyle& style)
{
  const Qt::PenStyle*ite = std::find(patterns, patterns + sizeof(patterns), style);
  if (ite == patterns + sizeof(patterns))
    this->setCurrentIndex(0); // default style is solid.
  else
    this->setCurrentIndex(ite - patterns);
}

Qt::PenStyle PenStyleBox::penStyle(int index)
{
  if (index < (int)sizeof(patterns))
    return patterns[index];
  else
    return Qt::SolidLine; // default style is solid. 
}

Qt::PenStyle PenStyleBox::style() const
{
  size_t i = this->currentIndex();
  if (i < sizeof(patterns))
    return patterns[i];
  else
    return Qt::SolidLine; // default style is solid. 
}

int PenStyleBox::styleIndex(const Qt::PenStyle& style)
{
  const Qt::PenStyle*ite = std::find(patterns, patterns + sizeof(patterns), style);
  if (ite == patterns + sizeof(patterns))
    return 0; // default style is solid.
  else
    return (ite - patterns);
}
