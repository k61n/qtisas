/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2025 Vitaliy Pipich <v.pipich@gmail.com>
Description: open/compile sasview function with python script: sasmodels
 ******************************************************************************/

#include "compile18.h"

//*******************************************
//+++  copyFiles: Preserve Structure
//*******************************************
void copyFilesPreserveStructure(const QStringList &lst, const QString &sourcePath, const QString &destinPath)
{
    QDir sourceDir(sourcePath);
    QDir destDir(destinPath);

    for (const QString &relativePath : lst)
    {
        QString srcFile = sourceDir.filePath(relativePath);
        QString destFile = destDir.filePath(relativePath);

        QFileInfo destInfo(destFile);
        QDir().mkpath(destInfo.path());

        if (!QFile::exists(destFile))
            QFile::copy(srcFile, destFile);
    }
}
//*******************************************
//+++  creation of the header file *.h
//*******************************************
void createHeaderFile(QString headers, const QString &outFilePath)
{
    QFile file(outFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QString fileName = QFileInfo(outFilePath).fileName();
    QTextStream out(&file);

    out << "// " << fileName << "\n\n";
    fileName = fileName.replace('.', '_').toUpper();

    out << "#ifndef " << fileName << "\n";
    out << "#define " << fileName << "\n\n";
    out << "#define FLOAT_SIZE 8\n\n";

    headers = "kernel_header.c," + headers;

    for (const QString &header : headers.split(','))
    {
        QString trimmedHeader = header.trimmed();
        if (!trimmedHeader.isEmpty())
            out << "#include \"models/" << trimmedHeader << "\"\n";
    }

    out << "\n#endif" << "\n";

    file.close();
}
//*******************************************
//+++  openSasViewPy
//*******************************************
void compile18::openSasViewPy()
{
    if (app()->activeScriptingLanguage() != QString("Python"))
    {
        QMessageBox::critical(nullptr, "QtiSAS",
                              "SasView models could be opened only with Python as an ACTIVE script language");
        return;
    }

    QString script = QString("import os, sasmodels\n"
                             "return [os.path.join(os.path.dirname(sasmodels.__file__), \"models\")]");
    

    QString Dir = app()->scriptCaller(script).toList()[0].toString() + "/";
    if (!QDir(Dir).exists())
    {
        QMessageBox::critical(nullptr, "QtiSAS", "Please install first sasmodels: python3 -m pip install sasmodels");
        return;
    }

    auto *fd = new QFileDialog(this, "Choose a file", Dir, "*.py");

    fd->setDirectory(Dir);
    fd->setFileMode(QFileDialog::ExistingFile);
    fd->setWindowTitle(tr("Compie - Select SasView Python File"));
    if (fd->exec() == QDialog::Rejected)
        return;

    QStringList selected = fd->selectedFiles();

    const QString &pyFile = selected[0];

    const QString model = QFileInfo(selected[0]).completeBaseName();

    newFIF();

    // Preparing common python script imports
    QString scriptImport = "from sasmodels.core import load_model_info\n";
    QString modelScript =
        scriptImport + "return [%1 for param in load_model_info(\"%2\").parameters.kernel_parameters]";

    // Retrieve model data
    auto getModelData = [&](const QString &paramName) {
        return app()->scriptCaller(modelScript.arg(paramName).arg(model)).toStringList();
    };

    QStringList paraNames = getModelData("param.name");
    QStringList paraDefaults = getModelData("param.default");
    QStringList paraUnits = getModelData("param.units");
    QStringList paraDescription = getModelData("param.description");
    QStringList paraLimits = getModelData("str(param.limits)");
    QStringList paraLength = getModelData("param.length");
    QStringList paraType = getModelData("param.type");

    // Getting source files
    QStringList source =
        app()->scriptCaller(scriptImport + QString("return load_model_info(\"%1\").source").arg(model)).toStringList();

    script = scriptImport + "return [str(load_model_info(\"%1\").%2)]";

    //+++[group Name]
    QString category = app()->scriptCaller(script.arg(model, "category")).toStringList()[0];
    lineEditGroupName->setText(category);

    //+++[name]
    QString name = model;
    lineEditFunctionName->setText("sasviewmodels/" + name.replace('_', '-'));
    textLabelInfoSAS->setText(QFileInfo(pyFile).fileName());

    //+++[number parameters]
    int pNumber = static_cast<int>(paraNames.count());
    if (pNumber <= 0)
    {
        QMessageBox::warning(this, "QtiSAS", "Error: [number parameters]");
        return;
    }
    spinBoxP->setValue(pNumber + 2);

    //+++[description]
    textEditDescription->clear();

    textEditDescription->insertPlainText("SASVIEW Web Link: ");
    QString link = "https://www.sasview.org/docs/user/models/" + model + ".html";
    textEditDescription->insertHtml("<a href=\"" + link + "\">" + link + "</a>\n");

    QString description = "\n\nTitle: " + app()->scriptCaller(script.arg(model).arg("title")).toStringList()[0] + "\n";

    description += "Name: " + name + "\n";
    description += "Category: " + category + "\n\n";

    description += app()->scriptCaller(script.arg(model).arg("description")).toStringList()[0];

    QString docs = app()->scriptCaller(script.arg(model).arg("docs")).toStringList()[0];
    if (docs.split("Reference").count() > 0)
        docs = docs.split("References\n")[1];

    description += "\n\nReferences\n" + docs.remove("**").replace(".. [#", "[") + "\n";
    textEditDescription->insertPlainText(description);

    //+++[x]
    lineEditXXX->setText("q");

    //+++[y]
    lineEditY->setText("I");

    //+++[parameter names]
    //+++ "scale" parameter
    tableParaNames->item(0, 0)->setText("scale");
    tableParaNames->item(0, 1)->setText("1.00");
    tableParaNames->item(0, 2)->setCheckState(Qt::Unchecked);
    tableParaNames->item(0, 2)->setText("-inf..inf");
    tableParaNames->item(0, 3)->setText("[1]: Scaling factor - automatical");
    //+++ "background" parameter
    tableParaNames->item(1, 0)->setText("background");
    tableParaNames->item(1, 1)->setText("0.001");
    tableParaNames->item(1, 2)->setCheckState(Qt::Unchecked);
    tableParaNames->item(1, 2)->setText("0..inf");
    tableParaNames->item(1, 3)->setText("[cm^-1]: background - automatical");
    //+++ rest parameters
    for (int i = 0; i < pNumber; i++)
    {
        tableParaNames->item(i + 2, 0)->setText(paraNames[i]);

        tableParaNames->item(i + 2, 1)->setText(paraDefaults[i]);

        tableParaNames->item(i + 2, 2)->setCheckState(paraType[i] != "orientation" ? Qt::Checked : Qt::Unchecked);

        QStringList limits = paraLimits[i].remove("(").remove(")").split(", ");
        tableParaNames->item(i + 2, 2)->setText(limits.count() == 2 ? limits[0] + ".." + limits[1] : "..");

        QString info = QString("[%1]: %2 - %3").arg(paraUnits[i], paraDescription[i].replace(',', ';'), paraType[i]);
        if (paraType[i] == "orientation")
            info += " ! not used for 1D function !";
        tableParaNames->item(i + 2, 3)->setText(info);
    }

    //+++ [h-headers]
    copyFilesPreserveStructure(QStringList() << "kernel_header.c", QDir::cleanPath(Dir + "/.."),
                               QDir::cleanPath(app()->sasPath + "FitFunctions/sasviewmodels/models"));
    copyFilesPreserveStructure(source, QDir::cleanPath(Dir),
                               QDir::cleanPath(app()->sasPath + "FitFunctions/sasviewmodels/models"));
    createHeaderFile(source.join(", "), QDir::cleanPath(app()->sasPath + "FitFunctions/sasviewmodels/" + name + ".h"));

    textEditHFiles->clear();
    textEditHFiles->append("#include \"" + name + ".h" + "\"");

    //+++[code]
    QString code;
    textEditCode->clear();

    //+++[code]: Iq or Fq
    bool hasFq = app()->scriptCaller(script.arg(model).arg("have_Fq")).toStringList().value(0) == "True";

    QStringList args;
    for (int i = 0; i < pNumber; ++i)
        if (paraType[i] != "orientation")
            args << paraNames[i];

    if (hasFq)
    {
        code = "\ndouble F1, F2;\n";
        code += QString("\nFq(q, &F1, &F2%1);\n").arg(args.isEmpty() ? "" : ", " + args.join(", "));
        code += "I = F2;\n";
    }
    else
        code = QString("\nI = Iq(q%1);\n").arg(args.isEmpty() ? "" : ", " + args.join(", "));

    //+++[code]: volume normalization
    QStringList volumeParameters;
    for (int i = 0; i < pNumber; i++)
        if (paraType[i] == "volume")
            volumeParameters << paraNames[i];

    if (!volumeParameters.isEmpty())
        code += QString("\nI /= form_volume(%1);\n").arg(volumeParameters.join(", "));

    //+++[code]: scaling and bacground
    code += "\nI *= scale;\n";
    code += "I += background;\n";

    textEditCode->append(code);
    textEditCode->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
}