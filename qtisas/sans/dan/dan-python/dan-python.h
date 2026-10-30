/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: DAN-SANS Python interface: one object per tab (Dan.Options, Dan.Rawdata, Dan.Mask, Dan.Sensitivity,
             Dan.Processing, Dan.Merge)
 ******************************************************************************/

#ifndef DAN_PYTHON_H
#define DAN_PYTHON_H

#include <QPair>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QtNumeric>

class dan18;
class QDoubleSpinBox;

// Auto mode (from qtikws's DAN-BlackBox): the runs of the input folder sorted into the roles of a reduction by the
// rules of the Options > Auto-Mode page, which an instrument saves as "[AutoMode-...]" lines. The parts' auto calls
// (Dan.Rawdata.info(), Dan.Processing.autoConfigurations(), ...) use it.
class DanAuto
{
  public:
    explicit DanAuto(dan18 *dan) : d(dan)
    {
    }
    struct Run
    {
        QString run, file, sample, name, CA, SA, polarization, polarizationAliased, attenuator;
        // name: as in the header, sample: cut; polarization remains the raw header value
        double duration = 0, counts = 0, C = 0, D = 0, lambda = 0, detectorX = 0, detectorY = 0, rotationX = 0,
               rotationY = 0;
        int position = 0;  // C, D [m]
        QStringList added; // a run made by adding others (Rawdata "Add"): those runs, from its header's comment
    };
    struct Choice
    {
        Run emptyBeam;
        QString darkCurrent, calibrant, calibrantEmptyBeam, calibrantDarkCurrent, center, cellEmptyBeam;
        bool transmission = false;
    };

    // the options: Python names, values as on the page; setOptions() returns the error or nothing
    QVariantMap options() const;
    QString setOptions(const QVariantMap &values);
    void resetOptions();
    // the instrument's lines "[AutoMode-<key>] <value>", and one such line read (false: no such key)
    QStringList optionLines() const;
    bool readOptionLine(const QString &key, const QString &value);

    // the runs of the input folder, their headers read; filtered: by the page's run filters
    QList<Run> runs(bool filtered) const;
    static QList<Run> filter(const QList<Run> &all, int minDuration, int minCounts, const QString &files,
                             const QString &skipFiles, const QString &samples, const QString &skipSamples);
    static bool matches(const QString &name, const QString &wildcards);
    // the headers runs() read are kept (a run read again when its detector file changed): forgotten when a header
    // file is written (Rawdata.setHeaderValue())
    static void forgetRuns();
    // "C8D7.68L4.9BS30x30|10x10" (+ "P<polarization>", "A<attenuator>" when they count): what the configuration
    // wildcards (transmissions in, skip) are matched against
    QString label(const Run &r) const;
    bool sameConfiguration(const Run &a, const Run &b, bool ignoreD = false) const;
    QList<QList<Run>> conditions(const QList<Run> &runs) const;
    QList<Run> standardRuns(const QList<Run> &runs) const;
    QList<Run> sampleRuns(const QList<Run> &runs) const;
    // the configurations: one per condition of the empty beams; dropped: the reasons for the conditions left out
    QList<Choice> configurations(const QList<Run> &runs, QStringList &dropped) const;
    // the empty beams of the configurations (in their order, the dropped ones left out)
    QList<Run> emptyBeams(const QList<Run> &runs) const;
    // "mask-bs-9.2m" / "mask-tr-9.2m" (+ "-<lambda>A" when another empty beam has the same D)
    static QString maskName(const Run &emptyBeam, bool transmission, const QList<Run> &emptyBeams);

  private:
    Run readRun(const QString &run, const QString &file) const;
    QString sampleName(const QString &name) const;
    dan18 *d;
};

