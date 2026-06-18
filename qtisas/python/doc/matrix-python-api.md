# QtiSAS Matrix Python API

Python scripting interface for the `Matrix` class.
Obtain a matrix via `matrix("name")`, `newMatrix()`, or `currentMatrix()`.

> **Note:** All SIP-wrapped methods accept positional arguments. Named optional parameters also accept keyword syntax; required parameters are always positional.

> **IMPORTANT:** Matrix cell methods use **(row, col)** order — ROW first — which is the **opposite** of Table which uses (col, row). `m.cell(2, 3)` → row 2, col 3.

---

## Quick Start

```python
# create a 4×5 matrix
m = newMatrix("MyMatrix", 4, 5)
m.setCoordinates(0.0, 1.0, 0.0, 1.0)
m.setFormula("sin(x) * cos(y)")
m.calculate()
```

```python
# access an existing matrix
m = matrix("RawData")
print(m.cell(1, 1))     # row 1, col 1
print(m.numRows(), m.numCols())
```

---

## Getting a Matrix

| Call | Returns | Notes |
|------|---------|-------|
| `matrix("name")` | `Matrix` | look up existing matrix by object name |
| `newMatrix()` | `Matrix` | blank matrix (32×32) |
| `newMatrix("name", rows, cols)` | `Matrix` | blank matrix with given size |
| `currentMatrix()` | `Matrix` | currently active matrix |

---

## Size

| Method | Returns | Notes |
|--------|---------|-------|
| `numRows()` | `int` | number of rows |
| `numCols()` | `int` | number of columns |
| `setNumRows(n)` | — | resize rows |
| `setNumCols(n)` | — | resize columns |
| `setDimensions(rows, cols)` | — | resize both at once |

---

## Cell Access

> **Row first:** Matrix uses `(row, col)` order — the **opposite** of `Table.cell(col, row)`.
> Both row and column indices are **1-based**.

| Method | Returns | Notes |
|--------|---------|-------|
| `text(row, col)` | `str` | raw cell text |
| `cell(row, col)` | `float` | numeric value |
| `setText(row, col, value)` | — | write string to cell |
| `setCell(row, col, value)` | — | write float to cell |

```python
m = matrix("MyMatrix")
m.setCell(1, 1, 3.14)
print(m.cell(1, 1))
```

---

## Coordinates

Maps row/column indices to physical x/y values used in formulas.

| Method | Returns | Notes |
|--------|---------|-------|
| `xStart()` | `float` | left x boundary |
| `xEnd()` | `float` | right x boundary |
| `yStart()` | `float` | top y boundary |
| `yEnd()` | `float` | bottom y boundary |
| `dx()` | `float` | x step between columns |
| `dy()` | `float` | y step between rows |
| `setCoordinates(xStart, xEnd, yStart, yEnd)` | — | set all boundaries at once |

---

## Formula

```python
m.setFormula("sin(x) * cos(y)")   # set formula
m.calculate()                      # evaluate over all cells
```

`calculate` signature (all params named — keyword syntax works for optional args):

```python
m.calculate(
    0,      # 1 startRow (0-based)
    -1,     # 2 endRow   (0-based, -1 = last)
    0,      # 3 startCol (0-based)
    -1,     # 4 endCol   (0-based, -1 = last)
    False,  # 5 forceMuParser
)
```

**Formula variables** (identical in MuParser and Python):

| Variable | Value |
|----------|-------|
| `i` / `row` | current row, **1-based** |
| `j` / `col` | current column, **1-based** |
| `x` | x coordinate at current column (`xStart + (j-1)*dx`) |
| `y` | y coordinate at current row (`yStart + (i-1)*dy`) |

Use `pow(x, n)` for powers — `^` only works in MuParser; `ln(x)` is the natural log in both.

```python
m.setFormula("pow(x, 2) + pow(y, 2)")
m.calculate()

m.setFormula("ln(x + 1)")
m.calculate()
```

| Method | Notes |
|--------|-------|
| `setFormula(expr)` | store formula string |
| `calculate(startRow=0, endRow=-1, startCol=0, endCol=-1, forceMuParser=False)` | evaluate; range indices 0-based |
| `setNumericPrecision(prec)` | significant digits for display |

---

## Mathematical Operations

