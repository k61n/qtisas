s = Svd
s.reset()
s.setSolvent(-5.61e9, 6.335e10)
s.setComponents([("a", 1.48e10, 2.374e10), ("b", 2.11e10, 3.199e10)])
s.setContrasts([("mix0", 0), ("mix100", 1)])
try:
    s.decompose("svd2mix")
except ValueError as e:
    print(e)  # ... at least 3 H/D contrasts are needed ...
    scriptPrint("R6:", 3)