// What every tab object shares: its DAN, and why its last call made nothing. A call runs DAN without dialogs
// (dan18::beginScriptRun); when it made nothing it returns false or an empty name and lastError() says why.
class DanPart
{
  public:
    explicit DanPart(dan18 *dan) : d(dan)
    {
    }
    virtual ~DanPart() = default;
    QString lastError() const
    {
        return m_lastError;
    }
    // DAN's messages during the last call (warnings too when it succeeded)
    QStringList lastMessages() const
    {
        return m_lastMessages;
    }

  protected:
    // the DAN tab of this part (Options 0, Rawdata 1, Mask 2, Sensitivity 3, Processing 4, Merge 5): a call shows it,
    // as a click would, and runs what the tab's selection runs (its lists refreshed)
    virtual int tab() const = 0;
    // starts a script run with the part's tab active
    void begin();
    // ends the script run; false and lastError() = the reason (or DAN's messages) when ok is false
    bool end(bool ok, const QString &reason = QString());
    QString fail(const QString &reason);
    bool isMask(const QString &name) const;
    bool isSens(const QString &name) const;
    QString noFile(const QString &file, const QString &given, bool mask) const;
    // auto mode's intermediate results, one line in the terminal QtiSAS runs in ("DAN auto: ...")
    static void report(const QString &line);

    dan18 *d;
    QString m_lastError;
    QStringList m_lastMessages;
};

// Options tab: session, instrument, mode, folders and the methods the examples change
class DanOptionsConfig;
class DanProcessingOptions;

class DanOptions : public DanPart
{
  protected:
    int tab() const override
    {
        return 0;
    }

  public:
    using DanPart::DanPart;
    DanOptionsConfig *config = nullptr; // Dan.Options.Config
    void newSession();
    bool openSession(const QString &scriptTable);
    QStringList instruments() const;
    QString instrument() const;
    bool selectInstrument(const QString &name);
    QStringList modes() const;
    bool selectMode(const QString &mode);
    static QStringList polarizationReductionModes();
    QString polarizationReductionMode() const;
    bool selectPolarizationReductionMode(const QString &mode);
    bool setInputPath(const QString &path);
    QString inputPath() const;
    bool setOutputPath(const QString &path);
    QString outputPath() const;
    // The session as a script continues it — instrument, folders, configurations, masks, sensitivities, script tables —
    // or empty when the project holds nothing of DAN (no mask, sensitivity or script table)
    QString state() const;
    // the current instrument saved under a name of its own (its .SANS file and .cpp), as "Save instrument" does
    bool saveInstrument(const QString &name);
};

// Options tab, its instrument configuration (Dan.Options.Config): the methods the examples change
class DanOptionsConfig : public DanPart
{
  protected:
    int tab() const override
    {
        return 0;
    }

  public:
    using DanPart::DanPart;
    static QStringList centerMethods();
    QString centerMethod() const;
    bool setCenterMethod(const QString &method);
    QStringList transmissionMethods() const;
    QString transmissionMethod() const;
    bool setTransmissionMethod(const QString &method);
    // auto mode's options (the Auto-Mode page), by name: minDuration, emptyBeam, center, masks, ...
    QVariantMap autoOptions() const;
    bool setAutoOptions(const QVariantMap &values);
    // the instrument's whole configuration, as its .SANS file holds it: "Options-1D-RemoveLast" -> "5", ...
    QVariantMap parameters();
    // some of them set, as selecting the instrument sets them (checked: a value the interface does not take fails)
    bool setParameters(const QVariantMap &values);
};

// Rawdata Tools tab: the runs of the input folder and header (info) tables
class DanRawdata : public DanPart
{
  protected:
    int tab() const override
    {
        return 1;
    }

