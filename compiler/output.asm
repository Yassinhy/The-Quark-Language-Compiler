section .text
	global _start
_start:
push rbp
mov rbp, rsp
sub rsp, 4
mov rax, 8

mov [rbp - 4], rax
push r9
push r8
push rcx
push rdx
push rsi
push rdi
mov rax, [rbp - 4]
mov rdi, rax
call add
pop rdi
pop rsi
pop rdx
pop rcx
pop r8
pop r9
mov rax, 5

push rax
mov rax, 3

pop rbx
add rax, rbx

mov [rbp - 4], rax
mov rax, [rbp - 4]

mov rdi, rax
mov rax, 60
syscall

; Exit program
mov rax, 60
mov rdi, 0
syscall



add:
push rbp
mov rbp, rsp
sub rsp, 4
mov rax, 67

mov [rbp - 4], rax
mov rax, rdi

push rax
mov rax, [rbp - 4]

pop rbx
cmp rax, rbx
jne .Lels0
mov rax, 5

mov rdi, rax
mov rax, 60
syscall
jmp .end_if_0
.Lels0:
mov rax, [rbp - 4]

mov rdi, rax
mov rax, 60
syscall
.end_if_0:
ret

; Exit program
mov rax, 60
mov rdi, 0
syscall
section .rodata
err_div0:	db "division by zero", 10
err_div0_len:  equ $ - err_div0
