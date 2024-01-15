/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2007 Ion Vasilief <ion_vasilief@yahoo.fr>
Description: An XML handler for the Fit class
 ******************************************************************************/

#ifndef FITMODELHANDLER_H
#define FITMODELHANDLER_H

#include <QXmlDefaultHandler>
#include <QVarLengthArray>

class Fit;

class FitModelHandler : public QXmlDefaultHandler
{
public:
    FitModelHandler(Fit *fit);

    bool startElement(const QString &namespaceURI, const QString &localName,
                       const QString &qName, const QXmlAttributes &attributes);
    bool endElement(const QString &namespaceURI, const QString &localName,
                     const QString &qName);
    bool characters(const QString &str);
    bool fatalError(const QXmlParseException &){return false;};
    QString errorString() const;

private:
    Fit* d_fit;
    bool metFitTag;
    QString currentText;
    QString errorStr;
    QString d_formula;
    QStringList d_parameters;
    QStringList d_explanations;
    QVarLengthArray<double> d_values;
};

#endif
