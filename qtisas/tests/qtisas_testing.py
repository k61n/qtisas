"""Helpers for the Python tests, which qtisas runs with -x --quit (see tests/CMakeLists.txt).

A test imports them after putting this folder on sys.path (every test is two folders below it):

    import os
    import sys

    sys.dont_write_bytecode = True  # no __pycache__ in the source tree
    sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
    from qtisas_testing import locale_german, set_app_locale, skip
"""

import contextlib
import ctypes
import os
import traceback

import qti

try:
    from PyQt6.QtCore import Q_ARG, QCoreApplication, QEvent, QLocale, QMetaObject, Qt, QTimer
    from PyQt6.QtGui import QAction
    from PyQt6.QtWidgets import QApplication, QCheckBox, QComboBox, QLineEdit, QMessageBox, QPushButton
except ImportError:
    from PyQt5.QtCore import Q_ARG, QCoreApplication, QEvent, QLocale, QMetaObject, Qt, QTimer
    from PyQt5.QtWidgets import QAction, QApplication, QCheckBox, QComboBox, QLineEdit, QMessageBox, QPushButton

DIRECT = getattr(Qt, "ConnectionType", Qt).DirectConnection
_DEFERRED_DELETE = getattr(QEvent, "Type", QEvent).DeferredDelete
_OPTIONS = getattr(QLocale, "NumberOption", QLocale)
_LANGUAGE = getattr(QLocale, "Language", QLocale)
_COUNTRY = getattr(QLocale, "Country", QLocale)


# --- results -------------------------------------------------------------------------------------------------------

def report(text, first=False):
    """One line on stderr: shown by ctest -V or --output-on-failure; first=True starts with an empty line."""
    os.write(2, (("\n" if first else "") + text + "\n").encode())


def skip(reason):
    """Ends the script; CTest reports the test as skipped (SKIP_REGULAR_EXPRESSION in tests/CMakeLists.txt)."""
    report("QTISAS_TEST_SKIPPED: " + reason)
    raise SystemExit(0)


try:  # C stdio buffers must be flushed around the redirection; Windows has no CDLL(None)
    _fflush = ctypes.CDLL(None).fflush
except (OSError, AttributeError, TypeError):
    _fflush = ctypes.cdll.msvcrt.fflush if os.name == "nt" else (lambda _: 0)


@contextlib.contextmanager
def quiet():
    """Silences what QtiSAS prints to stdout (FitTable's iteration log and timing) inside the block."""
    _fflush(None)
    saved = os.dup(1)
    null = os.open(os.devnull, os.O_WRONLY)
    os.dup2(null, 1)
    try:
        yield
    finally:
        _fflush(None)
        os.dup2(saved, 1)
        os.close(null)
        os.close(saved)


# --- number formats ------------------------------------------------------------------------------------------------

def _locale(locale, omit):
    locale.setNumberOptions(_OPTIONS.OmitGroupSeparator if omit else _OPTIONS.DefaultNumberOptions)
    return locale


def locale_c(omit=True):
    """1,000.0 of Preferences: C, group separators omitted (the default) or shown."""
    return _locale(QLocale.c(), omit)


def locale_german(omit=True):
    """1.000,0 of Preferences."""
    return _locale(QLocale(_LANGUAGE.German, _COUNTRY.Germany), omit)


def locale_french(omit=True):
    """1 000,0 of Preferences."""
    return _locale(QLocale(_LANGUAGE.French, _COUNTRY.France), omit)


def set_app_locale(locale):
    """The application's numerical format, as Preferences sets it (windows convert their numbers)."""
    qti.app.setLocale(locale)
    QApplication.processEvents()


# --- dialogs -------------------------------------------------------------------------------------------------------

