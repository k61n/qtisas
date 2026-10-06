# QtiSAS script server

Run Python, or requests to the internal AI, in a **running** QtiSAS from a terminal, and get the output,
errors and created objects back. It's for people, scripts and test tools: nothing here is loaded by the
internal AI, whose API docs live in `qtisas/python/doc/`.

| File | What it is |
|------|------------|
| `qtisas_server.py` | the server, running inside QtiSAS |
| `qtisas_send.py` | the client: plain Python 3, no PyQt needed |

The test instance and the AI benchmarks, which use the client, are in `qtisas/tests/src/server/` and
`qtisas/tests/src/ai/benchmarks/` (see its `README.md`).

macOS and Linux only for now (Unix sockets); Windows (named pipes) is not done yet.

## Starting the server

```bash
qtisas -s            # normal window, instance name "main"; dialogs are left to you
qtisas -s=second     # a second QtiSAS needs its own name
qtisas -S=main       # no window at all (also over plain ssh): dialogs are closed automatically,
                     # startup errors go to stderr with exit code 1
```

Long forms: `--server[=NAME]`, `--server-offscreen[=NAME]`. In an already running QtiSAS, from its
Python console:

```python
import sys
sys.path.insert(0, "/path/to/qtisas/python/server")
import qtisas_server
qtisas_server.start("main")
```

The socket is `/tmp/qtisas-<uid>/<name>.sock`. Only your user can connect. A name another QtiSAS still
answers on is refused, and so is a socket folder another user owns or can use.

## Sending requests

```bash
qtisas_send.py -c 'print(tableNames())'                 # code
qtisas_send.py -f myscript.py                           # a file (or: qtisas_send.py < myscript.py)
qtisas_send.py --instance second -f myscript.py
qtisas_send.py --mode ai -c 'Plot column y of table "Data"'       # the AI writes the script, nothing runs
qtisas_send.py --mode ai-run -c 'Plot column y of table "Data"'   # the AI writes and runs it
qtisas_send.py --mode status
```

The default instance is `test` (or `$QTISAS_SERVER`); use `--instance main` for a QtiSAS started with `-s`.

- **python:** the script's `print()`/`scriptPrint()` output goes to stdout, its error (with the script's
  own file and line) to stderr. Each request starts with fresh variables; `--keep` keeps them between
  requests.
- **ai / ai-run:** the AI's lines are printed as they come. The run counts as finished once the AI has been
  idle for 1.5 s.
- A one-line summary goes to stderr: seconds, created/removed windows and `ai/` fit functions, closed
  dialogs, and for the AI the scripts run and tokens sent.
- `--json` prints the whole result instead, `--stream-json` every event as one JSON line.
- `--dialogs auto` closes dialogs (with Cancel) for this request, `--timeout S` stops waiting after S
  seconds (a running script cannot be interrupted from outside).
- Exit code: 0 ok, 1 the script or the AI run failed, 2 no server, busy, refused or timed out.
- `--mode quit` ends QtiSAS at once **without saving** — meant for the test instance.

## AI agents and settings from Python

Available as `qti.app.…` only — not as short names and not in the API docs, so that an AI script cannot
switch its own agent or turn off its own checks:

```python
qti.app.aiAgents()                          # the presets of Preferences > Ask AI
qti.app.aiAgent()                           # the selected one
qti.app.setAiAgent("Anthropic-Claude-API")  # select and configure it, as Apply does (saved at once)
qti.app.aiClearHistory()
qti.app.aiConfig()                          # dict: agent, baseUrl, model, maxTokens, docsMode, and the page's
                                            # maxUserRules, maxAutoContinue, splitByDefault, maxSplitParts,
                                            # splitRequestMerge, selfReviewEnabled, exploreEnabled, localTimeoutSec
qti.app.setAiConfig("exploreEnabled", False)   # the page's settings, with its ranges (saved when QtiSAS quits)
```

The API key is never returned. The agent's own fields change only through `setAiAgent()`.
