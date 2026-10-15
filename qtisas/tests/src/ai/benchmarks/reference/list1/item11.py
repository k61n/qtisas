from PyQt6.QtGui import QFont
gw = graphWindow("FitResults")
top = gw.layer(1)
leg = top.legend()
if leg is None:
    leg = top.newLegend()
font = QFont()
font.setPointSize(12)
font.setBold(True)
leg.setFont(font)
top.setCurveLineColor(0, colorIndex("red"))
for layer in (gw.layer(1), gw.layer(2)):
    for axis in (Graph.Axis.Bottom, Graph.Axis.Left):
        layer.setMinorTicksType(axis, Graph.Ticks.Out)
scriptPrint("R11: %d %d %s" % (gw.numLayers(), top.numCurves(), top.legend() is not None))
