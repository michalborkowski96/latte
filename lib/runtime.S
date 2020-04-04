_input_buffer_size equ 4096
_print_int_buffer_size equ 25
section .text
global _concat
global _alloc
global _new_array
global error
global printString
global readString
global printInt
global readInt

;_functions might be accessible from the code and thus need to follow the calling convention (params on the stack, return in rax, clobber everything)
;__functions are private for this file, thus freestyle

_new_array:
	mov rax, [rsp+8]
	test rax, rax
	mov rbx, _empty_arr
	cmovz rax, rbx
	jz _after_new_array
	mov rbx, 0xf000000000000000
	test rax, rbx
	jnz error
	shl rax, 3
	add rax, 8
	push rax
	call _alloc
	add rsp, 8
	mov rbx, [rsp+8]
	mov [rax], rbx
	mov rcx, [rsp+16]
	lea rbx, [rax + rbx * 8]
	lea rdx, [rax + 8]
	jmp _new_array_fill_cond
_new_array_fill_act:
	mov [rdx], rcx
	add rdx, 8
_new_array_fill_cond:
	cmp rdx, rbx
	jle _new_array_fill_act
_after_new_array:
	ret

_printNewline:
	mov rax, 1
	mov rdi, 1
	push qword 10
	mov rsi, rsp
	mov rdx, 1
	syscall
	add rsp, 8
	ret

__read_from_stdin:
	mov rax, 0
	mov rdi, 0
	mov rsi, _read_buffer
	mov rdx, _input_buffer_size
	syscall
	cmp rax, 0
	jl error
	ret

printInt:
	mov r10, 10
	mov rcx, _print_int_buffer + _print_int_buffer_size - 1
	mov rax, qword [rsp+8]
	xor r9, r9
	cmp rax, 0
	setl r9b
	mov r12b, r9b
	mov r11, -2
	imul r9, r11
	inc r9
	imul rax, r9
_printInt_loop:
	cmp rax, 10
	jl _printInt_last_digit
	xor rdx, rdx
	idiv r10
	add dl, '0'
	mov [rcx], dl
	dec rcx
	jmp _printInt_loop
_printInt_last_digit:
	add al, '0'
	mov [rcx], al
	test r12b, r12b
	jz _printInt_after_minus
	dec rcx
	mov [rcx], byte '-'
_printInt_after_minus:
	mov rax, 1
	mov rdi, 1
	mov rsi, rcx
	mov rdx, _print_int_buffer+_print_int_buffer_size
	sub rdx, rcx
	syscall
	jmp _printNewline

readInt:
	mov rbx, [_read_buffer_pos]
	mov rax, [_read_buffer_limit]
	cmp rax, rbx
	jne _readInt_no_read
	call __read_from_stdin
	test rax, rax
	jz error
	mov rbx, 0
_readInt_no_read:
	mov r10b, 0
	lea rcx, [rax + _read_buffer]
	mov rax, 0
	lea rdx, [rbx + _read_buffer]
	mov rbx, 1
	cmp byte [rdx], '-'
	jne _readInt_continue
	mov rbx, -1
	inc rdx
	cmp rdx, rcx
	jne _readInt_continue
	call __read_from_stdin
	test rax, rax
	jz error
	lea rcx, [rax + _read_buffer]
	mov rdx, _read_buffer
	mov rax, 0
	mov rbx, -1
	mov r10b, 0
_readInt_continue:
	cmp rdx, rcx
	jne _readInt_doesnt_need_more
	push rax
	push rbx
	push r10
	call __read_from_stdin
	lea rcx, [rax + _read_buffer]
	pop r10
	test rax, rax
	jz _readInt_finish_eof
	pop rbx
	pop rax
	mov rdx, _read_buffer
_readInt_doesnt_need_more:
	mov r9b, [rdx]
	cmp r9b, '0'
	jl _readInt_finish_bad_char
	cmp r9b, '9'
	jg _readInt_finish_bad_char
	sub r9b, '0'
	movzx r9, r9b
	mov r12, 922337203685477579
	cmp rax, r12
	jge error
	imul rax, 10
	add rax, r9
	mov r10b, 1
	inc rdx
	jmp _readInt_continue
_readInt_finish_bad_char:
	imul rax, rbx
	push rax
	call _readInt_eat_whitespaces
	sub rdx, _read_buffer
	mov [_read_buffer_pos], rdx
	sub rcx, _read_buffer
	mov [_read_buffer_limit], rcx
	pop rax
	test r10b, r10b
	jz error
	ret
_readInt_finish_eof:
	pop rbx
	pop rax
	mov qword [_read_buffer_pos], 0
	mov qword [_read_buffer_limit], 0
	imul rax, rbx
	test r10b, r10b
	jz error
	ret
