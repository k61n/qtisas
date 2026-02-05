function Component()
{
    // default constructor
}

Component.prototype.createOperations = function()
{
    // Call the default implementation to install all files in the package
    component.createOperations();

    // Create a Start Menu shortcut for the application
    component.addOperation("CreateShortcut", "@TargetDir@/qtisas.exe", "@StartMenuDir@/QtiSAS.lnk",
                           "workingDirectory=@TargetDir@", "iconPath=@TargetDir@/icons/qtisas.ico",
                           "description=Launch QtiSAS Application");

    // Create a Start Menu shortcut for the Maintenance Tool
    component.addOperation("CreateShortcut", "@TargetDir@/maintenancetool.exe", "@StartMenuDir@/Uninstall\ QtiSAS.lnk",
                           "workingDirectory=@TargetDir@", "iconPath=@TargetDir@/icons/qtisas.ico",
                           "description=Open QtiSAS Maintenance Tool");


    var exePath = installer.value("TargetDir").replace(/\//g, "\\") + "\\qtisas.exe";
    var icoPath = installer.value("TargetDir").replace(/\//g, "\\") + "\\icons\\qtisas.ico";

    // Register application for "Open with"
    component.addOperation("Execute", "reg", "add", "HKCU\\Software\\Classes\\Applications\\qtisas.exe", "/ve", "/d", "QtiSAS", "/f");
    component.addOperation("Execute", "reg", "add", "HKCU\\Software\\Classes\\Applications\\qtisas.exe\\DefaultIcon", "/ve", "/d", icoPath, "/f");
    component.addOperation("Execute", "reg", "add", "HKCU\\Software\\Classes\\Applications\\qtisas.exe\\shell\\open", "/ve", "/d", "Open", "/f");
    component.addOperation("Execute", "reg", "add", "HKCU\\Software\\Classes\\Applications\\qtisas.exe\\shell\\open\\command", "/ve", "/d", "\"" + exePath + "\" \"%1\"", "/f");    

    // Register QTI file type
    component.addOperation("Execute", "reg", "add", "HKCU\\Software\\Classes\\.qti", "/ve", "/d", "QtiSAS.Project", "/f");
    component.addOperation("Execute", "reg", "add", "HKCU\\Software\\Classes\\QtiSAS.Project", "/ve", "/d", "QtiSAS Project File", "/f");
    component.addOperation("Execute", "reg", "add", "HKCU\\Software\\Classes\\QtiSAS.Project\\DefaultIcon", "/ve", "/d", icoPath, "/f");
    component.addOperation("Execute", "reg", "add", "HKCU\\Software\\Classes\\QtiSAS.Project\\shell\\open\\command", "/ve", "/d", exePath + " \"%1\"", "/f");

    // Refresh Explorer associations
    component.addOperation("Execute", "rundll32.exe", "shell32.dll,SHChangeNotify", "0x08000000", "0x0000");
};
