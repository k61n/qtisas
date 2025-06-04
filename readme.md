# QtiSAS

QtiSAS is an analysis software for SAS (small-angle scattering) measurements
based on QtiPlot.

# Installing

One can download qtisas package from [releases page](https://iffgit.fz-juelich.de/qtisas/qtisas/-/releases).

The latest packages are:

 * For macOS 13.6+:
   * [qtisas-arm64](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.12.3/qtisas-v0.12.3-arm64.dmg)
   * [qtisas-intel](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.12.3/qtisas-v0.12.3-intel.dmg)
 * For Windows:
   * [qtisas-arm64](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.12.3/qtisas-v0.12.3-arm64.exe)
   * [qtisas-x64](https://iffgit.fz-juelich.de/api/v4/projects/1655/packages/generic/qtisas/v0.12.3/qtisas-v0.12.3-x64.exe)
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

    xattr -r -d com.apple.quarantine "qtisas.app"

# Building

Refer to [building instructions](build.md).
