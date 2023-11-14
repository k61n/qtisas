# v0.9.13

 * ci: enable python in darwin builds
 * build python bindings with sip >= 6
 * qualify Python3.11
 * use QSettings::NativeFormat on macos
 * readme: upd for MacOS build
 * ci: macos build
 * ci: bionic 0.0.4 qt5.9.5 qt5.15 compatible
 * qt5: fix missing includes
 * qt5: delete obsolete QPixmap method from ui files
 * use QTransform instead of QMatrix for qt > 5.9.5
 * pass compilers to scripts that build libraries
 * libqwtplot3d: comply qt5.15 on macos 13
 * libqwt: comply qt5.15 on macos 13
 * libqtexengine: comply qt5.15 on macos 13

# v0.9.12

 * readme: ubuntu, qt5
 * ci 0.0.4 qt5
 * fix Python3 scripting for qt5
 * QLCDNumber complies qt5
 * Fix duplicate object names in .ui files
 * Delete empty z-order assignments
 * qt5 support
 * QString: toAscii and fromAscii methods are deprecated
 * QWorkspace is QMdiArea in qt5
 * QVariant complies qt5
 * QtGui is renamed to QtWidgets in qt5
 * qtcolorpickler: fix headers
 * QTabWidget complies qt5
 * Qt::WFlags renamed to Qt::WindowFlags in qt5
 * QPrinter::PostScriptFormat doesn't exist in qt5
 * QMenu complies qt5
 * QLineEdit complies qt5
 * QInputDialog complies qt5
 * QHttp is deprecated in qt5
 * QHeaderView complies qt5
 * QFileDialog complies qt5
 * QAction complies qt5
 * qt5: fix missing header includes
 * use nullptr instead of NULL
 * replace TRUE and FALSE macro
 * libqwtplot3d: build with qt5
 * libqwt: build with qt5
 * libqtexengine: build with qt5
 * scripting: set default to python
 * sans/compile: fix wrong Qt flag
 * update README.rst
 * scripting: set default to python
 * table: fix mouse events
 * qt4: remove <qt4 code compatibility
 * libyaml-cpp is a git submodule
 * libtiff: use libtiff from the system
 * libqtexengine updated
 * libmuparser is a git submodule
 * added README
 * ci 0.0.2: contains autoconf
 * adjust libgsl building routine
 * libgsl is a git submodule
 * delete libgsl to be added as git submodule

# v0.9.11

 * qt4: fix build for mac
 * qt4: drop qt3 support
 * qt4: muParserScripting.cpp rfc to remove compiler errors in qt4
 * qt4: QWidget drop qt3 suppor members
 * qt4: QToolBar drop qt3 support members
 * qt4: QTextStream drop qt3 support members
 * qt4: QTextEdit drop qt3 support members
 * qt4: QTabWidget drop qt3 support members
 * qt4: fix Qt3 Namespace
 * qt4: QStringList drop qt3 support members
 * qt4: QString drop qt3 support members
 * qt4: QSpinBox drop qt3 support members
 * qt4: QSettings drop qt3 support members
 * qt4: QRegExp drop qt3 support members
 * qt4: QRect drop qt3 support members
 * qt4: QPrinter calls comply qt4
 * qt4: QPixMap drop qt3 support members
 * qt4: QPalette drop qt3 support members
 * qt4: QPainter drop qt3 support members
 * qt4: QObject drop qt3 support members
 * qt4: QMouseEvent drop qt3 support members
 * Table.cpp fix copy/delete selections
 * qt4: QMenu drop qt3 support members
 * qt4: QMap drop qt3 support members
 * qt4: QList drop qt3 support members
 * qt4: QLibrary drop qt3 support members
 * qt4: QKeyEvent drop qt3 support members
 * qt4: QInputEvent drop qt3 support members
 * qt4: QInputDialog drop qt3 support members
 * qt4: QGridLayout drop qt3 support members
 * qt4: QtGlobal drop qt3 support members
 * qt4: QFont drop qt3 support members
 * qt4: QFileInfo drop qt3 support members
 * qt4: QFileDialog drop qt3 support members
 * qt4: QFile drop qt3 support members
 * qt4: QEventLoop drop qt3 support members
 * qt4: QDrag drop qt3 support members
 * qt4: QDir drop qt3 support members
 * qt4: QDebug comply to qt4
 * qt4: QComboBox drop qt3 support members
 * qt4: QApplication drop qt3 support members
 * qt4: QAction drop qt3 support members
 * qt4: QAbstractSlider drop qt3 support members
 * qt4: QAbstractButton drop qt3 support members
 * qt4: let Folder class return objectName() as name()
 * qt4: QVariant drop qt3 support members
 * use QTreeWidget and QTreeWidgetItem instead of Q3ListView and Q3ListViewItem for Folder related classes
 * use QTableWidget instead of Q3Table for MyTable and Table classes
 * buildlib script: cores has default value 1
 * container to build qtisas on ubuntu bionic
 * qtisas build pipeline
 * Use QTableWidget instead of Q3Table for ImportASCIIDialog
 * Use QPaintDevice instead of Q3PaintDeviceMetrics
 * Use QVector instead of Q3MemArray
 * Use QMap instead of Q3AsciiDict
 * Use QDrag instead of Q3DragObject, drop q3iconview

