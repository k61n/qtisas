"""Plain Python client regressions; no running QtiSAS or model required."""
import contextlib
import importlib.util
import io
from pathlib import Path
import tempfile
import types
import unittest
from unittest.mock import patch


CLIENT = Path(__file__).resolve().parents[3] / "python/server/qtisas_send.py"
spec = importlib.util.spec_from_file_location("qtisas_send", CLIENT)
client = importlib.util.module_from_spec(spec)
spec.loader.exec_module(client)


class ChatClientTests(unittest.TestCase):
    def test_chat_sources_and_timeout(self):
        with tempfile.TemporaryDirectory() as folder:
            source = Path(folder) / "question.txt"
            source.write_text("Explain fitting", encoding="utf-8")
            for args in (["-c", "Explain fitting"], ["-f", str(source)], ["-f", "-"]):
                with self.subTest(args=args), patch.object(client, "request", return_value={"ok": True}) as send:
                    with patch("sys.stdin", io.StringIO("Explain fitting")), contextlib.redirect_stdout(io.StringIO()):
                        self.assertEqual(client.main(["--mode", "ai-chat", "--json", "--timeout", "10"] + args), 0)
                    message, _, wait, _ = send.call_args.args
                    self.assertEqual(message["mode"], "ai-chat")
                    self.assertEqual(message["text"], "Explain fitting")
                    self.assertNotIn("code", message)
                    self.assertEqual(wait, 40)

    def test_chat_list_is_one_message(self):
        with patch("builtins.input", return_value="LIST") as read:
            self.assertEqual(client.read_entry("ai-chat"), "LIST")
            read.assert_called_once_with("ai-chat> ")

    def test_interactive_chat_switch(self):
        messages = []

        def send(message, *args):
            messages.append(message)
            return {"ok": True}

        with tempfile.TemporaryDirectory() as folder, patch.object(client, "request", side_effect=send):
            with patch.object(client, "history_path", return_value=str(Path(folder) / "history")), patch("atexit.register"):
                with patch("builtins.input", side_effect=[":ai-chat", "Hello", ":quit"]):
                    with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                        self.assertEqual(client.interactive("test", "/tmp/unused.sock", "python", None), 0)
        self.assertEqual(messages[-1]["mode"], "ai-chat")
        self.assertEqual(messages[-1]["text"], "Hello")


class ChatServerTests(unittest.TestCase):
    def test_server_uses_chat_entry_point(self):
        try:
            from PyQt6.QtCore import QCoreApplication
        except ImportError:
            try:
                from PyQt5.QtCore import QCoreApplication
            except ImportError:
                self.skipTest("PyQt is needed for the server event loop")
        application = QCoreApplication.instance() or QCoreApplication([])
        lines = []
        reply = "AI code (running)\nAI LIST: item 1/2 failed\nR1: 42"
        app = types.SimpleNamespace(
            _aiIdle=lambda: True, _aiTranscriptSize=lambda: len(lines),
            _aiTranscript=lambda start: lines[start:])

        def chat(text):
            self.assertEqual(text, "Explain fitting")
            lines.append("▸▸▸ AI chat:\n\n" + reply)
            return True

        app._askAiChat = chat  # deliberately no _askAi: chat must never call the script path
        spec = importlib.util.spec_from_file_location("chat_server_under_test", CLIENT.with_name("qtisas_server.py"))
        server_module = importlib.util.module_from_spec(spec)
        with patch.dict("sys.modules", {"qti": types.SimpleNamespace(app=app), "qtisas_send": client}):
            spec.loader.exec_module(server_module)
        server = server_module.ScriptServer.__new__(server_module.ScriptServer)
        server.dialogs = "leave"
        emitted = []
        with patch.object(server_module, "snapshot", return_value=set()), patch.object(server_module, "AI_IDLE_SECONDS", 0):
            result = server.run({"mode": "ai-chat", "text": "Explain fitting"}, emitted.append)
        self.assertTrue(result["ok"])
        self.assertEqual(result["reply"], reply)
        self.assertEqual(result["scripts"], 0)
        self.assertEqual(result["items"], [])
        self.assertEqual(emitted, lines)
        self.assertIsNone(server.ai_run)
        self.assertFalse(server_module.summarize_chat([])["ok"])
        self.assertFalse(server_module.summarize_chat(["▸▸▸ AI: connection error: offline"])["ok"])
        self.assertTrue(server_module.summarize_chat([], allow_empty=True)["ok"])

        stopped = []
        app._aiIdle = lambda: False
        app._aiStop = lambda: stopped.append(True)
        server.busy = "ai-chat"
        self.assertFalse(server.note_ai("change the answer")["noted"])
        self.assertEqual(stopped, [])
        self.assertTrue(server.stop_ai()["stopped"])
        self.assertEqual(stopped, [True])


if __name__ == "__main__":
    result = unittest.main(argv=[__file__], exit=False).result
    assert result.wasSuccessful()
