s = Svd
s.reset()
s.setSolvent(-5.61e9, 6.335e10)
s.setComponents([("a", 1.48e10, 2.374e10), ("b", 2.11e10, 3.199e10), ("c", 4.0e10, 4.0e10)])
scriptPrint("R10:", 3 * 4 // 2, s.rank([0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875, 1]))