  public:
    using DanPart::DanPart;
    QStringList runs() const;
    // the runs whose duration [s] and counts are above the minimums, file and sample names matching
    QStringList runs(int minDuration, int minCounts, const QString &files, const QString &skipFiles,
                     const QString &samples, const QString &skipSamples);
    // per run: run, sample, duration, counts, C, D, lambda, beamSize, polarization, attenuator, detectorX, detectorY
    QVariantList info(const QStringList &runs);
    // the runs grouped by configuration (as the script table groups them): label, C, D, lambda, beamSize, runs
    QVariantList conditions(const QStringList &runs);
    // runs next to each other in the folder of the same sample and DAN configuration: n 0 every whole series (2 runs
    // or more), n >= 2 the series in groups of n (6 such runs: 2 groups of 3); runs made by adding left out; per
    // group: label, condition, sample, runs
    QVariantList neighbours(int n, const QStringList &runs);
    // the runs minus every standard run (empty beams, dark currents, calibrants, sensitivity) by auto mode's rules
    QStringList sampleRuns(const QStringList &runs);
    QStringList headerItems() const;
    // the header map's keys ("Sample-Title", "Duration", "Polarization", ...): the header's own values (as DAN reads
    // them, before any conversion), read and written by these two
    QStringList headerKeys() const;
    QString headerValue(const QString &run, const QString &key);
    // the value written into each run's header file where the header map reads it (every format), read
    // back; backup: <file>.bak first, when there is none; the files written
    QStringList setHeaderValue(const QStringList &runs, const QString &key, const QString &value, bool backup = false);
    // the runs' files, full paths in the order given: the detector file and its own header file (KWS-3 .det, .yaml)
    QStringList files(const QStringList &runs);
    QString newHeaderTable(const QString &name, const QStringList &items, const QString &runs);
    // "Add :: Raw Files": the runs summed into one new run (matrix and header: duration, counts, monitors);
    // run empty: <first run>0000<N>; the files written (in the output folder)
    QStringList addRuns(const QStringList &runs, const QString &run);
    // the runs of the same sample and DAN configuration, wherever they are in the folder (2 runs or more; added runs
    // left out); per group: label, condition (as numbered by conditions()), sample, runs
    QVariantList sampleGroups(const QStringList &runs);
    // "Generate Merge Table": sampleGroups() as a table, a column each (first row: the new run), to edit and add
    // minimum: the runs a group needs to get a column (the button: 1, every selected run shown)
    // consecutive: "Only consecutive runs" (-1 as set, 0/1): a column per series of neighbours()
    QString newAddingTemplate(const QStringList &runs, int minimum = 2, int consecutive = -1);
    // the Add buttons on such a table: by "columns" (Two Step Add) or by "rows" (Quick Add); the files written
    // skipSingle: "Skip single runs" (-1 as set, 0/1)
    QStringList addRunsFromTable(const QString &table, const QString &by, int skipSingle = -1);
    // "Header(s) - to - Info-Table" (added to an info table of that name); the options: -1 as set, 0/1
    QString newInfoTable(const QString &name, const QStringList &runs, int compact, int matrices, int detectorSum);
    // "Image(s) - to - Info-Matrix" (added to an info matrix of that name); columns < 1 and the options -1: as set
    QString newInfoMatrix(const QString &name, const QStringList &runs, int columns, int mask, int sens, int norm,
                          int roi);
    // "Extract :: Raw-matrixes": the files written; asDouble -1: as set
    QStringList extractMatrices(const QStringList &runs, int asDouble);

  private:
    // the runs as files of the input folder; empty and lastError() set when one is missing
    QStringList runFiles(const QStringList &runs);
    // one run's files: the detector file and its own header file
    QStringList filesOf(const QString &run) const;
    // sampleGroups() / neighbours() of groups of at least minimum runs (the template: 1)
    QVariantList sampleGroups(const QStringList &runs, int minimum);
    QVariantList neighbours(int n, const QStringList &runs, int minimum);
};

// Mask tab
class DanMask : public DanPart
{
  protected:
    int tab() const override
    {
        return 2;
    }

