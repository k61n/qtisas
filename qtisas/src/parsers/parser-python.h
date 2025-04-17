/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: python code parser: information extraction
 ******************************************************************************/

#ifndef PARSERPYTHON_H
#define PARSERPYTHON_H

#include <QString>
#include <QStringList>

class ParserPython
{
  public:
    static QStringList extractBlock(const QString &filePath, const QString &start, const QString &finish,
                                    bool skipStartLine = true, bool finishFromStart = true);
};

#endif
