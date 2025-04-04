# v0.11.3

 * applicationwindow: consoleWindow is docked bottom by default now
 * fittable: weighting/poly/reso widgets handling optimized
 * compile: reading functions in windows with EOL '\r\n'
 * settings: re-organized project settings
 * ApplicationWindow: default appStyle variable definition changed
 * compile: in menu qtisas/sasview/fortran subfolders are possible now
 * compile: 'IncludedFunctions' could have subfolders now
 * compile: added button to download fit.finctions from git-repository
 * sans/common: a repository synchronization with a local folder
 * compile: hidden buttons to select 1D or 2D function type
 * gitlab-ci: windows deploy line modified
 * compile: several functions optimized
 * compile: compile.ui optimized
 * compile: logic of building functions simplified
 * compile: function pathUpdate() removed
 * compile: optimized updateFiles() function
 * compile: compiling script for windows modified
 * fittable: implemented sub-folders in fit.function explorer
 * compile: implemented sub-folders in fit.function explorer
 * compile/fittable: common functions for fit.function.explorer
 * compile: path to fitting functions could be changed for current session
 * compile:tidy-up: compile-init.cpp
 * compile: compileSingleFunction() renamed to buildSingleFunction()
 * compile: makeDLL() renamed to buildSharedLibrary()
 * compile: setting permissions of compile.script moved to makeCompileScript()
 * compile: function makeBATnew() renamed to makeCompileScript()
 * compile: defaultOptions() function simplified
 * compile: script BAT.BAT renamed to compile.script.bat
 * compile: modified qprocess with chmod: u+x used instead of 777
 * updated *app() function in sas-tools
 * parser-xml.cpp: fix deprecated `setContent` definitions
 * PythonScripting.cpp: clean-up
 * sasPath and templatesDir are predefined values
 * change default folders for python
 * improve builds against python
 * upd readme.md

# v0.11.2

 * python: ApplicationWindow::autoArrangeLayers() function is added
 * compile/fittable: implemented usage of the python script after fit
 * compile: openFIFfile function is optimized
 * compile: python-in-fit.function option improved for macos and windows

# v0.11.1

 * ci: post-release fix of linux release builds
 * ci: post-release fix of macos release job

# v0.11.0

 * upd linux release rules
 * ci: rework windows release job and add arm64 release
 * ci: rework macos release job
 * ci: simplify macos job code
 * changes in setting up the project with cmake
 * build with ninja in linux jobs
 * add windows-arm64 job
 * fix narrowing from type 'qsizetype' (aka 'long long') to 'long'

# v0.10.10

 * added the missing muparser library to the windows/python release
 * compile: compilation command modified

# v0.10.9

 * Table: muparser/python script is correctly selected by activation "Recalculate" in the column menu
 * ApplicationWindow::init(): in the python version, muparser could be also used as the default scripting language
 * dan/rt/tof: qstring argument is passed correctly
 * Project Explorer: drag-and-drop works now
 * dan:script-table: optional "Suffix" colomn used correctly now
 * dan:script-table: optional "Background" colomn read correctly now
 * dan/rawdatatools/tof/rt: bug in the file splitting solved
 * dan/rawdatatools/tof/rt: corrected QRegularExpression multi-capture() usage
 * dan/ParserASCII/readEntryFlexy: return now also string starting position
 * ConfigDialog: updated the magicMenu of all multilayers after modifying of the sasPath
 * multilayer: updateMagicMenu() function implemented
 * ApplicationWindow: added function multilayerList()
 * dan/rt/tof: qprogressdialogs are forced to be closed
 * compile/windows: gsl compile option updated to match commit 737b9ec3
 * CMakeLists.txt: the gsl library will be included in the osx bundle after compilation
 * compile/BAT.BAT: the path to the functions is now in quotation marks (linux/osx)
 * build.md: update build instructions for macos
 * adjust libgsl bundling for windows releases
 * use system muparser library
 * ascii1d: the line style is not updated when added a second time
 * compile: added an option for python compilation flags
 * windows installer updated
 * fittable: bug by openning fitting session solved
 * ApplicationWindow::saveWindow: note "fitCurve-*-scriptAfterFit" is saved with  "fitCurve-*" table
 * fittable: note "fitCurve-*-afterFitScript" is now  executed after pushing "Script:: After Fit" button
 * fittable: by re-generation of the table after fit, a curve in the active plot is not re-inserted
 * use original muparser repo
 * CMakeLists.txt: simplify
 * clang-tidy: remove some checks
 * clang-tidy all: Note
 * improve Note class
 * clang-tidy all: ScriptWindow
 * improve ScriptWindow class
 * refactor LineNumberDisplay class
 * clang-tidy all: Cone3D
 * clang-tidy all: ColorMapPreviewDialog
 * clang-tidy all: ColorMapEditor
 * clang-tidy all: ColorMapDialog

