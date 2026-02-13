# QtiSAS

QtiSAS is an analysis software for SAS (small-angle scattering) measurements
based on QtiPlot.

# Installing

One can download qtisas package from [releases page](https://iffgit.fz-juelich.de/qtisas/qtisas/-/releases).

The latest packages are:

 * For macOS 13.6+ universal bundle:
   * [qtisas](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.15.0/qtisas-v0.15.0.dmg)
 * For Windows:
   * [qtisas-arm64](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.15.0/qtisas-v0.15.0-arm64.exe)
   * [qtisas-x64](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.15.0/qtisas-v0.15.0-x64.exe)
 * GNU/Linux arm64 and x64 packages are available for the following non-EOL distros (see [instructions](https://software.opensuse.org//download.html?project=home%3Akholostov&package=qtisas)):
   * Debian
   * Fedora
   * RockyLinux
   * Ubuntu LTS

# Troubleshooting

### “qtisas.app” cannot be opened because the developer cannot be verified.

Go to System Setting -> Privace & Security, scroll until message regarding
qtisas.app. Click "Open anyway".
Alternately run in terminal from the directory containing qtisas.app:

    xattr -cr qtisas.app

# Adding python packages in macOS

Python is shipped with macOS bundle. By default, it is a clean python installation
having PyQt6 package. To add other python packages pass `pip install <package>`
as argument to `qtisas.app`:

    open qtisas.app --args pip install numpy

The packages are installed to `~/.config/qtisas/python/site-packages` directory.

# Building

Refer to [building instructions](build.md).
