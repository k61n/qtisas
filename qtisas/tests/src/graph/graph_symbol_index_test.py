"""symbolIndex(name): the shape's style for setCurveSymbolStyle(); the common names "square", "rect", "box" (rectangle),
"star" (star 1), "plus" (cross), "x" (diagonal cross) and "none" (no symbol) too; another name is a ValueError (it
returned -1, "no symbol": a curve styled with it lost its symbols without a word).
"""

assert symbolIndex("no symbol") == symbolIndex("none") == -1
assert symbolIndex("rectangle") == symbolIndex("Square") == symbolIndex("rect") == symbolIndex(" box ") == 1
assert symbolIndex("circle") == symbolIndex("ellipse") == 0
assert symbolIndex("star") == symbolIndex("star 1") == 12
assert symbolIndex("plus") == symbolIndex("cross") == 8
assert symbolIndex("x") == symbolIndex("diagonal cross") == 9
try:
    symbolIndex("squircle")
    raise AssertionError("no ValueError")
except ValueError as e:
    assert "squircle" in str(e) and "rectangle" in str(e), str(e)
