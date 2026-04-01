# Linux Programming — Summative Project

This repository contains five question deliverables: ELF reverse-engineering (C), x86-64 assembly (NASM), a Python C extension, a multithreaded producer–consumer simulation, and a TCP client–server library reservation system.


**Requirements:** Linux x86-64, **GCC**, **Python 3** with development headers, **pthread**. Question 2 additionally requires **NASM**.

---

## Quick reference

| Question | Primary artifacts                                    | Build toolchain               |
|----------|------------------------------------------------------|------------------------|
| Q1       | `program.c`, stripped `program`, analysis report     | `gcc`, `strip`                 |
| Q2       | `temperature.asm`, `temperature_data.txt`            | `nasm`, `gcc`                   |
| Q3       | `vibrationmodule.c`, `setup.py`, `test_vibration.py` | `gcc`, `python3`, `setuptools` |
| Q4       | `baggage_handling.c`                                 | `gcc` + `-pthread`              |
| Q5       | `library_server.c`, `library_client.c`               | `gcc` + `-pthread`              |

Each subdirectory has its own **README** with compile commands and expected behavior.

---

## Q1 — ELF binary (reverse engineering)

### Compilation

```bash
cd Q1
gcc -Wall -O0 -fno-inline -o program program.c
strip program
```

Optional unstripped binary for analysis:

```bash
gcc -Wall -O0 -fno-inline -o program_unstripped program.c
```

### Execution

```bash
./program
```

### Inputs

- **None** (no command-line arguments; no stdin).

### Expected output

```text
computed_sum=60
```

The program fills a 10-element array, sums strictly positive elements, formats the result on the heap, and prints one line to **stdout**.

### Further documentation

- Analysis write-up: Project Report
- Per-folder notes: **`Q1/README.md`**

---

## Q2 — Temperature file (x86-64 assembly)

### Dependencies

Install NASM if needed:

```bash
sudo apt install nasm
```

### Compilation

```bash
cd Q2
nasm -f elf64 temperature.asm -o temperature.o
gcc -nostartfiles -no-pie temperature.o -o temperature
```

### Execution

The program reads **`temperature_data.txt` in the current working directory**.

```bash
./temperature
```

### Inputs

- **File:** `temperature_data.txt` (UTF-8 text; one logical line per reading; empty lines allowed; LF or CRLF line endings supported).
- **No** command-line arguments.

### Expected output (with the bundled sample `temperature_data.txt`)

Format (exact labels):

```text
Total readings: 6
Valid readings: 4
```

- **Total readings:** every line in the file (including empty lines; an extra trailing newline after the last line would add one more empty line).
- **Valid readings:** lines that contain at least one non-whitespace character after optional trailing `\r` (CRLF handling).

If the file cannot be opened:

```text
Error: cannot open temperature_data.txt
```

(exit status non-zero via syscall in the assembly program.)

### Further documentation

- **`Q2/README.md`**

---

## Q3 — Python C extension

### Compilation / install

```bash
cd Q3
python3 -m pip install --upgrade pip setuptools
python3 setup.py build_ext --inplace
```

This produces `vibration*.so` (name varies by Python version, here: `vibration.cpython-312-x86_64-linux-gnu.so`) **in `Q3/`**.

Alternatively:

```bash
python3 -m pip install .
```

### Execution

```bash
cd Q3
python3 test_vibration.py
```

### Inputs

- **Programmatic:** `test_vibration.py` builds sample lists/tuples and edge cases.
- Public API (from Python after `import vibration as v`):
  - `v.peak_to_peak(seq)` — list or tuple of floats
  - `v.rms(seq)`
  - `v.std_dev(seq)` — sample standard deviation
  - `v.above_threshold(seq, threshold)`
  - `v.summary(seq)` — dict with `count`, `mean`, `min`, `max`

### Expected output (representative)

```text
Sample data: [0.1, -2.5, 3.3, 3.3, 0.0, 10.0]
peak_to_peak: 12.5
rms: 4.619523784980439
std_dev: 4.345879274285776
above_threshold 0.0: 4
summary: {'count': 6, 'mean': 2.3666666666666667, 'min': -2.5, 'max': 10.0}

--- Edge: empty list ---
peak_to_peak([]): 0.0
...
--- Edge: invalid type ---
TypeError: expected list or tuple of floats
...
```

Floating-point formatting may show minimal rounding differences; statistical checks should match within normal `double` tolerance.

### Further documentation

- **`Q3/README.md`**

---

## Q4 — Baggage handling

### Compilation

```bash
cd Q4
gcc -Wall -Wextra -pthread -O2 -o baggage_handling baggage_handling.c
```

### Execution

```bash
./baggage_handling
```

### Inputs

- **None.** Parameters are fixed in source (belt capacity 5, producer delay 2 s, consumer 4 s, monitor 5 s, **20** luggage items).

### Expected output

Console lines should include:

- `[Loader] Placed luggage ID …` with current belt size
- `[Aircraft] Removed luggage ID …` with belt size
- `[Monitor] loaded=… dispatched=… belt_size=…` approximately every 5 seconds during the run

 Finale:

```text
Simulation finished: total_loaded=20 total_dispatched=20
```

Runtime is on the order of **~80 seconds** (consumer is the slower stage overall).

### Further documentation

- **`Q4/README.md`**

---

## Q5 — Library TCP server and client

### Compilation

```bash
cd Q5
gcc -Wall -Wextra -pthread -O2 -o library_server library_server.c
gcc -Wall -Wextra -O2 -o library_client library_client.c
```

### Execution

**Terminal 1 — server:**

```bash
cd Q5
./library_server
```

Listens on **TCP port 9090** on all interfaces (`INADDR_ANY`).

**Terminal 2 — client:**

```bash
cd Q5
./library_client
```

### Inputs (client)

1. **Library ID** (interactive `scanf`): one of  
   `LIB1001`, `LIB2002`, `LIB3003`, `LIB4004`, `LIB5005`
2. After successful auth, **book index** to reserve: integer **0 … 5** (matches server list order).

### Expected output (client)

- Server greeting line, then authentication result (`AUTH_OK` or `AUTH_FAIL`).
- If authenticated: book list line (`LIST:…`), then reservation result (`RESERVE_OK`, `RESERVE_FAIL`, `ERR:…`).
- On exit:  
  `Session closed. Goodbye, <library_id>`

### Expected output (server)

- `Library server listening on port 9090…`
- After auth / reserve / disconnect: `[Server] Active users …` and `[Server] Books: …` showing **available** vs **reserved**.

### Automated demo (stdin piped to client)

Example: authenticate as `LIB1001` and reserve book index `2`:

```bash
./library_server &
sleep 0.3
printf 'LIB1001\n2\n' | ./library_client
```

Stop the server with `Ctrl+C` or `pkill library_server` when finished.

### Further documentation

- **`Q5/README.md`**

---

## Troubleshooting

| Issue                              | Suggestion                                        |
|------------------------------------|--------------------------------------------------|
| Q2: `nasm: command not found`      | Install NASM (`sudo apt install nasm`).                        |
| Q3: `Python.h: No such file`       | Install Python dev package (e.g. `sudo apt install python3-dev`).         |
| Q5: `bind: Address already in use` | Another process uses port 9090; stop it or change `PORT` in both sources. |
| Q5: client cannot connect          | Ensure server is running and firewall allows localhost TCP 9090.     |

---

## Author 

Denaton Agbikossi
