function Component() {}

function showStatus(message) {
    installer.setValue("InstallerProgressText", message);
    console.log(message);
}

function download(url, tempInstaller) {
    // --- Prefer curl if available ---
    var curlFound = installer.execute("where", ["curl.exe"]);
    if (curlFound) {
        component.addOperation(
            "Execute",
            "curl.exe",
            ["-L", "-o", tempInstaller, url]
        );
    } else {
        var psCommand = "$dst='" + tempInstaller + "'; " +
                        "Invoke-WebRequest -Uri '" + url + "' -OutFile $dst";
        component.addOperation(
            "Execute",
            "powershell.exe",
            ["-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", psCommand]
        );
    };
}

Component.prototype.createOperations = function() {
    component.createOperations();

    var arch = (systemInfo.currentCpuArchitecture === "arm64") ? "arm64" : "amd64";
    var archInPath = (arch === "arm64") ? "-arm64" : "";
    var tempDir = installer.environmentVariable("TEMP");
    var tempInstaller = tempDir + "\\python-__PY_VERSION__-" + arch + ".exe";
    var url = "https://www.python.org/ftp/python/__PY_VERSION__/python-__PY_VERSION__-" + arch + ".exe";

    var userData = installer.environmentVariable("LOCALAPPDATA");
    var pythonDir = userData + "\\Programs\\Python\\Python313" + archInPath;
    var pyInstalled = pythonDir + "\\python.exe";

    // Always install/upgrade Python __PY_VERSION__
    download(url, tempInstaller);
    var psCommand = 'Start-Process -FilePath "' + tempInstaller + '" ' +
                    '-ArgumentList "/quiet","InstallAllUsers=0","PrependPath=1","Include_launcher=0","SimpleInstall=1" ' +
                        '-Wait';
    component.addOperation(
        "Execute",
        "powershell.exe",
        ["-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", psCommand]
    );

    // Install PyQt6 __PYQT_VERSION__
    var psCmd = '$python="' + pyInstalled + '"; ' +
            'Start-Process -FilePath $python -ArgumentList "-m pip install --quiet --upgrade PyQt6==__PYQT_VERSION__" -Wait';
   component.addOperation(
       "Execute",
       "powershell.exe",
       ["-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", psCmd]
   );
}
