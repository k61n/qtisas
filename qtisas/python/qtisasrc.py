# **************************************************************************** #
# Project: QtiSAS
# License: GNU GPL Version 3 (see LICENSE)
# Copyright (C) by the authors:
#     2006 Knut Franke <knut.franke@gmx.de>
#     2006 Ion Vasilief <ion_vasilief@yahoo.fr>
#     2023 Konstantin Kholostov <k.kholostov@fz-juelich.de>
#     2025 Vitaliy Pipich <v.pipich@gmail.com>
# Description: default configuration file of QtiSAS' Python environment
# **************************************************************************** #

import builtins
import os
import platform
import sys

from qti import *


if platform.system() == 'Windows':
    sys.path.append(os.path.join(os.path.expanduser('~'), 'AppData', 'Local', 'qtisas', 'python'))
else:
    sys.path.append(os.path.join(os.path.expanduser('~'), '.config', 'qtisas', 'python'))


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
        # scipy.special re-exports a numpy ufunc named "round", which would silently replace
        # the Python builtin round() below — breaking round(x, ndigits) for every script (Notes,
        # AI, console, ...) for the rest of the session with a cryptic "TypeError: return arrays
        # must be of ArrayType" (numpy's round ufunc takes a different calling convention: the
        # second positional arg is an output array, not a digit count). Confirmed live. Keep the
        # real builtin — scipy.special.round(...) is still reachable via the qualified name.
        if modname == 'scipy.special':
            attrs = [a for a in attrs if a != 'round']
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
        'activateWindow', 'activeFolder', 'addCustomAction', 'addFolder', 'Ascii1d',
        'appendProject', 'autoArrangeLayers',
        'autoUpdateTableValues', 'changeFolder', 'clone', 'closeProject', 'closeWindow',
        'colorIndex', 'colorNames', 'columnsList', 'Compiler', 'copyFolder',
        'currentGraph', 'currentMatrix', 'currentNote',
        'currentTable', 'customActionNames', 'customActionsPath', 'Dan', 'deleteFolder',
        'displayInfo', 'existGraph', 'existMatrix', 'existNote', 'existPlot3D', 'existTable',
        'existWindow', 'Fittable',
        'graphWindow', 'graphs', 'hideWindow', 'importWaveFile', 'infoLineEdit', 'Jnse',
        'addRule', 'matrices', 'matrix', 'matrixNames', 'matrixToTable', 'maximizeWindow',
        'newGraph', 'newMatrix', 'newNote', 'newPlot3D', 'newProject', 'newTable',
        'openProject',
        'note', 'notes', 'openTemplate', 'plot', 'plot3D', 'plotImageProfiles',
        'removeCustomAction', 'removeRule', 'removeAllRules', 'resultsLog', 'rootFolder', 'runCustomAction',
        'updateCustomActionName', 'updateCustomActionScript',
        'updateCustomActionIcon', 'updateCustomActionMenu', 'saveAsTemplate',
        'saveFolder', 'saveGraphAsProject',
        'saveProject', 'saveProjectAs', 'scriptPrint', 'scriptPrintReplace', 'setAutoUpdateTableValues',
        'setPreferences', 'setWindowName', 'showRules', 'stemPlot', 'Svd', 'symbolIndex', 'symbolNames',
        'table', 'tableNames', 'tables',
        'tableToMatrix', 'tableToMatrixRegularXYZ', 'waterfallPlot', 'windows', 'workspace',
    ]
    for name in appImports:
        builtins.__dict__[name] = getattr(qti.app, name)

    # scriptPrint() like print(): any values, converted with str() and joined by sep; scriptPrint(None) stays an
    # empty line. The ApplicationWindow slot itself takes one str only
    def scriptPrint(*args, sep=" "):
        if len(args) == 1 and args[0] is None:
            args = ()
        qti.app.scriptPrint(sep.join(str(a) for a in args))

    builtins.scriptPrint = scriptPrint

    # import utility module
    import_to_global('python-qtiUtil', math=True)

    # Import standard math functions and constants into global namespace.
    import_to_global('math', math=True)

    # ln() alias for natural log: matches MuParser where ln(x) = log_e(x)
    builtins.ln = math.log
    qti.mathFunctions['ln'] = math.log

    # C/C++ style boolean aliases
    builtins.true = True
    builtins.false = False

    # Import scipy.special (if available) into global namespace
    import_to_global('scipy.special', math=True)

    # Provide easy access to SymPy, for symbolic mathematics
    import_to_global('sympy')

    print('Python scripting engine is ready.')
