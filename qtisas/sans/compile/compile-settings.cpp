/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2017 Vitaliy Pipich <v.pipich@gmail.com>
Description: Settings functions of compile interface
 ******************************************************************************/

#include "compile18.h"


//*******************************************
//+++  Read settings
//*******************************************
void compile18::readSettings()
{
    
#ifdef Q_OS_MACOS // Mac
    QSettings settings(QSettings::IniFormat,QSettings::UserScope, "JCNS", "QtiSAS");
#else
    QSettings settings(QSettings::NativeFormat,QSettings::UserScope, "JCNS", "QtiSAS");
#endif
    
    /* ---------------- group Compile --------------- */
    settings.beginGroup("/Compile");

    
    if (settings.value("/Compile_compilerLocal",true).toBool())
    {
        checkBoxCompilerLocal->setChecked(true);
        compilerLocal(true);
    }
    else
    {
        checkBoxCompilerLocal->setChecked(false);
        compilerLocal(false);
    }

    if (settings.value("/Compile_gslLocal",true).toBool())
    {
        checkBoxGSLlocal->setChecked(true);
        gslLocal(true);
        
        if (settings.value("/Compile_gslStatic",true).toBool())
            checkBoxGSLstatic->setChecked(true);
        else
            checkBoxGSLstatic->setChecked(false);
        
    }
    else
    {
        checkBoxGSLlocal->setChecked(false);
        gslLocal(false);
        checkBoxGSLstatic->setChecked(false);
    }

    pathGSL=settings.value("/Compile_gslPath",pathGSL).toString().replace("\\","/").replace("//","/");
    gslPathline->setText(pathGSL);

    pathMinGW=settings.value("/Compile_mingwPath",pathGSL).toString();
    mingwPathline->setText(pathMinGW);
    
    lineEditCompileFlags->setText(settings.value("/Compile_compileFlags",pathGSL).toString());
 
    lineEditLinkFlags->setText(settings.value("/Compile_linkFlags",pathGSL).toString());
    
    settings.endGroup();
    
}

//*******************************************
//+++  Write settings
//*******************************************
void compile18::saveSettings()
{
#ifdef Q_OS_MACOS // Mac
    QSettings settings(QSettings::IniFormat,QSettings::UserScope, "JCNS", "QtiSAS");
#else
    QSettings settings(QSettings::NativeFormat,QSettings::UserScope, "JCNS", "QtiSAS");
#endif
    
    /* ---------------- group Compile --------------- */
    settings.beginGroup("/Compile");
    
    settings.setValue("/Compile_gslPath",gslPathline->text());
    settings.setValue("/Compile_mingwPath",mingwPathline->text());
    settings.setValue("/Compile_compileFlags",lineEditCompileFlags->text());
    settings.setValue("/Compile_linkFlags",lineEditLinkFlags->text());

    settings.setValue("/Compile_gslLocal",checkBoxGSLlocal->isChecked());
    settings.setValue("/Compile_gslStatic",checkBoxGSLstatic->isChecked());
    settings.setValue("/Compile_compilerLocal",checkBoxCompilerLocal->isChecked());
    

    
    settings.endGroup();
}


