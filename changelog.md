# v0.12.1

 * improved build system with updated CMake and Python requirements
 * dropped outdated dependencies, and switched to Qt6 on macOS
 * compile: tools updated for macOS and Windows (MSVC)
 * compile: with support for multiple Fortran files and integrated script calling
 * compile: enhanced scripting features, SasView fitting functions integration
 * compile: repository sync improvements

# v0.12.0

 * gl2ps library now comes from original sources
 * Qtisas for Windows is built now with MSVC 2022
 * Python3 finally works in Windows

# v0.11.3

 * sub-folder support was added to fit.function explorer, IncludedFunctions
 * qtisas/sasview/fortran menus, along with common functions for compile/fittable
 * Windows compiling script was modified; BAT.BAT was renamed to compile.script.bat
 * sasPath and templatesDir are now predefined values
 * project settings were reorganized
 * default appStyle variable definition was updated
 * repository synchronization with a local folder was added
 * a button to download fit.functions from a Git repository was implemented

# v0.11.2

 * Python scripts can be used with fits

# v0.11.1

 * Follow-up fixes of release jobs for Linux and macOS

# v0.11.0

 * Optimize CI build system
 * Qtisas has arm64 Windows release

# v0.10.10

 * Follow-up fixes in Windows releases

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
 * compile/BAT.BAT: the path to the functions is now in quotation marks (linux/osx)
 * adjust libgsl bundling for windows releases
 * ascii1d: the line style is not updated when added a second time
 * compile: added an option for python compilation flags
 * fittable: bug by openning fitting session solved
 * ApplicationWindow::saveWindow: note "fitCurve-*-scriptAfterFit" is saved with  "fitCurve-*" table
 * fittable: note "fitCurve-*-afterFitScript" is now  executed after pushing "Script:: After Fit" button
 * fittable: by re-generation of the table after fit, a curve in the active plot is not re-inserted
 * libgsl is correctly copied to macOS app bundle
 * installer for Windows now creates links in the Start menu
 * muparser library dependency now comes from official sources
 * Note and ScriptWindow functionality was revised and improved
 * Some of the code was cleaned-up to suppress clang-tidy warnings

# v0.10.8

 * ascii.1d: by re-reading the same table, a curve in the active plot is not re-inserted
 * ascii.1d: set value "---" to a numerical worksheet not allowed, "" used instead
 * ascii.1d: QRegularExpression actions were corrected
 * dan: SANS1 istrumens name pattern is changed
 * dan: in makeScriptTable: polarizations are correctly compared now
 * dan: readSettingNew function is rewritten
 * dan: absolute calibrant H2O[SANS-1] is improved

# v0.10.7

 * findDataTable, findActivePlot, findActiveGrap, and addCurve functions are re-organized
 * Undo/redo functionality was improved in Fittable module
 * Compile module was improved
 * Fixed regression from 709870a1
 * qwtplot3d library was updated
 * Fixed exportTableToTeX and openQtDesignerUi python functions
 * Some of the code was cleaned-up to suppress clang-tidy warnings
 * Some unused imports were removed form the code

# v0.10.6

 * Imported functionality of Compile module for Linux, macOS, and Windows

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
 * gsl library dependency now comes from official sources
 * Optimize CI build system

# v0.10.4

 * Follow-up fixes of macOS release jobs

# v0.10.3

 * Optimize CI build system
 * Follow-up fixes of Windows release jobs

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

 * Follow-up fixes of release jobs for Linux and macOS

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

 * Follow-up updates to CI

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
 * Qualify Python3.12
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

 * drop Qt4 support
 * Gsl v1.15 is distributed with releases
 * Linux releases are build in public OpenSUSE Build Service for Debian, Rockylinux, and Ubuntu
 * In Windows Qtisas can be built with MinGW 8.1.0 x64 compiler
 * Fixed various bugs and crashes
 * COMPILE/WINDOWS: compile/link options are updated to current installation
 * COMPILE/LINUX: Implementation of GSL 1.15 as the global library by default
 * ImageProfiles: standard configuration changed, more optimized for sans
 * DAN/Tools: imported matrix is maximized after creation, not after update
 * FIT/Simulated_Table: individual setColumnType command
 * FIT/SANS_mode/Bug: wrong structure was used in Levenberg fit mode
 * FIT/SANS mode: polydispersity function is correctly updated now
 * DAN: Polar table generation modified
 * Graph/MultiLayer: The resolution of the vector export is fixed to the screen resolution
 * clang_format: headers are not checked for 3rdparty files
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
 * Added application link for Qtisas for Linux

# v0.9.17

 * Git tree was reworked
 * Optimized CI build system

# v0.9.16

 * Git tree was reworked
 * New file information headers are introduced
 * Optimized CI build system

# v0.9.15

 * DAN: fix formatting and sast errors
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
 * DAN: Files-managment-functions are splitted to a new separated object
 * DAN-OPTIONS: HDF5 options implemented to the instrument configuration
 * DAN: transparent switch between ASCII and BINARY detector image implemented
 * DAN-OPTIONS: Added dead-time fields for flux monitors
 * DAN: ASCII parser's functions are defined in parser-ascii.h/cpp files
 * DAN: YAML parser function is defined in parser-yaml.h/cpp files
 * DAN: XML parser function is defined in parser-xml.h/cpp files
 * DAN: HDF5 parser's functions are defined in parser-hdf5.h/cpp files
 * DAN-OPTIONS: Collimation Units are implemented
 * hdf5: add new dependency

# v0.9.14

 * Updated Qtisas about output
 * Static analysis and code formatting checks are introduced in CI on new pushed code
 * Added possibility to generate .deb and .rpm packages with cpack
 * macOS:
   * Scripting menu is displayed now
   * app bundle is built by default
   * compile module uses clang
 * Windows:
   * Qtisas is built with MinGW 8.1.0 32 bit compiler
   * Qtisas is released via installer file made using Qt Installer Framework
 * Fixed problems importing scipy functions in python
 * DAN module got many improvements
 * Updated Fortran functions in Compile module
 * Various bugs and crashes are fixed

# v0.9.13

 * Fully qualify Qt5.15
 * Fully qualify Python3.11
 * Fully qualify sip6
 * Enable python for macOS builds

# v0.9.12

 * Fully qualify Qt5.9
 * Various bugs are fixed

# v0.9.11

 * Gitlab CI jobs are introduced
 * Fully qualify Qt4, Qt3 support is dropped

# v0.9.10

 * Dropped support of qmake build system
 * Python3 now is used instead of Python2
 * Qtisas can be built on macOS now
 * New terminal-like widget is added to Compile and Dan modules

# v0.9.9

 * Qtisas changes applied on QtiPlot repo
 * Introduced new build script to build each 3rd party lib
 * 3rd party libs are build with cmake
 * Qtisas build is adjusted with qmake
 * Introduced fully functional cmake build system
 * Fixed various bugs
