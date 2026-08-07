; Generated ARM64 Assembly Library for host macOS
.global _sp_exit
.global _sp_fork
.global _sp_read
.global _sp_write
.global _sp_open
.global _sp_close
.global _sp_unlink
.global _sp_getpid
.global _sp_kill
.global _sp_dup
.global _sp_pipe
.global _sp_mmap
.global _sp_munmap
.align 4

_sp_exit:
	mov x16, #1
	svc #0x80
	ret

_sp_fork:
	mov x16, #2
	svc #0x80
	ret

_sp_read:
	mov x16, #3
	svc #0x80
	ret

_sp_write:
	mov x16, #4
	svc #0x80
	ret

_sp_open:
	mov x16, #5
	svc #0x80
	ret

_sp_close:
	mov x16, #6
	svc #0x80
	ret

_sp_unlink:
	mov x16, #10
	svc #0x80
	ret

_sp_getpid:
	mov x16, #20
	svc #0x80
	ret

_sp_kill:
	mov x16, #37
	svc #0x80
	ret

_sp_dup:
	mov x16, #41
	svc #0x80
	ret

_sp_pipe:
	mov x16, #42
	svc #0x80
	ret

_sp_mmap:
	mov x16, #197
	svc #0x80
	ret

_sp_munmap:
	mov x16, #191
	svc #0x80
	ret