_readInt_eat_whitespaces:
	cmp rdx, rcx
	jne _readInt_eat_whitespaces_no_read
	call __read_from_stdin
	lea rcx, [rax + _read_buffer]
	mov rdx, _read_buffer
	test rax, rax
	jz _readInt_eat_whitespaces_end
_readInt_eat_whitespaces_no_read:
	mov al, byte [rdx]
	cmp al, 9
	je _readInt_eat_whitespaces_char_match
	cmp al, 10
	je _readInt_eat_whitespaces_char_match
	cmp al, 11
	je _readInt_eat_whitespaces_char_match
	cmp al, 12
	je _readInt_eat_whitespaces_char_match
	cmp al, 13
	je _readInt_eat_whitespaces_char_match
	cmp al, 32
	jne _readInt_eat_whitespaces_end
_readInt_eat_whitespaces_char_match:
	inc rdx
	cmp al, 10
	je _readInt_eat_whitespaces_end
	jmp _readInt_eat_whitespaces
_readInt_eat_whitespaces_end:
	ret

printString:
	mov rax, 1
	mov rdi, 1
	mov rsi, [rsp+8]
	mov rdx, [rsi]
	add rsi, 8
	syscall
	jmp _printNewline

readString: ;r12 -> result, r13 -> cur_pos, r14 -> _read_buffer + cur_pos, r15 -> [_read_buffer_limit], r8 -> starting pos
	mov r12, _empty_str
	mov r13, [_read_buffer_pos]
	mov r8, r13
	mov r15, [_read_buffer_limit]
	mov r14, _read_buffer
	add r14, r13
	jmp _readString_1
_readString_0:
	inc r13;
	inc r14;
_readString_1:
	cmp r13, r15
	je _readString_2
	cmp byte [r14], 10
	jne _readString_0
_readString_2:
	push r13
	push r15
	push r12
	sub r13, r8
	sub r14, r13
	sub r14, 8
	mov [r14], r13
	push r14
	call _concat
	mov r12, rax
	add rsp, 16
	pop r15
	pop r13
	cmp r13, r15
	je _readString_3
	inc r13
	mov [_read_buffer_pos], r13
_readString_4:
	mov rax, r12
	ret
_readString_3:
	call __read_from_stdin
	cmp rax, 0
	je _readString_4
	mov qword [_read_buffer_pos], 0
	mov r13, 0
	mov r8, 0
	mov r14, _read_buffer
	mov [_read_buffer_limit], rax
	mov r15, rax
	jmp _readString_1

_concat:
	mov r14, qword [rsp+16]
	mov r15, qword [rsp+8]
	mov r9, [r14]
	mov r10, [r15]
	test r9, r9
	jz _concat_ret_sec
	test r10, r10
	jz _concat_ret_fst
	add r14, 8
	add r15, 8
	lea rdx, [r9+r10+8]
	push rdx
	call _alloc
	pop rdx
	sub rdx, 8
	mov [rax], rdx
	lea r11, [rax+8]
	mov rcx, r9
_concat_first:
	mov dl, [r14]
	mov [r11], dl
	inc r14
	inc r11
	loop _concat_first
	mov rcx, r10
_concat_second:
	mov dl, [r15]
	mov [r11], dl
	inc r15
	inc r11
	loop _concat_second
	ret
_concat_ret_fst:
	mov rax, r14
	test r9, r9
	jz _concat_ret_empty
	add r9, 8
	push r9
	sub r9, 8
	call _alloc
	pop rdx
	mov rcx, qword [r14]
	mov qword [rax], rcx
	add r14, 8
	lea rdx, [rax+8]
_concat_copy:
	mov bl, [r14]
	mov [rdx], bl
	inc r14
	inc rdx
	loop _concat_copy
	ret
_concat_ret_sec:
	xchg r14, r15
	xchg r9, r10
	jmp _concat_ret_fst
_concat_ret_empty:
	mov rax, _empty_str
	ret

_alloc:
	mov r12, [rsp+8]
	cmp r12, 0
	jl error
	mov rdi, 0
	mov rax, 12
	syscall
	mov rdi, rax
	mov rbx, rax
	add rdi, r12
	mov rax, 12
	syscall
	cmp rbx, rax
	je error
	mov rax, rbx
	ret

error:
	mov rdi, 1
	mov rax, 60
	syscall

section .data
global _empty_str
global _empty_arr
_read_buffer_pos dq 0
_read_buffer_limit dq 0
_empty_str dq 0
_empty_arr dq 0

section .bss
_read_buffer_size resq 1
_read_buffer resb _input_buffer_size

_print_int_buffer resb _print_int_buffer_size
