s = Svd
s.reset()
s.setSolvent(-5.61e9, 6.335e10)
scriptPrint("R2:", "%.3f" % s.matchPoint(3.6e10, 4.2e10))
