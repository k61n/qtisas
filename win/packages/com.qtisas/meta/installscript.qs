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
};