# v0.10.8

 * ascii.1d: by re-reading the same table, a curve in the active plot is not re-inserted
 * ascii.1d: set value "---" to a numerical worksheet not allowed, "" used instead
 * ascii.1d: QRegularExpression actions were corrected
 * dan: SANS1 istrumens name pattern is changed
 * dan: in makeScriptTable: polarizations are correctly compared now
 * dan: readSettingNew function is rewritten
 * dan: absolute calibrant H2O[SANS-1] is improved

# v0.10.7

 * ApplicationWindow::activateWindow() is improved
 * findDataTable, findActivePlot, findActiveGrap, and addCurve functions are re-organized
 * fittable: undo/redo functionality is completely rewritten - all parameters of the fitting table are reachable during undo/redo actions
 * compile: info-editor was improved
 * compile: independent variable is global within the fitting function in the c++ code
 * clang-tidy all: ColorButton
 * clang-tidy all: CollapsibleGroupBox
 * clang-tidy all: ChiSquareTest
 * clang-tidy all: CanvasPicker
 * clang-tidy all: BoxCurve
 * revert patch 709870a1 due to broken functionality
 * bump qwtplot3d
 * clang-tidy all: BaselineDialog
 * clang-tidy all: AxesDialog
 * clang-tidy all: AssociationsDialog
 * clang-tidy all: Bar
 * clang-tidy all: ArrowMarker
 * clang-tidy all: Anova
 * clang-tidy all: AnovaDialog
 * clang-tidy all: AddWidgetTool
 * clang-tidy all: PlotToolInterface
 * python: fix issues with PyQt versions in ApplicationWindow::openQtDesignerUi
 * python: fix exportTableToTeX for modern Qt
 * build.md: update build instructions
 * readme: Fedora 41 repo was added
 * readme: remove link for windows builds with python
 * remove unused imports

# v0.10.6

 * COMPILE: linux/macos/windows compilation adjustments

# v0.10.5

 * EnrichmentDialog: improved latex equation generation
 * COMPILE: macos and linux compilation is updated to global gsl library status
 * ApplicationWindow::activateWindow(QString) is optimized
 * Graph3D: opened correctly in a minimized status
 * ApplicationWindow::activateWindow(QString name) function improved
 * ProgressDialog: corrected the incorrect counting of open windows in the project
 * Graph/SymbolDialog: buttonClicked signal is depricated in qt6 - idClicked is used instead
 * DAN/Megre: interface improved
 * DAN/Rawdata tools/Extract Raw-Matrices: added an option to extract matrices as doubles
 * upd build instructions
 * gsl: use modern gsl
 * ci: use alpine/git image for git jobs
 * ci: don't run .pre and build stages after merge
 * ci: adjust linux builds

# v0.10.4

 * ci/release: adjust release jobs for mac

# v0.10.3

 * ci: parametrize macos-arm64 job
 * win release: libwinpthread-1.dll was missing in the package
 * ci: parametrize images for linux builds
 * ci: sast job runs on macos arm64 runners
 * build.txt: change download link for windows libs

# v0.10.2

 * COMPILE: updated generation of the script for compilation of a fitting function
 * switch-case instead of multiple if-else statements in dan18-init
 * dan18::checkTableRange function is modified
 * ApplicationWindow::checkTableExistence: const QString& instead of QString
 * DAN:merge: join/rebin options are clearly integrated to a single interface
 * Table: added function 'int indexOf(const QString& label)'
 * DAN: Improved logic for data generation after the 'I[Q]::Output format' selection was changed.
 * Table::addColumns: bug corrected
 * ExportDialog: improved interface
 * QRegularExpression: interactive wildcard is not 'static const'
 * DAN/fastInfoExtractor: comboBoxCheck is now saved to and loaded from the settings
 * SAS tools: Improved font scaling
 * DAN: comboBoxSel is renamed to comboBoxInstrument
 * DAN: selectMode, selectModeTable and advUser functions are optimized
 * DAN: existWindow(QString) moved to ApplicationWindow.cpp
 * DAN: maximizeWindow(QString) moved to ApplicationWindow.cpp
 * DAN: findSANSinstruments() updated
 * DAN: kws1ORkws2() function is renamed to instrumentSelectedByButton()
 * ASCII1D: comboBoxFastPlot is always in focus after selection
 * DAN:dan.ui: improved interface for macOS
 * DAN/HDF: Reading of different HDF array formats has been implemented
 * DAN/Header::Map: The first column is stretchable now
 * DAN: hiding all info fields after starting/loading the session
 * DAN: QRegularExpression rx0(REGEXPS::wildcardToRE(...)) is not 'static const'
 * DAN/PN: Sample, Buffer, and EC masks are sinchronized now
 * Plot3DDialog is always OnTop now
 * clang_format: fix of another fail
 * clang_format: fix of fail of parsing the data
 * modified DAN layout (tabWidgets instead of toolBoxes)
 * ci: macos qt6 job builds with python

