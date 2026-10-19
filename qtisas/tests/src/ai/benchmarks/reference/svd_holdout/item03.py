s = Svd
s.reset()
s.setSolvent(-5.61e9, 6.335e10)
s.setComponents([("p", 2.0e10, 2.9e10), ("n", 3.6e10, 4.2e10)])
scriptPrint("R3:", 3, s.rank([0, 0.5, 0.5]))