# v0.9.10

 * drop qmake support
 * python3 support
 * fix compiler errors on macos
 * Merge branch 'master' into 'master'
 * - Implemented new 'terminal' command: radial - Corrected some bugs in Curves Dialog
 * Implemented terminal-like widget. Commands rn (rename), rm (remove), as well matrix calculator were implemented.
 * rfc CMakeLists.txt

# v0.9.9

 * fix cmake build with Python
 * fix case in header filename
 * fix 3rdparty scripts for macos
 * cmake build is fixed
 * build with cmake
 * silent output during compilation
 * qmake builds adjusted
 * some scripts not needed
 * garbage files deleted
 * libtiff: should be built withoug ljpeg and lzma
 * qwtplot3d: references qwt3d_surfaceplot.h are removed
 * scripting: fix include
 * libqwt3d: fix compile bugs
 * most of libs can be build with a single script if cmake install definitions are present
 * libtamuanova: renamed the folder, CMakeLists.txt now has install definitions
 * broken build due to commit cc2f16da, revert
 * libgsl: needs proper installation
 * core: delete duplicated files
 * libminigzip: a separate static lib
 * fittable: fix jnse18.h path
 * genmin: garbage file deleted
 * dan: findFileNumberInFileName returns Qstring::number(bool) instead bool
 * dan: unused method declaration is commented now
 * replace \| with \\
 * proper linking of libOpenGL for libqwtplot3d
 * printf replaced with qDebug()
 * auto_ptr is replaced with unique_ptr
 * QComboBox() and PatternBox() do not accept any argument during init
 * Seems like markers are removed by a dedicated removeMarker method
 * cursors.xmp must be somewhere in resources
 * setFlags doesn't accept bool parameter
 * remove build files
 * build scripts: delete build folder before creating it
 * libtiff is built using cmake
 * libtamuanova is a cmake project
 * libqwtplot3d: pass -DBUILD_SHARED_LIBS=OFF
 * libqwt: pass -DBUILD_SHARED_LIBS=OFF
 * libqtexengine: pass -DBUILD_SHARED_LIBS=OFF
 * libmuparser is a cmake project
 * cmake or qmake may execute scripts from different relative paths, this change helps
 * libalglib is a cmake project
 * scripts return 0 when the lib has been already built
 * remove some build files
 * restored missing lib component of qtisas
 * Libraries are build into 'libs' folder now
 * license
 * Remove build file
 * libyaml-cpp build script
 * libtiff build script
 * Removed build files for libtiff library
 * libtamuanova build script
 * Removed build files for libtamuanova library
 * libqwtplot3d build script
 * libqwt build script
 * libqtexengine build script
 * libmuparser build script
 * libgsl build script
 * gitignore
 * alglib build script
 * gitignore
 * Removed build files for muparser library
 * Removed build files for alglib library
 * Removed build files for gsl library
 * Merge branch 'master' into 'master'
 * this commit includes several changes (last 3 monthes)
 * Merge branch 'master' into 'master'
 * initial git upload
 * gitignore updated
 * gitignore added
 * gitignore added