# v0.10.1

 * ci: macos qt6 release lacks some links
 * public-obs: package-meta to be set to `no` to build

# v0.10.0

 * ci: win releases with qt6
 * ci: win qt6 builds
 * unset UNICODE for builds in win
 * upd muparser
 * clang-tidy: remove `hicpp-exception-baseclass` rule
 * use QString::toLocal8Bit instead of QString::toLatin1
 * ci: macos qt6 release build
 * rpm: use rhel macro instead of rocky
 * rpm: proper condition for rocky variable
 * upd debian configs for OBS
 * ci: rearrange jobs
 * upd readme and build instruction on qt6
 * cpack: on rpm systems python3-qt5 but python3-pyqt6
 * debian: drop Ubuntu 23.10, upd debian12 and ubuntu24.04 with qt6
 * CMakeLists.txt: variable must be in brackets
 * rpm spec: builds with qt5 for rocky and qt6 for fedora
 * CMakeLists.txt: make sure to use only 1 core when cmake is running in gitlab
 * CMakeLists.txt: print system information for verbosity
 * enable qt6 builds on ubuntu24.04 and debian12
 * upd qwtplot3d
 * use QPrintDialog::options instead of deprecated ::enabledOptions
 * drop support of ubuntu18 and qt < 5.12
 * enable builds with qt6
 * add debian11, debian12, ubuntu20.04 and ubuntu22.04 jobs
 * qtimod.sip: Graph has actually Ticks enum not TicksStyle
 * support sip4 in separate qti module
 * qtimod.sip: use Py_ssize_t type
 * no need in Core5Compat lib
 * QComboBox signals comply qt5/qt6
 * add menu About Qt
 * use QComboBox::AdjustToMinimumContentsLengthWithIcon
 * comply QContextMenuEvent to qt5/qt6
 * comply QMouseEvent to qt5/qt6
 * use QFontDatabase static members
 * QVariant complies qt6/qt5
 * add explicit braces to avoid dangling else
 * change in QMenu::addAction since qt 5.6
 * do not use deprecated QMessageBox calls
 * Use QDropEvent::position().toPoint() instead of deprecated QDropEvent::pos()
 * Use QAction::associatedObjects instead of deprecated QAction::associatedWidgets
 * Use custom function to conver Wildcards to QRegularExpression
 * ci: rfc macos jobs
 * CMakeLists.txt: pass properly CMAKE_PREFIX_PATH and PREFER_QT to buildlib.sh
 * CMakeLists.txt: print which QtCore lib is used for verbosity
 * 3party upd
 * ci: macos job with qt6
 * CMakeLists.txt: build qtisas with qt6
 * clang-format: better output
 * Use QRegularExpression instead of QRegExp
 * rename globals.h to version.h
 * modify QKeySequence statements
 * use QScreen instead of QDesktopWidget
 * Use QLayout::setContentsMargins instead of setMargin
 * explicitly send QTreeWidgetItem::Text to QDataStream from QList<QTreeWidgetItem *>
 * add missing includes when compiled against qt6
 * QLocale complies qt6
 * use Qt::MiddleButton instead of Qt::MidButton
 * use QTransform instead of QMatrix
 * QPalette enums are compatible with qt6
 * remove _POSIX_C_SOURCE undef

# v0.9.24

 * upd qwtplot3d
 * DAN/PN mode: fxP definition is changed - fxP = f * P
 * Graph3d: default dot size changed to 1.5/smooth
 * matrix scaling improved
 * DAN: flexiStop option is updated
 * Graph: default axis margin is set to 0
 * Folder: re-name of folders works again
 * ApplicationWindow/addFolder: The logic used for numbering the folder copies has been updated