| Method | Notes |
|--------|-------|
| `transpose()` | transpose rows ↔ columns |
| `invert()` | matrix inversion (square matrix) |
| `determinant()` | → `float` |
| `integrate()` | → `float` — numerical integration |
| `flipVertically()` | flip upside down |
| `flipHorizontally()` | flip left–right |
| `rotate90(clockwise=True)` | rotate 90°; `False` = counter-clockwise |
| `smooth()` | apply smoothing filter |
| `resample(rows, cols, method)` | resize with interpolation; `method`: `Matrix.ResamplingMethod.Bilinear` or `.Bicubic` |

```python
m = matrix("Data")
m.transpose()
m.resample(64, 64, Matrix.ResamplingMethod.Bicubic)
```

---

## View and Color Map

### View Type Constants

Access via `Matrix.ViewType.<name>`:

| Constant | Meaning |
|----------|---------|
| `Matrix.ViewType.TableView` | show as spreadsheet |
| `Matrix.ViewType.ImageView` | show as color image |

### Header View Type Constants

Access via `Matrix.HeaderViewType.<name>`:

| Constant | Meaning |
|----------|---------|
| `Matrix.HeaderViewType.ColumnRow` | column/row numbers |
| `Matrix.HeaderViewType.XY` | x/y coordinate values |

### Color Map Methods

| Method | Notes |
|--------|-------|
| `setGrayScale()` | gray colormap |
| `setRainbowColorMap()` | rainbow colormap |
| `setDefaultColorMap()` | reset to application default |
| `colorMap()` | → `LinearColorMap` — current colormap object |
| `setColorMap(lc)` | apply a `LinearColorMap` |
| `resetView()` | refresh the display |

```python
m = matrix("Intensity")
m.setViewType(Matrix.ViewType.ImageView)
m.setHeaderViewType(Matrix.HeaderViewType.XY)
m.setGrayScale()
```

---

## Import / Export

### importASCII

All arguments **positional only** (parameters are unnamed in the SIP binding):

```python
m.importASCII(
    "/path/to/data.dat",  # 1  file
    "\t",                 # 2  sep
    0,                    # 3  ignoredLines: header lines to skip
    False,                # 4  stripSpaces
    False,                # 5  simplifySpaces: collapse multiple spaces
    "#",                  # 6  commentString
    2,                    # 7  mode: 0=NewColumns, 1=NewRows, 2=Overwrite
    # args 8-10 (locale, endLineChar, maxRows) use defaults
)
```

### exportASCII

```python
ok = m.exportASCII(
    "/path/to/output.dat",  # 1 file
    "\t",                   # 2 sep
    False,                  # 3 exportSelection
)
```

### Image Export

```python
m.exportRasterImage(
    "/path/to/image.png",  # 1 file
    100,                   # 2 quality (0–100)
    0,                     # 3 dpi (0 = screen)
    0,                     # 4 compression
)

m.export("/path/to/image.pdf")       # vector or raster based on extension
m.exportVector("/path/to/image.svg") # vector only
```

---

## Examples

### Create and fill a matrix

```python
m = newMatrix("Grid", 50, 50)
m.setCoordinates(-1.0, 1.0, -1.0, 1.0)
m.setFormula("exp(-(pow(x, 2) + pow(y, 2)) / 0.1)")
m.calculate()
m.setViewType(Matrix.ViewType.ImageView)
m.setRainbowColorMap()
```

### Fill from Python values

```python
import math

m = newMatrix("Gauss", 20, 20)
m.setCoordinates(0.0, 1.0, 0.0, 1.0)

for row in range(1, 21):
    for col in range(1, 21):
        x = (col - 1) / 19.0
        y = (row - 1) / 19.0
        m.setCell(row, col, math.exp(-(x**2 + y**2)))
```

### Import ASCII and display as image

```python
m = newMatrix("Raw")
m.importASCII("/data/detector.dat", " ", 0, False, True, "#", 2)
m.setViewType(Matrix.ViewType.ImageView)
m.setGrayScale()
m.resetView()
```

### Partial recalculation

```python
m = matrix("Data")
m.setFormula("pow(i, 2) + pow(j, 2)")

# recalculate only the top-left 10×10 block (0-based indices)
m.calculate(0, 9, 0, 9)
```

### Matrix operations

```python
m = matrix("A")
det = m.determinant()
print(f"det = {det:.6f}")

m.invert()              # A → A⁻¹ in place
m.transpose()
m.flipVertically()
m.rotate90(True)        # 90° clockwise
m.rotate90(False)       # 90° counter-clockwise
```

### Resample to higher resolution

```python
m = matrix("LowRes")    # e.g. 32×32
m.resample(256, 256, Matrix.ResamplingMethod.Bicubic)
m.setViewType(Matrix.ViewType.ImageView)
```
