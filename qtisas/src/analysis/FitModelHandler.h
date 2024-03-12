/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2006 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: An XML handler for the Fit class
 ******************************************************************************/

#ifndef FITMODELHANDLER_H
#define FITMODELHANDLER_H

#include <QXmlDefaultHandler>
#include <QVarLengthArray>

#include "CustomActionDialog.h"
#include "Fit.h"


class FitModelHandler : public CustomXMLParser
{
public:
    FitModelHandler(Fit *fit);
    bool endElement(const QString &namespaceURI, const QString &localName,
                     const QString &qName);

private:
    Fit* d_fit;
    QString d_formula;
    QStringList d_parameters, d_explanations;
    QVarLengthArray<double> d_values;
};

#endif