@contextlib.contextmanager
def answering(*answers):
    """Answers the modal dialogs that open inside the block, in order, without fixed delays.

    answers[i](dialog) is called for the i-th dialog, every 50 ms until it returns True (e.g. once a file dialog
    has listed its folder). Message boxes in between are closed and their texts collected: the block gets that list.
    An error in an answer ends the block's dialogs and is raised after it.
    """
    pending = list(answers)
    answered = []  # keeps the dialogs answered so far, so that a new one is not mistaken for them
    messages = []
    errors = []

    def check():
        dialog = QApplication.activeModalWidget()
        if dialog is None:
            return
        try:
            if isinstance(dialog, QMessageBox):
                messages.append(dialog.text())
                accept(dialog)
            elif pending and all(dialog is not d for d in answered) and pending[0](dialog):
                answered.append(dialog)
                pending.pop(0)
        except Exception:
            errors.append(traceback.format_exc())
            pending.clear()
            QMetaObject.invokeMethod(dialog, "reject", DIRECT)

    timer = QTimer()
    timer.timeout.connect(check)
    timer.start(50)
    try:
        yield messages
    finally:
        timer.stop()
        timer.timeout.disconnect(check)
        # PyQt deletes the slot's proxy later: now, while Python runs, not in exit() after it (crash)
        QCoreApplication.sendPostedEvents(None, _DEFERRED_DELETE)
    assert not errors, "an answer failed:\n" + "".join(errors)
    assert not pending, "%d dialog(s) did not open" % len(pending)


def accept(dialog):
    """QDialog.accept(), also for a dialog QtiSAS created (PyQt does not call its protected override directly)."""
    QMetaObject.invokeMethod(dialog, "accept", DIRECT)


def accept_text(text):
    """An answer for an input dialog."""
    def answer(dialog):
        dialog.setTextValue(text)
        accept(dialog)
        return True
    return answer


def accept_file(folder, file_name):
    """An answer for a file dialog (Qt's own, see use_qt_dialogs()): opens the folder, types the file name, accepts."""
    def answer(dialog):
        if os.path.realpath(dialog.directory().absolutePath()) != os.path.realpath(folder):
            dialog.setDirectory(folder)
            return False
        dialog.findChild(QLineEdit, "fileNameEdit").setText(file_name)  # as typed: selectFile() skips a focused edit
        accept(dialog)
        return not dialog.isVisible()
    return answer


def use_qt_dialogs():
    """Qt's own file dialogs, which answering() can fill in, instead of the platform's."""
    QApplication.setAttribute(getattr(Qt, "ApplicationAttribute", Qt).AA_DontUseNativeDialogs)


# --- settings ------------------------------------------------------------------------------------------------------

@contextlib.contextmanager
def user_settings_kept():
    """Restores the user's qtisas.ini after the block: Preferences' Apply saves the settings."""
    ini = os.path.join(os.path.expanduser("~"), ".config", "qtisas", "qtisas.ini")
    saved = open(ini, "rb").read() if os.path.isfile(ini) else None
    try:
        yield
    finally:
        if saved is not None:
            with open(ini, "wb") as f:
                f.write(saved)


def apply_preferences(number_format=None, omit_thousands=None, muparser_c=None):
    """Sets the given options in the Preferences dialog and presses Apply; None leaves an option unchanged.

    number_format is an item of the format combo: "1,000.0", "1.000,0" or "1 000,0". Use it inside
    user_settings_kept().
    """
    def answer(dialog):
        if number_format is not None:  # the first combo with these items: the clipboard's follows
            combo = next(b for b in dialog.findChildren(QComboBox) if b.findText(number_format) >= 0)
            combo.setCurrentIndex(combo.findText(number_format))
        boxes = {b.text(): b for b in dialog.findChildren(QCheckBox)}
        if omit_thousands is not None:
            boxes["Omit &Thousands Separator"].setChecked(omit_thousands)
        if muparser_c is not None:
            boxes["mu&Parser uses C locale settings"].setChecked(muparser_c)
        next(b for b in dialog.findChildren(QPushButton) if b.text() == "&Apply").click()
        QMetaObject.invokeMethod(dialog, "reject", DIRECT)
        return True

    with answering(answer):
        QMetaObject.invokeMethod(qti.app, "showPreferencesDialog", DIRECT)


def show_windows_of_active_folder():
    """The folders' "View Windows" option the tests assume: the windows of the active folder are shown."""
    action = QAction(qti.app)
    action.setData(1)  # ApplicationWindow::ActiveFolder
    QMetaObject.invokeMethod(qti.app, "setShowWindowsPolicy", DIRECT, Q_ARG(QAction, action))