# v0.9.23

 * plot2D/dialogs/EnrichmentDialog: the aspect ratio is set correctly now
 * rfc CMakeLists.txt for dan module
 * CMakeLists: simplify includes of the qtisas libs
 * genmin cmake rfc
 * remove FITMATRIX
 * remove ORIGIN_IMPORT
 * rfc include sections
 * explicitly use Qwt3D::round function
 * headers macro rfc
 * ci-.pre: consider skipping tag
 * rewrite includes for qwt
 * fix relative includes and remove unused ones
 * remove SCRIPTING_MUPARSER definition
 * remove HAVE_ALGLIB definitions
 * remove TEX_OUTPUT definitions
 * remove HAVE_TAMUANOVA definitions
 * ci-sast: make sast script to compile libs on every run
 * rewrite includes for tamuanova
 * rewrite includes for qwtplot3d
 * rewrite includes for qtexengine
 * rewrite includes for alglib
 * QApplication::setActiveWindow duplicates QWidget::activateWindow call
 * Use QLayout::setContentsMargins instead of setMargin
 * QTextStream encoding is set to UTF8 by default in qt6
 * Use QLocale to convert QDateTime to QString
 * DAN/singleDanMultiButton: removed dependency on scriptTableManager
 * DAN/ScriptSettings: setting of several comboBox's is corrected now
 * DAN/ScriptTable: added optional columns RemoveFirst and RemoveLast
 * buildsip: explicitely add setuptools when venv is created
 * DAN/BugSolved: generation of the merging-script-table in case of selected rows
 * DAN/BugSolved: In the generation of non-standard radial averaging presentations
 * DAN/PNx4: corrections are moved to separated function
 * DAN/PNx2 mode: transmission and matrix corrections are moved to separated functions
 * DAN: script-table-manager activated in the code
 * DAN: most of functions related to script table moved to script-table-manager
 * empty folders in "qtisas" folder are created automatically now
 * Graph3D upd related to qwtplot3d
 * ci: use util:0.0.9 with upd libs
 * upd qwt
 * upd qtexengine
 * upd qwtplot3d

# v0.9.22

 * ci/obs: allow obs job to fail
 * gitlab-ci/release needs finished upload job
 * obs-release: using jammy image since obs doesn't work in noble with python3.12

# v0.9.21

 * DAN/Polarized.Neutrons.Mode: added auto-generation of the merged table
 * DAN: readMergeInfo modified to read also directly from a table
 * DAN: saveSettings renamed to saveScriptSettings
 * resizeColumnToContents changed to adjustColumnsWidth
 * DAN/Polarized.Neuntrons.Mode: all needed fuctions implemented now
 * DAN/Single-Line-in-Script: modified output function
 * DAN/Polarized.Neuntrons.Mode: configuration files update
 * DAN: Modified dan.ui file to include the future Polarized.Neutrons option
 * DAN/Single-Line-in-Script: new functions added
 * DAN/New configuration type: to be used for polarizer/analyzer parameters
 * DAN/singleDan: the method for calculating errors has been changed
 * DAN/dan-parser-header: the number of rows in the header table is no longer fixed
 * SAST and clang-format should not exit with error on fresh rep fork
 * rework About QtiSAS
 * ci: util image is based on ubuntu24.04
 * ci: use ubuntu24.04 instead of ubuntu22.04
 * update readme and build instructions
 * obs: add python3-pyqt5 as runtime dependency
 * obs: retire ubuntu:23.04 support, add ubuntu:24.04 package
 * minigzip: added includes to fix build on fedora 40
 * sip/pyproject.toml: qualify PEP 621 and sip7
 * PyUnicode_EncodeUTF8 and PyUnicode_GET_DATA_SIZE are deprecated since python3.3
 * macos sonoma: fix build with python
 * ApplicationWindow/closeWindow: blocked signal of the closed widget
 * DAN/Processing: added functions to process a single line from a script table
 * DAN/Processing: generation of names of output files changed
 * QSetting Folder is changed from "JCNS" to "qtisas"
 * Project Explorer: header sections are automatically resized to content
 * Project Explorer: sorting of widgets by pressing column headers works now
 * Project Exploler: View Windows Menu works correctly with qt5
 * Plot2d/Axes/Labels/Engineering format: mu-symbol is shown correctly now
 * Matrix/Rotation action: 90/-90 is shown now with degree symbol 90°/-90°
 * Plot2d/Axes/Labels format: scientific format with dot is shown now correctly
 * DAN/Process: added new function to find all unique configurations
 * DAN/Processing: defined general comparison fuction of two configurations
 * DAN: instrument mode combobox behavior is corrected
 * DAN/Processing: two saveSettings(...) functions are merged to one
 * DAN/Processing: two makeScriptTable(...) functions are merged to the one
 * DAN/Processing: two newScriptTable(...) functions are merged to the one
 * DAN: findActiveGraph(...) is used globally defined
 * DAN/Processing: addCopyOfLastConfiguration() function is improved
 * DAN/Processing: vert/horHeaderTableECPressed functions are simplified
 * DAN/Processing: two updateColInScript*(...) functions are merged to single one
 * DAN/Processing: hiding of columns of buffer runs is improved
 * DAN/Processing: script-table-combobox update is improved

