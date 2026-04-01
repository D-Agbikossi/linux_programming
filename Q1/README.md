# Q1 — ELF reverse-engineering program

## Purpose

Small C program used for **`readelf`**, **`objdump`**, **`strace`**, **GDB**, and **stripped**-binary analysis. See project report for the full write-up.

## Compilation

```bash
gcc -Wall -O0 -fno-inline -o program program.c
strip program
```

Unstripped binary (optional, for symbols in GDB):
 
```bash
gcc -Wall -O0 -fno-inline -o program_unstripped program.c
```

## Execution

```bash
./program
```

## Inputs

None.

## Expected output

```text
computed_sum=60
```

## Analysis tools (examples)

```bash
readelf -h program
readelf -S program
objdump -d program_unstripped
strace ./program
gdb ./program_unstripped
```

## Files

| File                 | Description                       |
|----------------------|-----------------------------------|
| `program.c`          | Source                            |
| `program`            | Stripped executable (submit)      |
| `program_unstripped` | Optional; same code, not stripped |

Project Report:
