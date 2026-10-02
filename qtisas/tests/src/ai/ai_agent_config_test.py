"""The AI agents and the "Ask AI" page from Python: aiAgents(), aiAgent(), setAiAgent(name), aiClearHistory(),
aiConfig() (the agent's fields and the page's settings) and setAiConfig(key, value) (the settings, with the page's
ranges; the agent's fields only through setAiAgent()). The user's settings file is restored afterwards.
"""

import os
import sys

import qti

sys.dont_write_bytecode = True  # no __pycache__ in the source tree
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from qtisas_testing import user_settings_kept  # noqa: E402

app = qti.app
KEYS = {"agent", "baseUrl", "model", "maxTokens", "docsMode", "maxUserRules", "maxAutoContinue", "splitByDefault",
        "maxSplitParts", "splitRequestMerge", "selfReviewEnabled", "exploreEnabled", "localTimeoutSec"}


def refused(call):
    try:
        call()
    except ValueError as e:
        return str(e)
    return ""


with user_settings_kept():
    config = app.aiConfig()
    assert set(config) == KEYS, set(config) ^ KEYS
    assert "apiKey" not in config and config["docsMode"] in ("compact", "full", "integrated"), config

    # the page's settings: set, read back, restored
    explore = config["exploreEnabled"]
    app.setAiConfig("exploreEnabled", not explore)
    app.setAiConfig("maxSplitParts", 5)
    assert (app.aiConfig()["exploreEnabled"], app.aiConfig()["maxSplitParts"]) == (not explore, 5), app.aiConfig()
    app.setAiConfig("exploreEnabled", explore)
    app.setAiConfig("maxSplitParts", config["maxSplitParts"])
    assert app.aiConfig() == config, (app.aiConfig(), config)

    # wrong values and keys
    assert "2..30" in refused(lambda: app.setAiConfig("maxSplitParts", 1))
    assert "True or False" in refused(lambda: app.setAiConfig("exploreEnabled", 1))
    assert "integer" in refused(lambda: app.setAiConfig("maxAutoContinue", True))
    assert "setAiAgent" in refused(lambda: app.setAiConfig("model", "x"))
    assert "no setting" in refused(lambda: app.setAiConfig("nope", 1))

    # agents: the presets of the "ai" folder
    agents = app.aiAgents()
    assert agents == sorted(agents), agents
    assert "agents:" in refused(lambda: app.setAiAgent("no such agent"))
    if agents:
        assert app.setAiAgent(agents[0]) and app.aiAgent() == agents[0], app.aiAgent()
        assert app.aiConfig()["agent"] == agents[0], app.aiConfig()
    app.aiClearHistory()
