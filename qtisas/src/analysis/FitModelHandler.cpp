/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
    2022 Konstantin Kholostov <k.kholostov@fz-juelich.de>
    2022 Vitaliy Pipich <v.pipich@gmail.com>
Description: An XML handler for the Fit class
 ******************************************************************************/

#include "FitModelHandler.h"

FitModelHandler::FitModelHandler(Fit *fit) : d_fit(fit)
{
    metFitTag = false;
    handlerType = "fit";
}

bool FitModelHandler::endElement(const QString &namespaceURI, const QString &localName, const QString &qName)
{
    if (qName == "model")
        d_fit->setObjectName(currentText);
    else if (qName == "type")
        d_fit->setType((Fit::FitType)currentText.toInt());
    else if (qName == "function")
        d_formula = currentText.replace("&lt;", "<").replace("&gt;", ">");
    else if (qName == "name" && !currentText.isEmpty())
        d_parameters << currentText;
    else if (qName == "explanation")
        d_explanations << currentText;
    else if (qName == "value")
        d_values.append(currentText.toDouble());
    else if (qName == "fit")
    {
        d_fit->setParametersList(d_parameters);
        d_fit->setFormula(d_formula, false);
        d_fit->setInitialGuesses(d_values.data());
        d_fit->setParameterExplanations(d_explanations);
    }
     return true;
 }