  public:
    using DanPart::DanPart;
    // edge, beamstop: x1 x2 y1 y2 (pixels, 1-based); empty: as set (the instrument's default), {0}: none
    // triangles, deadRows, deadCols: as the fields take them; a null string: as set
    QString newMask(const QString &name, const QList<int> &edge, const QList<int> &beamstop, const QString &shape,
                    const QString &triangles, bool transmission, const QString &deadRows = QString(),
                    const QString &deadCols = QString(), const QString &edgeShape = QString());
    QString loadMask(const QString &name, const QString &file);
    // the tools of "Adv Masking Options" on any matrix: center x, y and sizes in pixels (1-based), value set inside
    // (or outside) the shape
    QString setValue(const QString &matrix, double value);
    QString rectangle(const QString &matrix, double x, double y, double sizeX, double sizeY, bool inside, double value);
    QString sector(const QString &matrix, double x, double y, double phiFrom, double phiTo, bool inside, double value);
    QString ellipse(const QString &matrix, double x, double y, double sizeX, double sizeY, bool inside, double value);
    QString ellipseShell(const QString &matrix, double x, double y, double sizeX, double sizeY, double shell,
                         bool inside, double value);
    // the beam-stop box masked in an existing mask (the mask kept otherwise); beamstop empty: as set
    QString addBeamstop(const QString &mask, const QList<int> &beamstop, const QString &shape);
    // the mask as a file; file empty: <input folder>/<mask>.mask, as the interface saves it
    QString saveMask(const QString &mask, const QString &file);
    QStringList masks() const;
    // the masks of auto mode: "mask" and what the Masks option asks for (beam-stop and transmission masks per
    // configuration or per wavelength, or the *.mask files of the input folder)
    QStringList autoMasks();
    QStringList loadMasks(const QString &pattern);

  private:
    // a tool of "Adv Masking Options" pressed on the matrix, its fields set first
    QString tool(const QString &matrix, const QList<QPair<QDoubleSpinBox *, double>> &fields, bool inside, double value,
                 void (dan18::*press)());
};

// Sensitivity tab
class DanSensitivity : public DanPart
{
  protected:
    int tab() const override
    {
        return 3;
    }

  public:
    using DanPart::DanPart;
    // transmissionMode: empty (the number transmission), "measured" (the run against emptyBeam through the mask) or
    // "formula" (T(lambda)), as the Transmission button; errorRange: (min, max) relative error in % of the pixels used,
    // empty: as set; errorMatrix: -1 as set, 0/1; maskedValue: NaN as set
    QString newSensitivity(const QString &name, const QString &run, const QString &emptyBeam,
                           const QString &darkCurrent, double transmission, const QString &mask,
                           const QString &transmissionMode = QString(), const QList<double> &errorRange = {},
                           int errorMatrix = -1, double maskedValue = qQNaN());
    // the sensitivity as a file; file empty: <input folder>/<sens>.sens, as the interface saves it
    QString saveSensitivity(const QString &name, const QString &file);
    QString loadSensitivity(const QString &name, const QString &file);
    QStringList sensitivities() const;
    // the sensitivity of auto mode: the sensitivity run, the *.sens file(s) of the input folder, or flat
    QString autoSensitivity();
    QStringList loadSensitivities(const QString &pattern);
    // the transmission of a run against an empty beam through a mask; dTr its error; false when none
    bool transmission(const QString &run, const QString &emptyBeam, const QString &mask, double &tr, double &dTr);

  private:
    // the masked pixels of a mask (0 when there is none)
    int maskedPixels(const QString &mask) const;
};

// Data Processing tab: the table of configurations, script tables, the processing buttons
class DanProcessing : public DanPart
{
  protected:
    int tab() const override
    {
        return 4;
    }

