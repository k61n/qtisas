param(
    [string]$os,
    [string]$arch,
    [string]$cores,
    [string]$name,
    [string]$libdir,
    [string]$cc,
    [string]$cxx,
    [string]$make,
    [string]$gen,
    [string]$qt,
    [string]$zlib,
    [string]$gsl,
    [string]$gl2ps
)

$qtisasdir = Split-Path -Path (Split-Path -Path $libdir -Parent) -Parent
if ($gen -eq "Visual Studio 17 2022") {
    $file = "$qtisasdir\libs\$os-$arch\$name\lib\$name.lib"
} else {
    $file = "$qtisasdir\libs\$os-$arch\$name\lib\lib$name.dll.a"
}

if (Test-Path -Path $file) {
    Write-Host "Library ${name} is already built: $file"
    Exit 0
}

Write-Host "Building $name library"

Remove-Item -Path "$libdir\tmp" -Recurse -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path "$libdir\tmp" | Out-Null

$install_path = Split-Path -Path (Split-Path -Path $file -Parent) -Parent

switch ($name) {
    "minigzip" {
        $args = "-DZLIB_ROOT=" + '"' + $zlib + '"'
    }
    {$_ -in @("qtexengine", "qwt")} {
        $args = '-DCMAKE_PREFIX_PATH="' + $qt + '"'
    }
    "qwtplot3d" {
        $args = '-Dgl2ps_ROOT="' + $gl2ps + '" ' + '-DCMAKE_PREFIX_PATH="' + $qt + '"'
    }
    "tamuanova" {
        $args = "-DGSL_ROOT=" + '"' + $gsl + '"'
    }
    default {
        $args = '-DUNUSED=""'
    }
}

if ($gen -eq 'Visual Studio 17 2022') {
    if (("minigzip", "qtexengine", "qwt", "qwtplot3d") -contains $name) {
        $shared = "ON"
    }
    else {
        $shared = "OFF"
    }
    $compilers = "-G`"$gen`""
    $build = "--config Release"
}
else {
    $shared = "ON"
    $compilers = "-G`"$gen`" -DCMAKE_MAKE_PROGRAM=$make -DCMAKE_C_COMPILER=$cc -DCMAKE_CXX_COMPILER=$cxx"
    $build = "--parallel $cores"
}

$process = Start-Process -FilePath "cmake.exe" -ArgumentList `
    "-S", "$libdir", "-B", "$libdir/tmp", `
    $compilers, `
    "-DCMAKE_BUILD_TYPE=Release", `
    "-DBUILD_SHARED_LIBS=$shared", `
    "-DCMAKE_INSTALL_PREFIX=$install_path", `
    "-DCMAKE_INSTALL_LIBDIR=lib", `
    $args `
    -PassThru `
    -RedirectStandardOutput "$libdir\tmp\configure.log" `
    -RedirectStandardError "$libdir\tmp\configure_error.log"
$process.WaitForExit()

$process = Start-Process -FilePath "cmake.exe" -ArgumentList `
    "--build", "$libdir\tmp", $build -PassThru `
    -RedirectStandardOutput "$libdir\tmp\build.log" `
    -RedirectStandardError "$libdir\tmp\build_error.log"
$process.WaitForExit()

$process = Start-Process -FilePath "cmake.exe" -ArgumentList `
    "--install", "$libdir\tmp" -PassThru `
    -RedirectStandardOutput "$libdir\tmp\install.log" `
    -RedirectStandardError "$libdir\tmp\install_error.log"
$process.WaitForExit()

if (!(Test-Path -Path $file)) {
    Write-Host "Error building $name"
    Exit 1
}

Remove-Item -Path "$libdir\tmp" -Recurse -ErrorAction SilentlyContinue

Exit 0
