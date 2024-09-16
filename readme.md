# QtiSAS

QtiSAS is an analysis software for SAS (small-angle scattering) measurements
based on QtiPlot.

# Installing

One can download qtisas package with or without python scripting support. Please
refer [releases page](https://iffgit.fz-juelich.de/qtisas/qtisas/-/releases).

The latest packages are:

 * For macos 13.6+:
   * arm:
     * [qtisas](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.10.4/qtisas-v0.10.4-arm.dmg)
     * [qtisas-py](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.10.4/qtisas-py-v0.10.4-arm.dmg)
   * intel:
     * [qtisas](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.10.4/qtisas-v0.10.4-intel.dmg)
     * [qtisas-py](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.10.4/qtisas-py-v0.10.4-intel.dmg)
 * For windows x64:
   * [qtisas](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.10.4/qtisas-v0.10.4-x64.exe)
   * [qtisas-py](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.10.4/qtisas-py-v0.10.4-x64.exe)
 * GNU/Linux arm64 and x64 packages are available for the following distros (see [instructions](https://software.opensuse.org//download.html?project=home%3Akholostov&package=qtisas)):
   * Debian 11
   * Debian 12
   * Fedora 39
   * Fedora 40
   * RockyLinux 8
   * RockyLinux 9
   * Ubuntu 20.04
   * Ubuntu 22.04
   * Ubuntu 24.04

# Troubleshooting

### “qtisas.app” cannot be opened because the developer cannot be verified.

Go to System Setting -> Privace & Security, scroll until message regarding
qtisas.app. Click "Open anyway".
Alternately run in terminal from the directory containing qtisas.app:

    xattr -r -d com.apple.quarantine "qtisas.app"

### Please indicate the correct path to the Python configuration files in the preferences dialog

    /usr/bin/python3 -m pip install pyqt6

# Building

Refer to [building instructions](build.md).
