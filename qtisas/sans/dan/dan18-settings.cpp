/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2022 Vitaliy Pipich <v.pipich@gmail.com>
    2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
Description: SANS data analysis interface
 ******************************************************************************/

#include "dan18.h"

//*******************************************
//+++  Read settings
//*******************************************
void dan18::readSettings()
{
    QSettings *settings = Settings::DefaultSettings();

    settings->beginGroup("/DAN");

    bool ok = settings->contains("/lineEditPathDAT");
    QString ss = settings->value("/lineEditPathDAT", 0).toString();
    if (ok && ss.left(4) != "home")
        lineEditPathDAT->setText(ss);
    ok = settings->contains("/lineEditPathRAD");
    ss = settings->value("/lineEditPathRAD", 0).toString();
    if (ok && ss.left(4) != "home")
        lineEditPathRAD->setText(ss);
    if (settings->contains("/instrument"))
    {
        ss = settings->value("/instrument", 0).toString();
        if (comboBoxInstrument->findText(ss) >= 0)
        {
            comboBoxInstrument->setCurrentIndex(comboBoxInstrument->findText(ss));
            instrumentSelected();
        }
    }
    if (settings->contains("/rawdataFastExtractorMode"))
        comboBoxCheck->setCurrentIndex(settings->value("/rawdataFastExtractorMode", 0).toInt());

    settings->endGroup();

    delete settings;
}
//*******************************************
//+++  Write settings
//*******************************************
void dan18::writeSettings()
{
    QSettings *settings = Settings::DefaultSettings();

    settings->beginGroup("/DAN");

    if (lineEditPathDAT->text() != "home")
        settings->setValue("/lineEditPathDAT", lineEditPathDAT->text());
    if (lineEditPathRAD->text() != "home")
        settings->setValue("/lineEditPathRAD", lineEditPathRAD->text());
    settings->setValue("/instrument", comboBoxInstrument->currentText());
    settings->setValue("/rawdataFastExtractorMode", comboBoxCheck->currentIndex());
    settings->endGroup();

    delete settings;
}