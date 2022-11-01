/***************************************************************************
 File                   : compile-settings.cpp
 Project                : QtiSAS
 --------------------------------------------------------------------
 Copyright              : (C) 2017-2021 by Vitaliy Pipich
 Email (use @ for *)    : v.pipich*gmail.com
 Description            : compile interface: settings functions
 
 ***************************************************************************/

/***************************************************************************
 * *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or  *
 *  (at your option) any later version.*
 * *
 *  This program is distributed in the hope that it will be useful,*
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  *
 *  GNU General Public License for more details.   *
 * *
 *   You should have received a copy of the GNU General Public License *
 *   along with this program; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,*
 *   Boston, MA  02110-1301  USA   *
 * *
 ***************************************************************************/
#include "compile18.h"


//*******************************************
//+++  Read settings
//*******************************************
void compile18::readSettings()
{
    
#ifdef Q_OS_MAC // Mac
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
#ifdef Q_OS_MAC // Mac
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


