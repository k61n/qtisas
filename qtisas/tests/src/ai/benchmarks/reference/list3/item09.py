data = table("DecayData")
fit = table("fitCurve-ai/RefDecay")
g = newGraph("DecayPlot", 2, 2, 1)
top, bottom = g.layer(1), g.layer(2)
top.addCurve(data, "y", Graph.CurveType.Scatter)
top.insertCurve(fit, "x", "y", Graph.CurveType.Line)
bottom.insertCurve(fit, "x", "residues", Graph.CurveType.Scatter)
largest = max(abs(v) for v in fit.colData(5) if v is not None)  # column 5: the residues
scriptPrint("R9: %d %d %d %s" % (g.numLayers(), top.numCurves(), bottom.numCurves(), largest < 1e-4))
