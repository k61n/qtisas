/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: An XML handler for the Fit class
 ******************************************************************************/

#include "FitModelHandler.h"
#include "Fit.h"

#include <QMessageBox>

FitModelHandler::FitModelHandler(Fit *fit)
     : d_fit(fit)
 {
     metFitTag = false;
 }

 bool FitModelHandler::startElement(const QString & /* namespaceURI */,
                                const QString & /* localName */,
                                const QString &qName,
                                const QXmlAttributes &attributes)
 {
     if (!metFitTag && qName != "fit") {
         errorStr = QObject::tr("The file is not an QtiPlot fit model file.");
         return false;
     }

     if (qName == "fit") {
         QString version = attributes.value("version");
         if (!version.isEmpty() && version != "1.0") {
             errorStr = QObject::tr("The file is not an QtiPlot fit model version 1.0 file.");
             return false;
         }
         metFitTag = true;
     }

     currentText.clear();
     return true;
 }

bool FitModelHandler::endElement(const QString & /* namespaceURI */,
                              const QString & /* localName */,
                              const QString &qName)
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
    else if (qName == "fit"){
        d_fit->setParametersList(d_parameters);
        d_fit->setFormula(d_formula, false);
        d_fit->setInitialGuesses(d_values.data());
        d_fit->setParameterExplanations(d_explanations);
     }
     return true;
 }

 bool FitModelHandler::characters(const QString &str)
 {
     currentText += str;
     return true;
 }

 QString FitModelHandler::errorString() const
 {
     return errorStr;
 }
