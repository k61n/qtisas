# QtiSAS script server

Run Python, or requests to the internal AI, in a **running** QtiSAS from a terminal, and get the output,
errors and created objects back. It's for people, scripts and test tools: nothing here is loaded by the
internal AI, whose API docs live in `qtisas/python/doc/`.

| File | What it is |
|------|------------|
| `qtisas_server.py` | the server, running inside QtiSAS |
| `qtisas_send.py` | the client: plain Python 3, no PyQt needed |
| `qtisas_mcp.py` | MCP server: QtiSAS as a tool of an external AI chat (Claude Code, Codex CLI, Claude Desktop, IDEs) |

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
qtisas_send.py --mode ai-chat -c 'Explain weighted fitting'     # conversation: reads the project, changes after a yes
qtisas_send.py --mode status
qtisas_send.py --mode stop                              # cancel the running AI request
qtisas_send.py --mode note -c 'use table "B", not "A"'  # correct the running AI request
```

The default instance is `test` (or `$QTISAS_SERVER`); use `--instance main` for a QtiSAS started with `-s`.

- **python:** the script's `print()`/`scriptPrint()` output goes to stdout, its error (with the script's
  own file and line) to stderr. Each request starts with fresh variables; `--keep` keeps them between
  requests.
- **ai-chat:** natural-language conversation, with separate chat history: it reads the project with read-only probes
  and hands a change to the script AI only after your yes.
  Uses the configured AI provider/model and requires Python as QtiSAS's global scripting language.
  Its probes only read (QtiSAS cuts each before its first write call); the reply itself is words, never run.
  QtiSAS reference docs follow the configured documentation mode: compact/full docs are selected by topic,
  BIGMODEL includes all docs, and Integrated sends topic hints to the backend instead of local docs.
  Topics are retained for follow-up questions and reset by `clear history`.
  JSON results include the answer in `reply` and the complete log in `transcript`.
- **ai / ai-run:** the AI's lines are printed as they come. The run counts as finished once the AI has been
  idle for 1.5 s.
- A one-line summary goes to stderr: seconds, created/removed windows and `ai/` fit functions, closed
  dialogs, and for the AI the scripts run and tokens sent.
- `--json` prints the whole result instead, `--stream-json` every event as one JSON line.
- `--dialogs auto` closes dialogs (with Cancel) for this request, `--timeout S` stops waiting after S
  seconds (a running script cannot be interrupted from outside).
- Exit code: 0 ok, 1 the script or the AI run failed, 2 no server, busy, refused or timed out.
- `--mode stop` cancels a running AI request, also while the server is busy with it: the model call, a LIST's
  remaining items, a split plan's remaining parts (as QtiSAS's own `stop` command); that request then ends with
  status `stopped`. A Python script that is running cannot be interrupted; an AI script stops at its 300 s limit.
- `--mode note` corrects a running AI request, as text typed into QtiSAS's input line while it runs does: every
  later model call of that request (in a LIST: of the current item) carries the correction; with a leading `!` the
  call in flight starts again with it.
  While a correction is active, the scope and deliverable checks (which read the original request) are skipped.
- `--mode quit` ends QtiSAS at once **without saving** — meant for the test instance.

### Chat

After updating from a version without server chat support, rebuild QtiSAS (including its Python bindings)
and restart it with the updated server files. Updating `qtisas_send.py` alone is not enough.

From the repository root, with QtiSAS started using `-s`:

```bash
python3 qtisas/python/server/qtisas_send.py --instance main --mode ai-chat -c 'Explain weighted fitting'
python3 qtisas/python/server/qtisas_send.py --instance main --mode ai-chat -f question.txt --json
python3 qtisas/python/server/qtisas_send.py --instance main --mode ai-chat -c 'clear history'
python3 qtisas/python/server/qtisas_send.py --instance main --mode stop
```

Chat history belongs to the running QtiSAS instance and is shared with its terminal's `/ai-chat` mode;
it is separate from `/ai` and `/ai-run` history. `clear history` clears only chat history.
Use `--mode stop` (or interactive `:stop`) to cancel an in-flight request. `--mode note` is for script-AI
corrections, not chat. Chat text such as `LIST` or “run it” does not start scripting workflows.

## The prompt

Without code, in a terminal, `qtisas_send.py` opens a prompt for your QtiSAS:

```
$ qtisas_send.py --instance main
QtiSAS main — :help for commands
py> t = newTable("T", 5, 2)
py> t.numRows()
5
py> :ai-run
ai-run> plot column 2 of table "T"
  ... (the AI's lines as they come)
ai-run> :ai-chat
ai-chat> What does chi2/dof mean?
  ... (the AI's explanation)
ai-chat> clear history
ai-chat> :quit
```

- **py** (the default): Python in the script server, variables kept from one input to the next. A bare
  expression prints its value, and a line opening a block (`if`/`for`/`def` ...) continues until an empty
  line, as in Python's own prompt.
- **ai-chat:** switch with `:ai-chat`, or start with `--mode ai-chat`, for a conversation: the chat reads the project
  (read-only probes) and hands a change to the script AI only after your yes.
  `LIST` is ordinary chat text; each input is one message. Use `:file PATH` for a multiline question.
- **ai / ai-run:** the text goes to QtiSAS's AI, which writes the script (`:ai`) or also runs it (`:ai-run`).
  A request starting with `LIST` (e.g. `LIST NOSPLIT`) continues until an empty line, so a whole numbered task
  list can be pasted.
- `:file PATH` sends a file in the current mode: a Python script under `py>`, an AI request (e.g. a LIST) under
  `ai>`/`ai-run>`, or a chat message under `ai-chat>`.
- **Ctrl+C while the AI works** gives the prompt back; the AI request goes on in QtiSAS. Meanwhile `py>` inputs
  run (between the AI's steps; a script of the AI's own that is running finishes first), `:follow` shows the
  request again from where you left it, up to its result, `:note TEXT` corrects it (`:note !TEXT` at once) and
  `:stop` cancels it. While a Python input runs,
  Ctrl+C only stops waiting: the script goes on.
  Chat supports the same Ctrl+C, `:follow`, and `:stop` behavior; `:note` applies only to script-AI requests.
- `:status`, `:instance NAME` (another QtiSAS), `:help`, `:quit` or Ctrl+D. History with the arrow keys, kept in `~/.config/qtisas/qtisas_send_history`.
- Errors carry the same hints as QtiSAS's console, e.g. "'setDecimalSeparators' is not a global function but
  a method of Table".

## QtiSAS as a tool of an external AI chat (MCP)

`qtisas_mcp.py` is an MCP server (Model Context Protocol, over stdio; plain Python 3): a chat app that supports MCP
gets tools for a running QtiSAS, and the chat's own model writes the code from the API docs.

| Tool | |
|------|---|
| `run_python(code)` | Python in QtiSAS, variables kept between calls; output, error (with the console's hints), created/removed objects |
| `ask_qtisas_ai(text, run, wait)` | hands a request or a LIST to QtiSAS's own AI; its transcript and summary — or, with `wait=false`, starts it and returns at once |
| `qtisas_ai_progress(from_line, wait_seconds)` | the new lines of a request started with `wait=false`, and its result once finished |
| `list_objects()` | tables, matrices, graphs, 3D plots, notes, `ai/` fit functions |
| `show_graph(name)` | a 2D graph or 3D plot as a PNG image in the chat (at most 1200 px) |
| `get_file(path)` | a file from QtiSAS's computer: an image as an image, a text file as text, a small binary as a blob (larger: copy it, e.g. `scp`) |
| `status()`, `stop()` | the server's state; cancel a running AI request |
| `correct_qtisas_ai(text)` | correct a running `ask_qtisas_ai` request (`!text`: restart its current model call) |

The API docs (`*-python-api.md`, `.pyi`) are offered as resources `qtisas-doc://...`.

**Coach tools** (`qtisas_coach.py`; offered only from the source tree, where `tests/src/ai/benchmarks` is): an IDE AI
coaching QtiSAS's AI runs the benchmark lists through the same harness as `qtisas_benchmark.py`, in test instances.

| Tool | |
|------|---|
| `qtisas_run(benchmark, agent, instance, docs, bigmodel, markers, label, reference)` | starts a list (`"list2"`, `"ascii1d"`) in a fresh test instance (default `coach`); returns a run id at once |
| `qtisas_wait(run_id, timeout)` | waits (at most 1500 s, below the clients' limit): the summary when done, else `timeout` — call again |
| `qtisas_summary(run_id or report)` | score, calls, run identity, and each lost item's dossier: signature (seen N× before), doc topics, errors, blocks, why it stopped, last script |
| `qtisas_failures(text, item, since)` | failure signatures of all runs (`failures.jsonl`) with counts, items, models, dates |
| `qtisas_stats(labels)` | runs compared by label prefix: score, time and calls, tokens, failed items, warnings |
| `qtisas_probe(code, instance)` | Python on a side test instance (default `probe`, started if needed) — to check an API call before documenting it |
| `qtisas_instance(action, name)` | start, stop or show a test instance |
| `qtisas_environment()` | the build, the app's doc copy vs the source docs, the local AI backends and llama-server, the runs going on |

**Supervising QtiSAS's AI.** For a long or complicated request, the chat can hand it to QtiSAS's AI with
`wait=false`, watch it with `qtisas_ai_progress` and step in with `correct_qtisas_ai` (or `stop`) when it goes wrong —
e.g. in Claude Code: "Give this LIST to QtiSAS's AI, watch it, and correct it if a step goes wrong".

```bash
claude mcp add qtisas -- python3 /path/to/qtisas_mcp.py --instance main                      # Claude Code
claude mcp add qtisas -- ssh computer1 python3 /path/to/qtisas_mcp.py --instance main        # QtiSAS on computer1
codex mcp add qtisas -- python3 /path/to/qtisas_mcp.py --instance main                       # Codex CLI
```

Claude Desktop (`claude_desktop_config.json`): `{"mcpServers": {"qtisas": {"command": "python3", "args":
["/path/to/qtisas_mcp.py", "--instance", "main"]}}}`.

- It talks to the script server of the named instance (default `main`): start QtiSAS with `-s` (a window to watch) or
  `-S` (offscreen); `--start-offscreen` starts it with `-S` when none answers, `--qtisas PATH` chooses which QtiSAS.
- Dialogs follow the server: with `-S` they are closed automatically, with `-s` they are left to you (a dialog a script
  opens waits for you; your own dialogs, e.g. Preferences, stay open while the chat's request runs).
- In an installed QtiSAS the file is `qtisas.app/Contents/Resources/python/server/qtisas_mcp.py`.
- Over the network, use SSH as above: the script server only listens on a local socket, and SSH does the login and
  encryption. Do not expose it otherwise: the tools run any code as your user.
- Nothing is saved by itself: the chat saves with `saveProjectAs(...)` when asked.

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
                                            # splitRequestMerge, selfReviewEnabled, exploreEnabled, requestTimeoutSec
qti.app.setAiConfig("exploreEnabled", False)   # the page's settings, with its ranges (saved when QtiSAS quits)
```

The API key is never returned. The agent's own fields change only through `setAiAgent()`.
