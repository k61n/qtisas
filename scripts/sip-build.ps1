param(
    [string]$sipbuild,
    [string]$path,
    [string]$qmake,
    [string]$builddir
)

$process = Start-Process -FilePath $sipbuild -WorkingDirectory $path `
    -ArgumentList `
    "--no-compile", "--qmake", "$qmake", "--build-dir", $builddir `
    -PassThru
$process.WaitForExit()

Exit 0
