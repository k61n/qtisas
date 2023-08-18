param(
    [string]$os,
    [string]$arch,
    [string]$cores,
    [string]$name,
    [string]$libdir,
    [string]$cc,
    [string]$cxx,
    [string]$make,
    [string]$qt,
    [string]$zlib,
    [string]$png,
    [string]$gsl
)

$qtisasdir = Split-Path -Path (Split-Path -Path $libdir -Parent) -Parent
$file = "$qtisasdir\libs\$os-$arch\$name\lib\lib$name.a"

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
        $zlib_root = "-DZLIB_ROOT=" + '"' + $zlib + '"'
        $process = Start-Process -FilePath "cmake.exe" -ArgumentList `
            "-S", "$libdir", "-B", "$libdir/tmp", "-G", '"MinGW Makefiles"', `
            "-DCMAKE_C_COMPILER=$cc", "-DCMAKE_CXX_COMPILER=$cxx", `
            "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_SHARED_LIBS=OFF", `
            $zlib_root, `
            "-DCMAKE_INSTALL_PREFIX=$install_path", "-DCMAKE_INSTALL_LIBDIR=lib" `
            -PassThru `
            -RedirectStandardOutput "$libdir\tmp\configure.log" `
            -RedirectStandardError "$libdir\tmp\configure_error.log"
    }
    "muparser" {
        $process = Start-Process -FilePath "cmake.exe" -ArgumentList `
            "-S", "$libdir", "-B", "$libdir/tmp", "-G", '"MinGW Makefiles"', `
            "-DCMAKE_C_COMPILER=$cc", "-DCMAKE_CXX_COMPILER=$cxx", `
            "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_SHARED_LIBS=OFF", `
            "-DENABLE_SAMPLES=OFF", "-DENABLE_OPENMP=OFF", `
            "-DCMAKE_INSTALL_PREFIX=$install_path", "-DCMAKE_INSTALL_LIBDIR=lib" `
            -PassThru `
            -RedirectStandardOutput "$libdir\tmp\configure.log" `
            -RedirectStandardError "$libdir\tmp\configure_error.log"
    }
    {$_ -in @("qtexengine", "qwt")} {
        $process = Start-Process -FilePath "cmake.exe" -ArgumentList `
            "-S", "$libdir", "-B", "$libdir/tmp", "-G", '"MinGW Makefiles"', `
            "-DCMAKE_C_COMPILER=$cc", "-DCMAKE_CXX_COMPILER=$cxx", `
            "-DCMAKE_PREFIX_PATH=$qt", `
            "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_SHARED_LIBS=OFF", `
            "-DCMAKE_INSTALL_PREFIX=$install_path", "-DCMAKE_INSTALL_LIBDIR=lib" `
            -PassThru `
            -RedirectStandardOutput "$libdir\tmp\configure.log" `
            -RedirectStandardError "$libdir\tmp\configure_error.log"
    }
    "qwtplot3d" {
        $png_root = "-DPNG_ROOT=" + '"' + $png + '"'
        $process = Start-Process -FilePath "cmake.exe" -ArgumentList `
            "-S", "$libdir", "-B", "$libdir/tmp", "-G", '"MinGW Makefiles"', `
            "-DCMAKE_C_COMPILER=$cc", "-DCMAKE_CXX_COMPILER=$cxx", `
            "-DCMAKE_PREFIX_PATH=$qt", $png_root, `
            "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_SHARED_LIBS=OFF", `
            "-DCMAKE_INSTALL_PREFIX=$install_path", "-DCMAKE_INSTALL_LIBDIR=lib" `
            -PassThru `
            -RedirectStandardOutput "$libdir\tmp\configure.log" `
            -RedirectStandardError "$libdir\tmp\configure_error.log"
    }
    "tamuanova" {
        $gsl_hdrs = "-DGSL_HEADERS=" + '"' + $gsl + '"'
        $process = Start-Process -FilePath "cmake.exe" -ArgumentList `
            "-S", "$libdir", "-B", "$libdir/tmp", "-G", '"MinGW Makefiles"', `
            "-DCMAKE_C_COMPILER=$cc", "-DCMAKE_CXX_COMPILER=$cxx", `
            "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_SHARED_LIBS=OFF", `
            $gsl_hdrs, `
            "-DCMAKE_INSTALL_PREFIX=$install_path", "-DCMAKE_INSTALL_LIBDIR=lib" `
            -PassThru `
            -RedirectStandardOutput "$libdir\tmp\configure.log" `
            -RedirectStandardError "$libdir\tmp\configure_error.log"
    }
    default {
        $process = Start-Process -FilePath "cmake.exe" -ArgumentList `
            "-S", "$libdir", "-B", "$libdir/tmp", "-G", '"MinGW Makefiles"', `
            "-DCMAKE_C_COMPILER=$cc", "-DCMAKE_CXX_COMPILER=$cxx", `
            "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_SHARED_LIBS=OFF", `
            "-DCMAKE_INSTALL_PREFIX=$install_path", "-DCMAKE_INSTALL_LIBDIR=lib" `
            -PassThru `
            -RedirectStandardOutput "$libdir\tmp\configure.log" `
            -RedirectStandardError "$libdir\tmp\configure_error.log"
    }
}
$process.WaitForExit()

$process = Start-Process -FilePath "cmake.exe" -ArgumentList `
    "--build", "$libdir\tmp", "--parallel", "$cores" -PassThru `
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
