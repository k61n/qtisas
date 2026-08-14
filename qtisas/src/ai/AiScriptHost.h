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

#include <QString>

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
};

#endif
