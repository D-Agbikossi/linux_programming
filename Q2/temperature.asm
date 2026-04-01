; =============================================================================
; temperature.asm — x86-64 Linux (NASM): count lines in temperature_data.txt
;
; Build:
;   nasm -f elf64 temperature.asm -o temperature.o
;   ld temperature.o -o temperature \
;        -dynamic-linker /lib64/ld-linux-x86-64.so.2 \
;        /usr/lib/x86_64-linux-gnu/libc.so.6
;
; Or:  gcc -nostartfiles -no-pie temperature.o -o temperature
;
; File handling: open(2) → fstat(2) → mmap(2) MAP_PRIVATE of file → close(2).
; Line logic: scan buffer; lines end at LF; CRLF handled by trimming CR before
;             classifying a line as empty/valid.
; Control flow: outer loop over buffer position; inner loop until LF or EOF;
;               conditional checks for empty vs valid (non-whitespace) lines.
; =============================================================================

        bits 64

        section .rodata
fname:          db "temperature_data.txt", 0
msg_open_err:   db "Error: cannot open temperature_data.txt", 10
msg_open_len:   equ $ - msg_open_err
label_total:    db "Total readings: "
label_total_len equ $ - label_total
label_valid:    db "Valid readings: "
label_valid_len equ $ - label_valid
newline:        db 10

        section .bss
st_buf:         resb 144                 ; struct stat (x86-64; st_size at +48)
align 8
digit_buf:      resb 24                  ; decimal conversion scratch

        section .data
total_lines:    dq 0
valid_lines:    dq 0

        section .text
        global _start

; Linux x86-64 syscall numbers (subset)
SYS_read        equ 0
SYS_write       equ 1
SYS_open        equ 2
SYS_close       equ 3
SYS_fstat       equ 5
SYS_mmap        equ 9
SYS_exit        equ 60

O_RDONLY        equ 0

PROT_READ       equ 1
MAP_PRIVATE     equ 0x02

; -----------------------------------------------------------------------------
; _start: open file, map into memory, parse lines, print decimal counts, exit.
; -----------------------------------------------------------------------------
_start:
        ; open(fname, O_RDONLY)
        mov     rax, SYS_open
        lea     rdi, [rel fname]
        mov     rsi, O_RDONLY
        xor     edx, edx
        syscall
        cmp     rax, 0
        jl      .open_fail
        mov     r12, rax               ; fd

        ; fstat(fd, &st_buf)
        mov     rdi, r12
        lea     rsi, [rel st_buf]
        mov     rax, SYS_fstat
        syscall
        cmp     rax, 0
        jl      .close_and_fail

        ; size = st_buf.st_size (offset 48 on x86-64)
        mov     r13, [st_buf + 48]     ; file size in bytes
        test    r13, r13
        jz      .empty_file

        ; mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0)
        xor     rdi, rdi
        mov     rsi, r13
        mov     rdx, PROT_READ
        mov     r10, MAP_PRIVATE
        mov     r8, r12
        xor     r9d, r9d
        mov     rax, SYS_mmap
        syscall
        test    rax, rax               ; mmap returns -errno on failure (not only -1)
        js      .close_and_fail
        mov     r14, rax               ; mapped pointer

        mov     rdi, r12
        mov     rax, SYS_close
        syscall

        ; Parse: r14 = buf, r13 = len
        xor     r15, r15               ; index i
.parse_loop:
        cmp     r15, r13
        jge     .done_parse

        mov     rbx, r15               ; line_start

.inner_to_lf:
        cmp     r15, r13
        jge     .line_done_no_lf       ; EOF without trailing newline
        movzx   eax, byte [r14 + r15]
        cmp     al, 10
        je      .line_done_with_lf
        inc     r15
        jmp     .inner_to_lf

.line_done_with_lf:
        ; Line is [rbx, r15) ; LF at r15 (offsets into file; r14 = base)
        lea     rdi, [r14 + rbx]
        lea     rsi, [r14 + r15]
        call    line_classify
        inc     qword [rel total_lines]
        test    rax, rax
        jz      .after_line
        inc     qword [rel valid_lines]
