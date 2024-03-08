# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
# Description: build sip files
# **************************************************************************** #

import os
import platform
import subprocess
import sys
import venv


file = os.path.abspath(__file__)
qtisas_root = os.path.dirname(os.path.dirname(file))
ext = '.exe' if platform.system() == 'Windows' else ''


def find_resource(resource, paths):
    for path in paths:
        for root, dirs, filenames in os.walk(path):
            for filename in filenames:
                if filename == resource:
                    return os.path.join(root, filename)


def sip4():
    import sipconfig
    from PyQt5.QtCore import PYQT_CONFIGURATION

    cfg = sipconfig.Configuration()
    builddir = sys.argv[1]
    os.makedirs(os.path.join(builddir, 'sip', 'qti'), exist_ok=True)
    sipcmd = f'{cfg.sip_bin} -I {cfg.default_sip_dir}/PyQt5 ' \
             f'{PYQT_CONFIGURATION["sip_flags"]} -c ' \
             f'{os.path.join(builddir, "sip", "qti")} ' \
             f'{os.path.join(qtisas_root, "qtisas", "python", "sip", "qti", "qtimod.sip")}'
    subprocess.run(sipcmd, cwd=qtisas_root, shell=True)


def sip():
    import sipbuild

    # checks if the files are already built
    builddir = sys.argv[1]
    if os.path.exists(os.path.join(builddir, 'sip', 'sip.h')):
        print('Sip files are already generated.\n')
        return

    folders = ['/usr']
    if platform.system() == 'Darwin':
        folders = ['/opt/homebrew'] if platform.machine() == 'arm64' \
            else ['/opt/homebrew-x86', '/usr/local']
    sipexe = find_resource('sip-build', folders)
    sip_includes = find_resource('QtCoremod.sip', folders)
    if sipexe and sip_includes:
        sip_includes = os.path.dirname(os.path.dirname(sip_includes))
    else:
        print(f'\nCreating a venv to build sip files:')
        venv_root = os.path.join(qtisas_root, 'bin', 'venv')
        venv.create(venv_root, with_pip=True)
        pipexe = find_resource('pip3' + ext, [venv_root])
        subprocess.run(f'{pipexe} install pyqt5 pyqt-builder sip', shell=True)
        sipexe = find_resource('sip-build' + ext, [venv_root])
        sip_includes = find_resource('QtCoremod.sip', [venv_root])
        sip_includes = os.path.dirname(os.path.dirname(sip_includes))
        if platform.system() == 'Windows':
            sip_includes = sip_includes.replace('\\', '/')

    pyproject = f'''
# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
# Description: sip project file generated by scripts/sip_generate.py 
# **************************************************************************** #

[build-system]
requires = ["sip >=5, <7", "PyQt-builder >=1.6, <2"]
build-backend = "sipbuild.api"

[tool.sip.metadata]
name = "qti"
requires-dist = "PyQt5 (>=5.15)"

[tool.sip]
project-factory = "pyqtbuild:PyQtProject"

[tool.sip.project]
sip-include-dirs = ["{sip_includes}"]
'''

    with open(os.path.join(qtisas_root, 'qtisas', 'python', 'pyproject.toml'),
              'w') as f:
        f.write(pyproject)

    # executes sip-build
    os.makedirs(os.path.join(builddir, 'sip'), exist_ok=True)
    buildflag = '--no-compile' if sipbuild.version.SIP_VERSION >= 0x60000 else '--no-make'
    sipcmd = f'{sipexe} {buildflag} --qmake {sys.argv[2]} --build-dir {os.path.join(builddir, "sip")}'
    subprocess.run(sipcmd, shell=True, cwd=os.path.join(qtisas_root, 'qtisas', 'python'))


if __name__ == '__main__':
    try:
        sip()
    except ModuleNotFoundError:
        sip4()
