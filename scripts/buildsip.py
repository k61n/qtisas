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

import PyQt5

file = os.path.abspath(__file__)
qtisas_root = os.path.dirname(os.path.dirname(file))


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


def find_resource(resource, paths):
    for path in paths:
        for root, dirs, filenames in os.walk(path):
            for filename in filenames:
                if filename == resource:
                    return os.path.join(root, filename)


def sip6():
    import sipbuild

    # generates pyproject for sip-build
    pyqt_root = str(PyQt5).split("'")[3]
    if platform.system() == 'Windows':
        pyqt_root = pyqt_root.split('\\\\')
        pyqt_root[0] += '\\'
        pyqt_root = os.path.join(*pyqt_root)
    sip_includes = find_resource('QtCoremod.sip', [os.path.dirname(pyqt_root)])
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
requires = ["sip >=6, <7", "PyQt-builder >=1.6, <2"]
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

    # find sip-build
    if platform.system() == 'Linux':
        sipexe = find_resource('sip-build', ['/usr', '/home', '/root'])
    else:
        sip_path = str(sipbuild).split("'")[3]

        if platform.system() == 'Windows':
            sip_path = sip_path.split('\\\\')
            sip_path[0] += '\\'
            search_dir = os.path.join(*sip_path[:sip_path.index('Python') + 1])
            sipexe = find_resource('sip-build.exe', [search_dir])
        else:
            search_dir = os.path.join('/', *sip_path.split('/')[:3])
            sipexe = find_resource('sip-build', [search_dir])

    builddir = sys.argv[1]
    os.makedirs(os.path.join(builddir, 'sip'), exist_ok=True)
    sipcmd = f'{sipexe} --no-compile --qmake {sys.argv[2]} --build-dir {os.path.join(builddir, "sip")}'
    subprocess.run(sipcmd, shell=True, cwd=os.path.join(qtisas_root, 'qtisas', 'python'))


if __name__ == '__main__':
    try:
        sip6()
    except ModuleNotFoundError:
        sip4()