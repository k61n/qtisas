"""Duplicate window (ApplicationWindow::clone): the same result for every window type."""

try:
    from PyQt6.QtWidgets import QApplication
except ImportError:
    from PyQt5.QtWidgets import QApplication

caption_policy = getattr(MDIWindow, "CaptionPolicy", MDIWindow)


def make(kind, name):
    if kind == "Table":
        return newTable(name, 5, 2)
    if kind == "Matrix":
        return newMatrix(name, 5, 5)
    if kind == "Note":
        return newNote(name)
    if kind == "Graph":
        return newGraph(name)
    return newPlot3D(name, "sin(x)*cos(y)", -3, 3, -3, 3, -1, 1, 20, 20)


def duplicate(w):
    nw = clone(w)
    assert nw is not None, "clone() of %s returned None" % w.objectName()
    QApplication.processEvents()
    return nw


for kind in ["Table", "Matrix", "Note", "Graph", "Graph3D"]:
    # Maximized source: the copy is maximized, the source is restored behind it
    src = make(kind, "Max" + kind)
    src.setMaximized()
    QApplication.processEvents()
    assert src.isMaximized(), "%s: source not maximized before duplicate" % kind
    copy = duplicate(src)
    assert copy.objectName() == "Max%s-copy1" % kind, "%s: copy named %s" % (kind, copy.objectName())
    assert copy.isMaximized(), "%s: copy of a maximized window is not maximized" % kind
    assert not src.isMaximized(), "%s: source still maximized after duplicate" % kind

    # Normal source: the copy gets the source's size, label and caption policy
    src = make(kind, "Norm" + kind)
    src.setNormal()
    src.resize(333, 222)
    src.setWindowLabel("label-" + kind)
    src.setCaptionPolicy(caption_policy.Both)
    QApplication.processEvents()
    copy = duplicate(src)
    assert copy.objectName() == "Norm%s-copy1" % kind, "%s: copy named %s" % (kind, copy.objectName())
    assert not copy.isMaximized(), "%s: copy of a normal window is maximized" % kind
    assert (copy.width(), copy.height()) == (333, 222), "%s: copy size %dx%d" % (kind, copy.width(), copy.height())
    assert copy.windowLabel() == "label-" + kind, "%s: copy label %r" % (kind, copy.windowLabel())
    assert copy.captionPolicy() == caption_policy.Both, "%s: copy caption policy %r" % (kind, copy.captionPolicy())
