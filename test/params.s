	.file	"params.c"
	.intel_syntax noprefix
	.text
	.globl	add
	.type	add, @function
add:
	push	ebp
	mov	ebp, esp
	sub	esp, 4
	mov	edx, DWORD PTR [ebp+8]
	mov	eax, DWORD PTR [ebp+12]
	add	eax, edx
	mov	DWORD PTR [ebp-4], eax
	mov	eax, DWORD PTR [ebp-4]
	leave
	ret
	.size	add, .-add
	.globl	main
	.type	main, @function
main:
	push	ebp
	mov	ebp, esp
	sub	esp, 12
	mov	DWORD PTR [ebp-12], 5
	mov	DWORD PTR [ebp-8], 3
	push	DWORD PTR [ebp-8]
	push	DWORD PTR [ebp-12]
	call	add
	add	esp, 8
	mov	DWORD PTR [ebp-4], eax
	mov	eax, 0
	leave
	ret
	.size	main, .-main
	.section	.note.GNU-stack,"",@progbits
