# Q4 — Airport baggage handling (pthread)

## Purpose

Simulates a **bounded conveyor** (capacity **5**), one **producer** thread (2 s per item), one **consumer** thread (4 s per item), and a **monitor** thread (prints every **5** s). Loads **20** luggage items then shuts down cleanly.

## Compilation

```bash
gcc -Wall -Wextra -pthread -O2 -o baggage_handling baggage_handling.c
```

## Execution

```bash
./baggage_handling
```

## Inputs

None

## Expected output

- **`[Loader]`** lines when an ID is placed on the belt, with current **belt size** (never above 5).
- **`[Aircraft]`** lines when an ID is removed, with belt size.
- **`[Monitor]`** lines with cumulative **loaded**, **dispatched**, and **belt_size**.
- Final line:

```text
Simulation finished: total_loaded=20 total_dispatched=20
```

## Timing

Roughly **~80 seconds** total wall-clock time; output order interleaves across threads.

## Synchronization

- **`pthread_mutex_t`** protects shared buffer, counters, and flags.
- **`pthread_cond_t`** `not_empty` / `not_full` for blocking when the belt is empty or full.

## Files

| File                 | Description            |
|----------------------|------------------------|
| `baggage_handling.c` | Full source + comments |
