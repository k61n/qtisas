# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2006 Knut Franke <knut.franke@gmx.de>
#     2006 Ion Vasilief <ion_vasilief@yahoo.fr>
#     2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
# Description: default configuration file of QtiSAS' Python environment
# **************************************************************************** #

import builtins
import os
import platform
import sys

from qti import *


if platform.system() == 'Darwin':
    sys.path.append(os.path.join(os.path.dirname(sys.executable), '..', 'Resources'))
elif platform.system() == 'Linux':
    sys.path.append(os.path.join(os.path.expanduser('~'), '.config', 'qtisas', 'python'))
else:
    sys.path.append(os.path.join(os.path.dirname(sys.executable), 'python'))


def import_to_global(modname, attrs=None, math=False):
    """Imports functions to global namespace. If math==True, also import's to
    QtiSAS's math functions list."""

    try:
        mod = __import__(modname)
    except ModuleNotFoundError:
        print(f'No module named {modname}')
        return
    for submod in modname.split('.')[1:]:
        mod = getattr(mod, submod)
    if not attrs:
        attrs = [f for f in dir(mod) if '__' not in f]
    for name in attrs:
        f = getattr(mod, name)
        builtins.__dict__[name] = f
        # make functions available in QtiSAS's math function list
        if math and callable(f):
            qti.mathFunctions[name] = f
    print(f'Imported {len(attrs)} functions from {modname}.')


if __name__ == '__main__':
    # import selected methods of ApplicationWindow into the global namespace
    appImports = [
        'activeFolder', 'addFolder', 'appendProject', 'changeFolder', 'clone',
        'copyFolder', 'currentGraph', 'currentMatrix', 'currentNote',
        'currentTable', 'deleteFolder', 'displayInfo', 'graph',
        'importWaveFile', 'infoLineEdit', 'matrix', 'matrixToTable', 'newGraph',
        'newMatrix', 'newNote', 'newPlot3D', 'newTable', 'note', 'openTemplate',
        'plot', 'plot3D', 'plotImageProfiles', 'resultsLog', 'rootFolder',
        'saveAsTemplate', 'saveFolder', 'saveProjectAs', 'setPreferences',
        'setWindowName', 'stemPlot', 'table', 'tableToMatrix',
        'tableToMatrixRegularXYZ', 'waterfallPlot', 'workspace',
    ]
    for name in appImports:
        builtins.__dict__[name] = getattr(qti.app, name)

    # import utility module
    import_to_global('python-qtiUtil', math=True)

    # Import standard math functions and constants into global namespace.
    import_to_global('math', math=True)

    # Import scipy.special (if available) into global namespace
    import_to_global('scipy.special', math=True)

    # Provide easy access to SymPy, for symbolic mathematics
    import_to_global('sympy')

    print('Python scripting engine is ready.')
