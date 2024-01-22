# v0.9.17

 * rework git tree
 * clang_format: generates header info for c++ files and compares the pushed one
 * upd gitignore
 * yaml-cpp was not removed from .gitmodules
 * libgsl submodule update

# v0.9.16

 * rework git tree
 * clang_format: skip files which can't be read
 * rfc file information headers
 * macOS releases: safe handling of python version number
 * readme: upd

# v0.9.15

 * release: doesn't create tags anymore, tags are created by gitlab-ci job
 * ci: windows release is bundled with proper hdf5 and yaml libs
 * ci: explicit cmake options
 * ci: use images from qtisas-ci
 * DAN: fix formatting and sast errors
 * clang-format: skip some cases
 * bump C++ standard to 14
 * DAN: adding of YAML headers is re-written
 * DAN: adding of ASCII/FLEXI headers is re-written
 * DAN/HDF5: matrix reading implemented; building instructions added
 * Fixes: #4. DAN: updated functions to YAML/new-API > v.0.5.0
 * DAN: Path control is fully moved to 'FilesManager'
 * DAN: parsing of tof/rt related values is shifted to 'Tofrt'
 * DAN: parsing of normalization related values is shifted to 'Monitors'
 * DAN: 'cleanning' of header-reading old functions
 * DAN: listOfHeaders is moved completely to 'parserHeader'
 * DAN: comparison of sample positions is moved to 'Sample'
 * DAN: comparison of det X/Y (or beam) positions is moved to 'Detector'
 * DAN: read Attenuator/Polarization/Lenses and is moved to 'Collimation'
 * DAN: Detector x/y (and rotation) parsing is reorganized
 * DAN: Selector/Lambda  parsing is reorganized
 * DAN: function 'findFileNumberInFileName(...)' is moved to filesManager
 * DAN: checkFileNumber() is moved to filesManager->checkFileNumber()
 * DAN: parsing of sample-related-information is reorganized
 * Usage of (dan)newFileNameUni() is moved to filesManager->newFileNameFull()
 * Usage of fileNameUni() is finally moved to filesManager->fileNameFull()
 * DAN: R1/R2 (radious of  sample/collimation aparture) parsing is reorganized
 * DAN: SA/CA (sample/collimation apartures) parsing is reorganized
 * DAN: C (collimation) parsing is reorganized
 * DAN: D (or SDD) parsing from a header is reorganized
 * dan.ui: renamed comboBoxUnitsCandD to comboBoxUnitsD
 * DAN: parsing of headers are splitted to the separated object
 * YAML parser: back to older cpp standard
 * DAN: Files-managment-functions are splitted to a new separated object
 * .clang-tidy: remove modernize-loop-convert
 * Removed 'empty' file dan-init-interface.cpp
 * YAMLYAML flag removed
 * sast.py: fix incorrect handling of deleted files
 * clang_format.py: fix incorrect handling of deleted files
 * DAN-OPTIONS: HDF5 options implemented to the instrument configuration
 * DAN: transparent switch between ASCII and BINARY detector image implemented
 * DAN-OPTIONS: Added dead-time fields for flux monitors
 * DAN: ASCII parser's functions are defined in parser-ascii.h/cpp files
 * DAN: YAML parser function is defined in parser-yaml.h/cpp files
 * DAN: XML parser function is defined in parser-xml.h/cpp files
 * DAN: HDF5 parser's functions are defined in parser-hdf5.h/cpp files
 * sast.py: handle when clang-tidy exits with status 1
 * clang_format.py: fix when `git diff` results in empty output
 * DAN-OPTIONS: Collimation Units are implemented
 * hdf5: add new dependency

