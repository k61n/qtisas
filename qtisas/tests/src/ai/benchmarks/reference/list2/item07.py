g3 = plot3D(table("Metrics"), "z")
setWindowName(g3, "Metrics3D")
g3.setPlotStyle(Qti3D.FILLEDMESH)
scriptPrint("R7: %s" % existPlot3D("Metrics3D"))