# v0.9.20

 * Graph/MultiLayer@WIN@LINUX: The resolution of the vector export corrected
 * compile/windows: changed compilation options

# v0.9.19

 * COMPILE/WINDOWS: compile/link options are updated to current installation
 * COMPILE/LINUX: Implementation of GSL 1.15 as the global library by default
 * ImageProfiles: standard configuration changed, more optimized for sans
 * DAN/Tools: imported matrix is maximized after creation, not after update
 * compile: in linux gsl headers are installed to include/gsl115/gsl
 * ci: add gsl headers to windows bundle
 * ci: automation to trigger release builds on OBS
 * bump container versions
 * upd build instructions
 * package gsl v1.15 for linux bundles
 * ci: upd windows release jobs
 * add new win x64 runner
 * remove libpng dependancy
 * upd minigzip CMakeLists.txt
 * upd qwtplot3d
 * remove unused flag QT_MAC_USE_COCOA
 * fix crash when LegendWidget properties are selected
 * rfc main.cpp
 * FIT/Simulated_Table: individual setColumnType command
 * FIT/SANS_mode/Bug: wrong structure was used in Levenberg fit mode
 * FIT/SANS mode: polydispersity function is correctly updated now
 * DAN: Polar table generation modified
 * Graph/MultiLayer: The resolution of the vector export is fixed to the screen resolution
 * `info.indexOf("|w|"-3)` looks like a bug
 * headers check handles deleted files
 * new class XMLParser uses QXmlStreamReader and replaces deprecated Xml classes
 * rfc CMakeLists for 120 character width
 * move CustomXMLParser to src/parsers/parser-xml
 * rfc src/parsers
 * move parsers to src/
 * rfc classes using QXmlDefaultHandler
 * upd cmake policies
 * upd readme
 * upd OBS builds
 * QWheelEvent::posistion is available since qt 5.14
 * silence GL deprecation warning only on MacOS
 * upd sipbuild script
 * replace ASCII1D COMPILE DAN FITTABLE JNSE SVD definitions with QTISAS
 * upd qwtplot3d, and silence OpenGL deprecation warnings on MacOS
 * qtexengine is a git submodule
 * qwtplot3d is a git submodule
 * qwt is a git submodule
 * Graph3D: correct update of the color maps
 * Graph3D: preventing crashes caused by infinite calls to setScaledColors()
 * fix broken by a4837231 muParserScripting::mathFunction
 * remove confusing include
 * using QFileDialog.birthTime instead of created() since qt 5.10
 * replace deprecated qt_mac_set_dock_menu
 * QColorDialog.getRgba is deprecated, using getColor()
 * QFileDialog.setConfirmOverwrite() is deprecated, using setOption
 * QDirModel is deprecated, using QFileSystemModel
 * QLable.pixmap() by pointer is deprecated
 * fix returning reference to local temporary object
 * QPixmap::grabWidget is deprecated, using QWidget.grab
 * QWeelEvent.x() and .y() are deprecated, using position().x() and y()
 * QWheelEvent.delta is deprecated, using angleDelta() instead
 * get rid of setAutoCompletion method in QComboBox
 * QTextEdit method `setTabStopWidth` is deprecated
 * fix deprecated QPrinter methods
 * PyEval_InitThreads() doesn't do anything since python 3.7 and is deprecated since 3.9
 * array should be deleted with delete[]
 * fix confusing assignments inside condition
 * QDateTime(QDate) is deprecated, using QDate::startOfDay() for qt >= 5.14
 * use std::sort instead qSort
 * use QLocale instead QDate to translate number into names of days and months
 * fix 'swap' is deprecated: Use QList<T>::swapItemsAt()
 * use proper QProcess::start call
 * 'operator=' is deprecated in QDir, using setPath()
 * fix ignored return value of function
 * QDesktopWidget is obsolete class, using QGuiApplication
 * fix unknown excape sequences
 * fix unused expressions
 * unused comparison
 * use uint16_t and uint32_t instead of uint16 and uint32
 * using QElapsedTimer instead of QTime
 * QString::sprintf is deprecated, using asprintf()
 * use snprintf instead of deprecated sprintf
 * fix printf mismatch between format and argument
 * fix qDebug expression
 * fix `non-void function does not return a value`
 * fix `ISO C++11 does not allow conversion from string literal to 'char *'`
 * fix logic triggering -Wlogical-not-parentheses copiler warning
 * override proper functions
 * QFontMetrics::width is deprecated, using horizontalAdvance() instead
 * fix QString::split deprecation warnings
 * fix QFlags deprecation warning
 * remove QString::null deprecated occurances
 * clang_format: headers are not checked for 3rdparty files
 * add deb specs for builds in OBS
 * sip5 support
 * add rpm spec for builds in OBS
 * use containers with cmake coming from native reps
 * remove testing command in CMakeLists
 * Individual setting of the column type instead of common (table) one
 * FIXES: #1. 3d graphs are now visible
 * FIXES: #13. solved a crash during adding function curve
 * Graph/Options/Axis/Title: text is shown now in the plain text mode
 * LineDialog: signal currentChanged(QWidget*) changed to currentChanged(int)
 * AxesDialog: signal currentChanged(QWidget*) changed to currentChanged(int)
 * Fit.Curve(s): the logic for simulating the fitting curve has been changed
 * Fit.Curve(s): Q-Factor is integrated now to fitCurve-table and fitLabel
 * Fit.Curve(s): supressed resizing to content of the data table
 * FIXES: #22. re-set of data fit range after "Fitting Session" <->"General Results"
 * Preferences/QtiSAS: Default-interface comboBox works correctly now
 * Preferences/QtiSAS: Image-format comboBox works correctly now
 * DAN/FitTable: Removed local versions of checkTableExistence(...)
 * ApplicationWindow::saveWindow modified to co-save fit-statistic-note
 * checkNoteExistence() function  moved from DAN to ApplicationWindow
 * ApplicationWindow::noteList() new function implemented
 * Graph/AddText/UnicodeButton rewritten conversion from QString to char*
 * MultiLayer/confirmRemoveLayer/QMessageBox 'QtiPlot' changed to 'QtiSAS'
 * Multilayer: solved a program crash after copy of the selected graph
 * Graph: solved a program crash after adding of the text or equation
 * FitTable: added Qfactor parameter to test goodness of a fit
 * Latex@Graph correctly implemented now

