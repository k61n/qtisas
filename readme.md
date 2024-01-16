# QtiSAS

QtiSAS is an analysis software for SAS (small-angle scattering) measurements
based on QtiPlot.

# Installing

One can download qtisas package with or without python scripting support. Please
refer [releases page](https://iffgit.fz-juelich.de/qtisas/qtisas/-/releases).

The latest packages are:

 * For macos 13.6+:
   * arm:
     * [qtisas](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.9.16/qtisas-v0.9.16-arm.dmg)
     * [qtisas-py](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.9.16/qtisas-py-v0.9.16-arm.dmg)
   * intel:
     * [qtisas](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.9.16/qtisas-v0.9.16-intel.dmg)
     * [qtisas-py](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.9.16/qtisas-py-v0.9.16-intel.dmg)
 * For windows 11 x86-32:
   * [qtisas](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.9.16/qtisas-v0.9.16-x86_32.exe)
   * [qtisas-py](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.9.16/qtisas-py-v0.9.16-x86_32.exe)

# Troubleshooting

### “qtisas.app” cannot be opened because the developer cannot be verified.

Go to System Setting -> Privace & Security, scroll until message regarding
qtisas.app. Click "Open anyway".
Alternately run in terminal from the directory containing qtisas.app:

    xattr -r -d com.apple.quarantine "qtisas.app"

### Please indicate the correct path to the Python configuration files in the preferences dialog

    /usr/bin/python3 -m pip install PyQt5

# Building

Refer to [building instructions](build.md).
