function Component() {}

function showStatus(message) {
    installer.setValue("InstallerProgressText", message);
    console.log(message);
}

function vsInstalled() {
    // Path to vswhere
    var vswhere = installer.environmentVariable("ProgramFiles(x86)") + "\\Microsoft Visual Studio\\Installer\\vswhere.exe";

    // Require specific components
    var exitCode = installer.execute(vswhere, [
        "-products", "*",
        "-version", "[17.0,18.0)",
        "-requires", "Microsoft.VisualStudio.Component.VC.Tools.ARM64",
        "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
        "-requires", "Microsoft.VisualStudio.Component.Windows11SDK",
        "-format", "value",
        "-property", "installationPath"
    ]);

    return exitCode === 0;
}

Component.prototype.createOperations = function() {
    component.createOperations();

    if (!vsInstalled()) {
        showStatus("MSVC 2022 not found. Installing...");
        var tempDir = installer.environmentVariable("TEMP");
        var vsInstaller = tempDir + "\\vs_community.exe";

        // Download Visual Studio 2022 Community installer
        var vsUrl = "https://aka.ms/vs/17/release/vs_Community.exe";
        component.addOperation(
            "Execute",
            "powershell.exe",
            ["-NoProfile", "-ExecutionPolicy", "Bypass", "-Command",
             "$wc = New-Object System.Net.WebClient; $wc.DownloadFile('" + vsUrl + "', '" + vsInstaller + "')"]
        );

        // Install only the MSVC compiler for detected architecture
        component.addOperation(
            "Execute",
            vsInstaller,
            [
                "--quiet",
                "--wait",
                "--norestart",
                "--add", "Microsoft.VisualStudio.Component.VC.Tools.ARM64",
                "--add", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                "--add", "Microsoft.VisualStudio.Component.Windows11SDK",
                "--includeRecommended",
                "--installPath", installer.environmentVariable("ProgramFiles") + "\\Microsoft Visual Studio\\2022\\Community"
            ]
        );
    } else {
        showStatus("MSVC 2022 is already installed.");
    }
};
