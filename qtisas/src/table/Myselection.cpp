/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Custom QTableWidgetSelectionRange class having isEmpty() method
 ******************************************************************************/

#include "Myselection.h"

bool MySelection::isEmpty() const
{
    return (topRow() < 0 && leftColumn() < 0 && bottomRow() < 0 && rightColumn() < 0);
}
