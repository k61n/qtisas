gw = newGraph("FitResults", 2, 2, 1)
Fittable.plotResults("FitResults")
scriptPrint("R10: %d %d %d" % (gw.numLayers(), gw.layer(1).numCurves(), gw.layer(2).numCurves()))
