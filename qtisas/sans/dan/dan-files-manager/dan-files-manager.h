/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
    2024 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: Header Parser used in DAN-SANS interface
 ******************************************************************************/
#ifndef DAN_FILES_MANAGER_H
#define DAN_FILES_MANAGER_H

#include "compat.h"

#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QLineEdit>
#include <QString>
#include <QStringList>
#include <QToolButton>

class FilesManager : public QObject
{
    Q_OBJECT
  protected:
    QLineEdit *pathIn;
    QLineEdit *wildCardIn;
    QCheckBox *subFoldersActive;
    QToolButton *buttonPathIn;
    QLineEdit *pathOut;
    QToolButton *buttonPathOut;
    QLineEdit *wildCard;
    QLineEdit *wildCard2nd;
    QCheckBox *wildCard2ndActive;

  public:
    FilesManager(QLineEdit *pathIn, QLineEdit *wildCardIn, QCheckBox *subFoldersActive, QToolButton *buttonPathIn,
                 QLineEdit *pathOut, QToolButton *buttonPathOut, QLineEdit *wildCard, QLineEdit *wildCard2nd,
                 QCheckBox *wildCard2ndActive);
    bool secondHeaderYN()
    {
        return wildCard2ndActive->isChecked();
    }
    QString wildCardHeader();
    QString wildCardDetector();
    QString pathInString()
    {
        return pathIn->text();
    }
    QString pathOutString()
    {
        return pathOut->text();
    }
    bool subFoldersYN()
    {
        return subFoldersActive->isChecked();
    }
    static QString fileName(QString Number, QString wildCardLocal, QString &subDir);
    QString fileNameFull(const QString &Number, const QString &wildCardLocal);
    QString newFileNameFull(const QString &Number, const QString &wildCardLocal);

    QString fileNameFullDetector(const QString &Number)
    {
        return fileNameFull(Number, wildCardDetector());
    }
    QString fileNameFullHeader(QString &Number)
    {
        return fileNameFull(Number, wildCardHeader());
    }
    //+++ checkFileNumber
    bool checkFileNumber(QString Number);
    //+++ find-File-Number-In-File-Name
    static QString findFileNumberInFileName(QString wildCardLocal, QString file);

  public slots:
    //+++ SLOT: pathIn
    bool pushedPathIn();
    //+++  SLOT: pathOut
    bool pushedPathOut();
};
#endif
