#!/usr/bin/env python3
"""Demonstrate the vibration extension: normal use and edge cases."""

import math

import vibration as v


def main() -> None:
    data = [0.1, -2.5, 3.3, 3.3, 0.0, 10.0]
    print("Sample data:", data)
    print("peak_to_peak:", v.peak_to_peak(data))
    print("rms:", v.rms(data))
    print("std_dev:", v.std_dev(data))
    print("above_threshold 0.0:", v.above_threshold(data, 0.0))
    print("summary:", v.summary(data))

    print("\n--- Edge: empty list ---")
    print("peak_to_peak([]):", v.peak_to_peak([]))
    print("rms([]):", v.rms([]))
    print("std_dev([]):", v.std_dev([]))
    print("above_threshold([], 0):", v.above_threshold([], 0.0))
    print("summary([]):", v.summary([]))

    print("\n--- Edge: invalid type ---")
    try:
        v.peak_to_peak("not a list")  # type: ignore[arg-type]
    except TypeError as e:
        print("TypeError:", e)

    print("\n--- Edge: tuple and single element ---")
    t = (1.0,)
    print("std_dev((1.0,)):", v.std_dev(t))
    print("summary((1.0,)):", v.summary(t))

    print("\n--- Consistency check: std_dev vs math ---")
    xs = [2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0]
    m = sum(xs) / len(xs)
    py_std = math.sqrt(sum((x - m) ** 2 for x in xs) / (len(xs) - 1))
    print("Python ref:", py_std, "extension:", v.std_dev(xs))


if __name__ == "__main__":
    main()