# v0.9.14

 * gitlab ci: create release on tag pushed to main repo
 * gitlab ci: sync job on merge_requests rfc
 * current qtisas version is 0.9.13
 * CMakeLists.txt: python resources files are copied to macos bundle only if the project is build with python support
 * python script to prepare a release
 * rfc qtisas about logic
 * clang_format: call diff() with all=True
 * ci: new jammy build, new util image for .pre and sync stages
 * ci: bionic image upd
 * ci: renamed jobs
 * ci: rocky8 build
 * ci: rocky9 image upd
 * rfc readme
 * .rpm package generation with cpack
 * .deb packages generation with cpack
 * fix osx related definitions
 * clang_format: use `--patience` option for `git diff` to improve quality of the output
 * windows installer using Qt Installer Framework
 * rfc python configuration file
 * python configuration files on macos are bundled in .app
 * CMakeLists.txt: .app bundle is built on macos by default
 * qtisasrc.py: some problems with import of scipy functions
 * update README.md
 * add windows11 x86_32 build
 * new darwin intel runner
 * DAN:comboBoxSel Instrument ComboBox functionality is improved
 * DAN:Options:Calibrant Calibrant ComboBox works correctly now
 * DAN:SANS1 instrument settings of SANS1 is changed
 * DAN: Transmission of EC is allowed to be large than 1.0 (Tr < 1.5)
 * Dan@Merge: generation of merged-tables is improved
 * radAvTableGeneration@DAN: optimization of the QI-table generation
 * Graph.cpp: 'emit modifiedGraph()' instead of 'modifiedGraph()'
 * Fittingfittable: slightly changed format
 * Rawdata@Dan: crash by creation/use of the new mask is solved
 * DAN@mask/sensitivity comboboxes: show correctly existing matrixes
 * changeFolder(...): works correctly by calling from SAS-interfaces
 * CMakeLists: generation of sip files reworked
 * ci: sast stage updated
 * ci: clang-format is performed on new pushed code
 * upd readme
 * ci: sync qtisas/qtisas with github mirror on merges
 * ci: static analysis is performed on new pushed code
 * sip: rfc generate script
 * EnrichmentDialog:QTabWidget: no signal  currentChanged(widget*)
 * fittable-simulate*.cpp: cleanned structure of the files
 * compile-explorer.cpp: cleanned structure of the file
 * Fortran@Compile@MacOS&Linux: works now co-compilation of Fortran functions
 * readme: instruction to build on Windows
 * windows build supports python
 * working windows build
 * CMakeLists.txt: simplifying code
 * windows build
 * windows build
 * libtamuanova: cmake build accepts path to gsl headers as a cmake argument
 * QDir comply qt5 syntax
 * gsl is build with cmake now and so the scripts are updated
 * readded submodules
 * remove submodules
 * COMPILE: FORTRAN tools are hidden for MacOS now
 * Compile: clang is defaulf for MacOS now; compile-init is "cleaned" now
 * Compile-Code-Menu: adopted QMenu actions to QT5 logic
 * MagicTemplate@Multilayer: crash after template's selection is solved
 * Compile.Table: default defenision of  pathGSL variable is changed
 * MyTable::isRowSelected #include <iostream> removed
 * MyTable::isRowSelected is now works correctly with full=TRUE
 * dan@generateMergingTable col type is defined BEFORE input content
 * Table::adjustColumnsWidth(bool) corrected in case if comment is ON
 * Infotable@DAN "---" replaced by "" to prevent warning message
 * Rawdata@Extractor@DAN Reading Monitor-3 crash is solved
 * Table to PDF / Table Print Preview: vertical header is shown now
 * Fit.Compile@MacOS compilation with Fortran functions is updated
 * Open Dialogs: crash problem solved
 * MacOS-crash after requesting of the menu inside a NOTE is solved
 * Bug / Plot-Selected-Columns /  Active Table / Corrected
 * Folders: Delete-Selection bug corrected
 * muParser Worksheet-Column-Calculation problem is solved
 * Scripting menu is displayed now on macOS
 * Compile options for macOS updated
 * 'PyQt4' string replaced by 'PyQt5'
 * updated .gitignore
 * char* metaObject()->className() values were converted to QString
 * fittable: generation table fix
 * scripting: unset python as default
 * readme: instructions to build on rocky9
 * ci rocky9-0.0.1
 * ci: add current container version for bionic-0.0.4
 * sip: workaround to build on rocky9
 * sip-build.sh: remove line left during debugging
 * find sip-build script: now works on rocky9
 * find sip script: consider sip6 as default
 * buildlib script: force cmake install libs to lib/ folder
 * ci: enable darwin intel builds
 * sip: change the script to look for sip-build
 * libgsl: force deletion of missing script

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
 * Implemented new 'terminal' command: radial - Corrected some bugs in Curves Dialog
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
 * qtisas init
 * qtiplot -> qtisas
