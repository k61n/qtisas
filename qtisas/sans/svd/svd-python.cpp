/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Python interface for svd (contrast-variation SVD)
 ******************************************************************************/

#include "svd.h"

// why the last decompose() made no table (empty when it made one)
QString svd::lastError() const
{
    return m_lastError;
}

// the warnings of the last decompose(), e.g. Q points that differ between the data tables
QStringList svd::lastMessages() const
{
    return m_messages;
}