# v0.9.18

 * Fixes #9. 'void Table::clearSelection()' was rewritten to work correctly
 * Fixes #8. Table: delete selected rows works now corrrectly
 * Fixes: #7. Double-clicking on a column header in the table works in all cases
 * Fixes: #6. 2d new graph axis offset problem is solved
 * dan18::addNmatrixesUni is simplified
 * fix bug when DAN crashed on selecting a output folder
 * fix bug when DAN crashed on creating a mask
 * SVD interface was updated
 * ApplicationWindow, added function: bool checkTableExistence(QString)
 * Sip@Python: several functions added to python bindings
 * ApplicationWindow.cpp: removed double-defined lines
 * macOS: python spripts are located now in Resource/python-scripts in the bundle
 * SASPLUGINS: removed obsoleted functions
 * fittable.ui: slightly corrected
 * Settings read/save is corrected withing different places
 * macOS: gsl headers/library are added to the Resources folder of the bundle
 * macOS: python files moved from Resources to Resource/python in the bundle
 * DAN: dan-related folders moved from qtisas/sans/ to qtisas/sans/dan/
 * pass ".pre=skip" in commit description to skip the .pre CI stage
 * cmake version is 3.10
 * CMakeLists: rework linux installation specs
 * file info headers are modified in every qtisas src file
 * buildsip script uses sip from packages by default
 * QMatrix is used until qt5.12.8
 * qtexengine: always use QString::SkipEmptyParts

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
