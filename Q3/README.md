# Q3 — `vibration` Python C extension

## Purpose

Fast statistics on Python **list** or **tuple** of **float** values: peak-to-peak, RMS, sample standard deviation, count above threshold, and summary dictionary.

## Dependencies

- Python 3.x with **development headers** (`python3-dev` or equivalent)
- **setuptools** (usually via `pip`)

## Compilation / installation

```bash
python3 setup.py build_ext --inplace
```

This drops a shared module next to `setup.py`


If you add package metadata later:

```bash
python3 -m pip install -e .
```

## Execution

```bash
python3 test_vibration.py
```

Or interactively:

```python
import vibration as v
print(v.peak_to_peak([1.0, -2.0, 3.0]))
```

## Inputs (API)

All functions take a **list or tuple** of floats as the first argument (except `above_threshold`, which also takes `threshold: float`).

| Function                   | Returns                                       |
|----------------------------|-----------------------------------------------|
| `peak_to_peak(data)`       | `max - min` (0.0 if empty)                    |
| `rms(data)`                | √(mean of squares)                            |
| `std_dev(data)`            | sample standard deviation; `0.0` when `n < 2` |
| `above_threshold(data, t)` | count of values **strictly greater than** `t` |
| `summary(data)`            | `dict`: `count`, `mean`, `min`, `max`         |

Invalid types raise **`TypeError`**.

## Expected output (`test_vibration.py`)

The test script prints:

- Main sample list and results for all five functions
- Edge cases: empty list, invalid type (caught), single-element tuple
- A cross-check of `std_dev` against pure Python `math`

Example opening lines:

```text
Sample data: [0.1, -2.5, 3.3, 3.3, 0.0, 10.0]
peak_to_peak: 12.5
rms: 4.619523784980439
std_dev: 4.345879274285776
above_threshold 0.0: 4
summary: {'count': 6, 'mean': 2.3666666666666667, 'min': -2.5, 'max': 10.0}
```

## Files

| File                | Description                         |
|---------------------|-------------------------------------|
| `vibrationmodule.c` | Extension implementation + comments |
| `setup.py`          | `setuptools.Extension` build        |
| `test_vibration.py` | Demonstration and edge-case tests   |
