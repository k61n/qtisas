try:
    Dan.Options.selectInstrument("KWS9")
    found = "yes"
except ValueError:
    found = "no"
scriptPrint("R15:", found, Dan.Options.instrument())
