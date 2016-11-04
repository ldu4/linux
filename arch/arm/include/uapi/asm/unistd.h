/*
 *  arch/arm/include/asm/unistd.h
 *
 *  Copyright (C) 2001-2005 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Please forward _all_ changes to this file to rmk@arm.linux.org.uk,
 * no matter what the change is.  Thanks!
 */
#ifndef _UAPI__ASM_ARM_UNISTD_H
#define _UAPI__ASM_ARM_UNISTD_H

#define __NR_OABI_SYSCALL_BASE	0x900000

#if defined(__thumb__) || defined(__ARM_EABI__)
#define __NR_SYSCALL_BASE	0
#include <asm/unistd-eabi.h>
#else
#define __NR_SYSCALL_BASE	__NR_OABI_SYSCALL_BASE
#include <asm/unistd-oabi.h>
#endif

#include <asm/unistd-common.h>
#define __NR_sync_file_range2		__NR_arm_sync_file_range
<<<<<<< HEAD
#define __NR_tee			(__NR_SYSCALL_BASE+342)
#define __NR_vmsplice			(__NR_SYSCALL_BASE+343)
#define __NR_move_pages			(__NR_SYSCALL_BASE+344)
#define __NR_getcpu			(__NR_SYSCALL_BASE+345)
#define __NR_epoll_pwait		(__NR_SYSCALL_BASE+346)
#define __NR_kexec_load			(__NR_SYSCALL_BASE+347)
#define __NR_utimensat			(__NR_SYSCALL_BASE+348)
#define __NR_signalfd			(__NR_SYSCALL_BASE+349)
#define __NR_timerfd_create		(__NR_SYSCALL_BASE+350)
#define __NR_eventfd			(__NR_SYSCALL_BASE+351)
#define __NR_fallocate			(__NR_SYSCALL_BASE+352)
#define __NR_timerfd_settime		(__NR_SYSCALL_BASE+353)
#define __NR_timerfd_gettime		(__NR_SYSCALL_BASE+354)
#define __NR_signalfd4			(__NR_SYSCALL_BASE+355)
#define __NR_eventfd2			(__NR_SYSCALL_BASE+356)
#define __NR_epoll_create1		(__NR_SYSCALL_BASE+357)
#define __NR_dup3			(__NR_SYSCALL_BASE+358)
#define __NR_pipe2			(__NR_SYSCALL_BASE+359)
#define __NR_inotify_init1		(__NR_SYSCALL_BASE+360)
#define __NR_preadv			(__NR_SYSCALL_BASE+361)
#define __NR_pwritev			(__NR_SYSCALL_BASE+362)
#define __NR_rt_tgsigqueueinfo		(__NR_SYSCALL_BASE+363)
#define __NR_perf_event_open		(__NR_SYSCALL_BASE+364)
#define __NR_recvmmsg			(__NR_SYSCALL_BASE+365)
#define __NR_accept4			(__NR_SYSCALL_BASE+366)
#define __NR_fanotify_init		(__NR_SYSCALL_BASE+367)
#define __NR_fanotify_mark		(__NR_SYSCALL_BASE+368)
#define __NR_prlimit64			(__NR_SYSCALL_BASE+369)
#define __NR_name_to_handle_at		(__NR_SYSCALL_BASE+370)
#define __NR_open_by_handle_at		(__NR_SYSCALL_BASE+371)
#define __NR_clock_adjtime		(__NR_SYSCALL_BASE+372)
#define __NR_syncfs			(__NR_SYSCALL_BASE+373)
#define __NR_sendmmsg			(__NR_SYSCALL_BASE+374)
#define __NR_setns			(__NR_SYSCALL_BASE+375)
#define __NR_process_vm_readv		(__NR_SYSCALL_BASE+376)
#define __NR_process_vm_writev		(__NR_SYSCALL_BASE+377)
#define __NR_kcmp			(__NR_SYSCALL_BASE+378)
#define __NR_finit_module		(__NR_SYSCALL_BASE+379)
#define __NR_sched_setattr		(__NR_SYSCALL_BASE+380)
#define __NR_sched_getattr		(__NR_SYSCALL_BASE+381)
#define __NR_renameat2			(__NR_SYSCALL_BASE+382)
#define __NR_seccomp			(__NR_SYSCALL_BASE+383)
#define __NR_getrandom			(__NR_SYSCALL_BASE+384)
#define __NR_memfd_create		(__NR_SYSCALL_BASE+385)
#define __NR_bpf			(__NR_SYSCALL_BASE+386)
#define __NR_execveat			(__NR_SYSCALL_BASE+387)
#define __NR_userfaultfd		(__NR_SYSCALL_BASE+388)
#define __NR_membarrier			(__NR_SYSCALL_BASE+389)
#define __NR_mlock2			(__NR_SYSCALL_BASE+390)
#define __NR_copy_file_range		(__NR_SYSCALL_BASE+391)
#define __NR_preadv2			(__NR_SYSCALL_BASE+392)
#define __NR_pwritev2			(__NR_SYSCALL_BASE+393)
#define __NR_pkey_mprotect		(__NR_SYSCALL_BASE+394)
#define __NR_pkey_alloc			(__NR_SYSCALL_BASE+395)
#define __NR_pkey_free			(__NR_SYSCALL_BASE+396)
=======
>>>>>>> linux-next/akpm-base

/*
 * The following SWIs are ARM private.
 */
#define __ARM_NR_BASE			(__NR_SYSCALL_BASE+0x0f0000)
#define __ARM_NR_breakpoint		(__ARM_NR_BASE+1)
#define __ARM_NR_cacheflush		(__ARM_NR_BASE+2)
#define __ARM_NR_usr26			(__ARM_NR_BASE+3)
#define __ARM_NR_usr32			(__ARM_NR_BASE+4)
#define __ARM_NR_set_tls		(__ARM_NR_BASE+5)

#endif /* _UAPI__ASM_ARM_UNISTD_H */
