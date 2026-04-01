# Q2 — Temperature statistics (NASM x86-64)

## Purpose

Reads **`temperature_data.txt`** from the **current directory**, maps the file into memory, counts all lines (including empty) and counts **valid** lines (non-empty after CRLF trim and whitespace rules).

## Dependencies

- **NASM** (`sudo apt install nasm`)
- **GCC** (linking)

## Compilation

```bash
nasm -f elf64 temperature.asm -o temperature.o
gcc -nostartfiles -no-pie temperature.o -o temperature
```

`gcc` is used instead of raw `ld` so the usual dynamic linker and `libc` paths match your distribution.

## Execution

```bash
./temperature
```

## Inputs

`temperature_data.txt`

Lines may use **LF** (`\n`) or **CRLF** (`\r\n`). |

No command-line arguments.

## Expected output

With the bundled sample file, expect:

```text
Total readings: 6
Valid readings: 4
```

**Meaning:**

- **Total readings:** number of lines (empty lines count; if the file ends with an extra newline after the last reading, that adds another empty line).
- **Valid readings:** lines with at least one character other than space or tab (after stripping a trailing carriage return before line feed).

## Error output

If `temperature_data.txt` cannot be opened:

```text
Error: cannot open temperature_data.txt
```

## Files

| File                   | Description                                 |
|------------------------|---------------------------------------------|
| `temperature.asm`      | NASM source                                 |
| `temperature`          | Built executable (after compile step above) |
| `temperature_data.txt` | Sample input                                |