.after_line:
        inc     r15                    ; skip LF
        jmp     .parse_loop

.line_done_no_lf:
        ; Last segment [rbx, r13)
        lea     rdi, [r14 + rbx]
        lea     rsi, [r14 + r13]
        call    line_classify
        inc     qword [rel total_lines]
        test    rax, rax
        jz      .done_parse
        inc     qword [rel valid_lines]

.done_parse:
        jmp     .print_results

.empty_file:
        mov     rdi, r12
        mov     rax, SYS_close
        syscall
        jmp     .print_results

.close_and_fail:
        mov     rdi, r12
        mov     rax, SYS_close
        syscall
.open_fail:
        mov     rax, SYS_write
        mov     edi, 2
        lea     rsi, [rel msg_open_err]
        mov     edx, msg_open_len
        syscall
        mov     rdi, 1
        mov     rax, SYS_exit
        syscall

.print_results:
        ; "Total readings: "
        mov     rax, SYS_write
        mov     edi, 1
        lea     rsi, [rel label_total]
        mov     edx, label_total_len
        syscall
        mov     rdi, [rel total_lines]
        call    print_uint64
        mov     rax, SYS_write
        mov     edi, 1
        lea     rsi, [rel newline]
        mov     edx, 1
        syscall

        ; "Valid readings: "
        mov     rax, SYS_write
        mov     edi, 1
        lea     rsi, [rel label_valid]
        mov     edx, label_valid_len
        syscall
        mov     rdi, [rel valid_lines]
        call    print_uint64
        mov     rax, SYS_write
        mov     edi, 1
        lea     rsi, [rel newline]
        mov     edx, 1
        syscall

        xor     rdi, rdi
        mov     rax, SYS_exit
        syscall

; -----------------------------------------------------------------------------
; line_classify(buf_start, buf_end_exclusive) -> rax 1 if valid, 0 if empty
; Trims optional trailing CR before LF (CRLF). Empty = no non-whitespace byte.
; -----------------------------------------------------------------------------
line_classify:
        mov     r8, rdi                ; start
        mov     r9, rsi                ; end
        ; Trim trailing CR from [r8, r9)
.trim_cr:
        cmp     r8, r9
        jge     .trim_done
        movzx   eax, byte [r9 - 1]
        cmp     al, 13
        jne     .trim_done
        dec     r9
        jmp     .trim_cr
.trim_done:
        mov     rcx, r8
.scan_ws:
        cmp     rcx, r9
        jge     .empty_line
        movzx   eax, byte [rcx]
        inc     rcx
        cmp     al, 32                 ; space
        je      .scan_ws
        cmp     al, 9                  ; tab
        je      .scan_ws
        ; Non-whitespace found -> valid reading line
        mov     rax, 1
        ret
.empty_line:
        xor     rax, rax
        ret

; -----------------------------------------------------------------------------
; print_uint64 — write unsigned decimal for rdi to stdout (syscall write)
; -----------------------------------------------------------------------------
print_uint64:
        mov     rbx, rdi
        mov     rax, rbx
        test    rax, rax
        jnz     .not_zero
        mov     rax, SYS_write
        mov     edi, 1
        lea     rsi, [rel digit_buf + 19]
        mov     byte [rsi], '0'
        mov     edx, 1
        syscall
        ret
.not_zero:
        lea     rsi, [rel digit_buf + 20]
        mov     rcx, 10
.conv:
        xor     edx, edx
        mov     rax, rbx
        div     rcx
        mov     rbx, rax
        dec     rsi
        add     dl, '0'
        mov     [rsi], dl
        test    rbx, rbx
        jnz     .conv
        lea     rax, [rel digit_buf + 20]
        sub     rax, rsi
        mov     rdx, rax
        mov     rax, SYS_write
        mov     edi, 1
        syscall
        ret