  public:
    using DanPart::DanPart;
    DanProcessingOptions *options = nullptr; // Dan.Processing.Options
    bool setConfigurations(int number);
    int configurations() const;
    // configuration 1..n; an empty argument (transmission < 0) leaves the field as it is
    bool setConfiguration(int index, const QString &emptyBeam, const QString &darkCurrent, const QString &calibrant,
                          const QString &calibrantEmptyBeam, const QString &calibrantDarkCurrent, const QString &center,
                          const QString &mask, const QString &sens, const QString &transmissionMask,
                          const QString &cellEmptyBeam, int transmission);
    QVariantMap configuration(int index);
    bool calculate();
    QString newScriptTable(const QString &name, const QStringList &runs);
    bool calculateTransmissions(const QString &scriptTable);
    QStringList process(const QString &scriptTable, const QString &kind, const QString &output);
    // the table of configurations from the runs of the input folder (auto mode); configuration(i) of each
    QVariantList autoConfigurations();
    // per wavelength whose configurations all measure their own transmissions: a "Scale" column normalizing every
    // row to the transmission of the same sample at the longest distance; the rows scaled
    int autoScale(const QString &scriptTable);
    // a run's beam center (1-based pixels) through a mask and sensitivity, by the center method of Options (H.F.
    // from the mask's beam-stop center, symmetry, or the header); false and lastError() when it cannot
    bool runCenter(const QString &run, const QString &mask, const QString &sens, double &x, double &y);
    // the shift that moves the run's center onto (xCenter, yCenter): HShift = xCenter - x, VShift = yCenter - y
    bool centerOffset(const QString &run, double xCenter, double yCenter, const QString &mask, const QString &sens,
                      double &hShift, double &vShift);
    // the "HShift" and "VShift" columns of a script table (made, or written again): each sample run's center moved
    // onto its configuration's (the empty beam's: X-center, Y-center of the row); mask, sens empty: as the
    // configuration's center was found (the row's, or the active ones with "Use Current Mask and Sensitivity");
    // the rows written, -1 on an error
    int addCenterOffsets(const QString &scriptTable, const QString &mask, const QString &sens);
    // auto mode up to the script table, ready for processing (qtikws DAN-BlackBox steps 3-7): masks, sensitivity,
    // configurations, calculate(), the script table of the sample runs, transmissions, scale; its name. Processing
    // (process()) and merging (Dan.Merge.join()) are calls of their own
    QString autoConfig(const QString &scriptTable);

  private:
    bool selectScriptTable(const QString &name);
};

// Data Processing tab, its options (Dan.Processing.Options)
class DanProcessingOptions : public DanPart
{
  protected:
    int tab() const override
    {
        return 4;
    }

  public:
    using DanPart::DanPart;
    // the I[Q] points removed at the beginning and the end of each curve
    bool setRemovePoints(int first, int last);
    QList<int> removePoints() const;
};

// Merge tab
class DanMerge : public DanPart
{
  protected:
    int tab() const override
    {
        return 5;
    }

  public:
    using DanPart::DanPart;
    // the Merge tab's Join page with the template as the project table, then Merge (output "project") or Merge ASCII
    // ("files"); smart: the curves scaled to each other in their overlap ("Smart"); the merged tables (or files)
    QStringList join(const QString &templateTable, int overlap, int removeFirst, int removeLast, const QString &output,
                     bool smart);
    // the Rebin page: points < 1 and logStep < 0 keep the page's values; manual: the Q range qMin..qMax, else the
    // data's
    QStringList rebin(const QString &templateTable, int points, bool manual, double qMin, double qMax, int logStep,
                      int removeFirst, int removeLast, const QString &output);
    // any tables joined into one curve, through the tab's own merging table (one row): its name
    QString joinTables(const QString &name, const QStringList &tables, int overlap);
    // where two curves overlap: Q range, number of points of the second inside it, mean ratio second/first
    bool overlap(const QString &first, const QString &second, double &qMin, double &qMax, int &points, double &ratio);

  private:
    QStringList mergeTemplate(const QString &templateTable, int removeFirst, int removeLast, const QString &output);
};

#endif
