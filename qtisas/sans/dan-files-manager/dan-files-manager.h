/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2023 Vitaliy Pipich <v.pipich@gmail.com>
Description: Header Parser used in DAN-SANS interface
 ******************************************************************************/
#ifndef DAN_FILES_MANAGER_H
#define DAN_FILES_MANAGER_H

#include <QCheckBox>
#include <QDir>
#include <QLineEdit>
#include <QString>
#include <QStringList>

class FilesManager : public QObject
{
    Q_OBJECT
  protected:
    QLineEdit *pathIn;
    QCheckBox *subFoldersActive;
    QLineEdit *pathOut;
    QLineEdit *wildCard;
    QLineEdit *wildCard2nd;
    QCheckBox *wildCard2ndActive;

  public:
    FilesManager(QLineEdit *pathIn, QCheckBox *subFoldersActive, QLineEdit *pathOut, QLineEdit *wildCard,
                 QLineEdit *wildCard2nd, QCheckBox *wildCard2ndActive);
    bool secondHeaderYN()
    {
        return wildCard2ndActive->isChecked();
    }
    QString wildCardHeader();
    QString wildCardDetector();

    QString fileNameFull(QString Number, QString wildCard);
    QString fileNameFullDetector(QString &Number)
    {
        return fileNameFull(Number, wildCardDetector());
    }
    QString fileNameFullHeader(QString &Number)
    {
        return fileNameFull(Number, wildCardHeader());
    }
  public slots:

};
#endif
