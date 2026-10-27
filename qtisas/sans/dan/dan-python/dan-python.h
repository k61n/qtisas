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

#include <QStringList>
#include <QVariantMap>

class dan18;

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
    QStringList headerItems() const;
    QString newHeaderTable(const QString &name, const QStringList &items, const QString &runs);
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
    QString newMask(const QString &name, const QList<int> &edge, const QList<int> &beamstop, const QString &shape,
                    const QString &polygons, bool transmission);
    QString loadMask(const QString &name, const QString &file);
    QStringList masks() const;
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
    QString newSensitivity(const QString &name, const QString &run, const QString &emptyBeam,
                           const QString &darkCurrent, double transmission, const QString &mask);
    QString loadSensitivity(const QString &name, const QString &file);
    QStringList sensitivities() const;
    // the transmission of a run against an empty beam through a mask; dTr its error; false when none
    bool transmission(const QString &run, const QString &emptyBeam, const QString &mask, double &tr, double &dTr);
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
