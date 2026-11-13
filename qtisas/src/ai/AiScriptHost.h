/******************************************************************************
Project: QtiSAS
License: GNU GPL Version 3 (see LICENSE)
Copyright (C) by the authors:
    2026 Vitaliy Pipich <v.pipich@gmail.com>
Description: Abstract interface between the AI orchestration logic (currently in
             ApplicationWindow::askAi()'s responseReady handler) and whatever
             environment actually runs the scripts it generates. Depends only on
             QtCore — no Widgets, no Table/Graph/Note, no ApplicationWindow —
             so the orchestration logic can eventually move into AiAssistant (or
             a dedicated orchestrator class) and be extracted into a standalone
             library, with ApplicationWindow (or anything else) implementing
             this interface as the only QtiSAS-specific dependency.
 ******************************************************************************/

#ifndef AISCRIPTHOST_H
#define AISCRIPTHOST_H

#include <QList>
#include <QString>
#include <QStringList>

// Result of running one candidate script (an action script or an EXPLORE script). No separate
// "did it even compile" flag: a SyntaxError is just another exception, captured in `error`
// exactly like a RuntimeError would be — this deliberately mirrors how the interpreter itself
// treats it (confirmed live: a `return` outside a function surfaced as an ordinary "Python
// script Error" via the same signal a RuntimeError would use, not a special separate path).
// Callers that need to decide "is this even worth attempting, or is it plain English instead of
// code" (the action-script branch's "not runnable" case) must do that check THEMSELVES before
// calling runScript() — it is not this interface's job to silently swallow that distinction.
struct AiScriptResult
{
    QString output; // everything printed during execution (scriptPrint()/print()), in order —
                    // includes the exception's own text if one was raised, same as before
    QString error;  // non-empty exception text (SyntaxError or RuntimeError alike) if the script
                    // raised or failed to parse; empty on success
};

class AiScriptHost
{
  public:
    virtual ~AiScriptHost() = default;

    // Runs `code` under the given script name (e.g. "<ai>" for an action script, "<ai-explore>"
    // for an EXPLORE script — matches what currently shows up in traceback location text like
    // "at <ai>:4"). Always attempts to run — no internal pre-check of any kind.
    virtual AiScriptResult runScript(const QString &code, const QString &scriptName) = 0;

    // Prints a message to wherever the user is watching AI output (console panel, note, ...).
    virtual void print(const QString &message) = 0;

    // Checks whether `code` is even syntactically valid, WITHOUT running it — used by the
    // action-script branch to decide "not runnable" (e.g. the model returned plain English
    // instead of code, or a malformed markdown fence remnant leaked into the extracted code) vs.
    // "worth sending to self-review / running". Deliberately a separate method from runScript()
    // rather than an implementation detail of it (see AiScriptResult's comment) — the underlying
    // implementation (Py_CompileString/PyGILState on the QtiSAS side) is Python-C-API-specific
    // and does not belong bleeding into the orchestration logic that calls this interface.
    // errorOut, if non-null and compilation fails, is set to the actual SyntaxError text (e.g.
    // "invalid syntax (<ai>, line 7)") — used to feed the SAME retry machinery a runtime error
    // already goes through, instead of dead-ending with no recovery (confirmed live: a stray
    // markdown-fence remnant left in the extracted code produced "not runnable" with no further
    // AI response ever sent — a silent, permanent stall).
    virtual bool canCompile(const QString &code, QString *errorOut = nullptr) const = 0;

    // For each name in `names`, what the host currently holds under it: an entry per name, each a
    // (possibly empty) list of kind labels — "window", "compiled function", "fit function".
    // An empty list means nothing of any kind is known by that name.
    //
    // Answered from the host's own live state, with NO script execution. That is the whole point:
    // the same facts used to cost a generated read-only script plus a full round-trip (measured at
    // 24.2s on one item), and could not be obtained at all in generate-only mode, where executing
    // anything violates what the user asked for. A host that cannot answer may return an empty
    // list per name; callers treat that as "unknown", never as "does not exist".
    virtual QList<QStringList> entityKinds(const QStringList &names) = 0;

    // What the host holds right now, as "kind:name" ("table:Squares", "note:Report", "function:ai/RefLine").
    // Compared before and after a script, so a retry can be told what a failed attempt already created and left
    // behind. Empty when the host cannot tell.
    virtual QStringList liveObjects()
    {
        return {};
    }

    // The language a table column formula is evaluated in ("Python", "muParser", ...), or empty when unknown. A repair
    // that rewrites formula syntax must know it: ^ is the power in muParser and XOR in Python.
    virtual QString formulaLanguage() const
    {
        return {};
    }

    // The shape of the table `name` — "21 rows; columns: 1 \"x\" X, 2 \"y\" Y, 3 \"dy\" yErr" — or empty when there is
    // no such table or the host cannot tell. Read from live state, like entityKinds(): it answers the most frequent
    // EXPLORE probe ("the column names of the table ...") without a round-trip.
    virtual QString tableShape(const QString &name)
    {
        Q_UNUSED(name)
        return {};
    }

    // What the graph window `name` shows — "layer 1: curves \"Table2_I\" (x \"Table2_Q\"), ...; x log10 0.004..0.3,
    // y log10 0.1..200" — or empty when there is no such graph or the host cannot tell. Read from live state, like
    // tableShape(): the chat's project state names a graph's curves and scales without a probe.
    virtual QString graphShape(const QString &name)
    {
        Q_UNUSED(name)
        return {};
    }

    // What the note `name` holds, briefly — "tab 1 (120 characters): \"Beamtime 2026-11, ...\"" for its first tabs — or
    // empty when there is no such note or the host cannot tell. The chat's project state shows what a note is about.
    virtual QString noteShape(const QString &name)
    {
        Q_UNUSED(name)
        return {};
    }

    // The fit-function library, briefly: how many functions and how many in each folder ("312 fit functions, by folder:
    // top level 120, sasviewmodels/ 98, ai/ 68, ...") — or empty when the host cannot tell. The chat's project state:
    // confirmed (2026-11-13, the user's QtiSAS), shown only the "ai/" functions, the chat insisted that no
    // sasviewmodels/ function existed, even after its own probe had listed them.
    virtual QString functionLibrary()
    {
        return {};
    }

    // True while an AI script runs (it can take long and process events meanwhile, e.g. a DAN reduction): the AI is
    // not idle then, although no model call is in flight.
    virtual bool scriptRunning() const
    {
        return false;
    }

    // The state the host's modules keep between scripts, which a later script continues instead of starting anew — one
    // line per module ("DAN-SANS session (Dan): instrument ..., masks: ...") — or empty when no module holds any.
    virtual QStringList moduleStates()
    {
        return {};
    }

    // Closes and deletes the windows among `objects` ("kind:name", as liveObjects() lists them: tables, matrices,
    // notes, graphs, 3D plots — never functions) and returns the ones it closed. Used to give a retry the state the
    // failed attempt started from; a host that cannot close returns an empty list and the retry is told instead.
    virtual QStringList closeObjects(const QStringList &objects)
    {
        Q_UNUSED(objects)
        return {};
    }

    // The global names (the host's functions and modules, Python's builtins) that a function defined in `code` reads
    // where it may not have assigned them yet: at a script's top level those lines read the global, inside a function
    // they raise UnboundLocalError ("table = table(...)"). Empty when there are none or the host cannot tell.
    virtual QStringList unboundGlobalReads(const QString &code) const
    {
        Q_UNUSED(code)
        return {};
    }
};

#endif
