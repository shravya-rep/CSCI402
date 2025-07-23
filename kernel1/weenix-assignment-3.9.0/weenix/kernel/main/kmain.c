/******************************************************************************/
/* Important Spring 2024 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "types.h"
#include "globals.h"
#include "kernel.h"
#include "errno.h"

#include "util/gdb.h"
#include "util/init.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/printf.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/pagetable.h"
#include "mm/pframe.h"

#include "vm/vmmap.h"
#include "vm/shadowd.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "main/acpi.h"
#include "main/apic.h"
#include "main/interrupt.h"
#include "main/gdt.h"

#include "proc/sched.h"
#include "proc/proc.h"
#include "proc/kthread.h"

#include "drivers/dev.h"
#include "drivers/blockdev.h"
#include "drivers/disk/ata.h"
#include "drivers/tty/virtterm.h"
#include "drivers/pci.h"

#include "api/exec.h"
#include "api/syscall.h"

#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/fcntl.h"
#include "fs/stat.h"

#include "test/kshell/kshell.h"
#include "test/s5fs_test.h"

GDB_DEFINE_HOOK(initialized)

void      *bootstrap(int arg1, void *arg2);
void      *idleproc_run(int arg1, void *arg2);
kthread_t *initproc_create(void);
void      *initproc_run(int arg1, void *arg2);
void      *final_shutdown(void);


extern void *faber_thread_test(int arg1, void *arg2);
extern void *sunghan_test(int arg1, void *arg2);
extern void *sunghan_deadlock_test(int arg1, void *arg2);


#ifdef __DRIVERS__

        int do_faber(kshell_t *kshell, int argc, char **argv) {
            KASSERT(kshell != NULL);
            int rv;
            proc_t *proc = proc_create("faber");
            kthread_t *thr = kthread_create(proc, faber_thread_test, 0, NULL);
            KASSERT(proc && thr && "Cannot create thread or process");
            sched_make_runnable(thr);
            do_waitpid(proc->p_pid, 0, &rv);
            dbg(DBG_PRINT, "(GRADING1C 1)\n");
            return 0;
        }

        int do_sunghan(kshell_t *kshell, int argc, char **argv) {
            KASSERT(kshell != NULL);
            int rv;
            proc_t *proc = proc_create("sunghan");
            kthread_t *thr = kthread_create(proc, sunghan_test, 0, NULL);
            KASSERT(proc && thr && "Cannot create thread or process");
            sched_make_runnable(thr);
            do_waitpid(proc->p_pid, 0, &rv);
            dbg(DBG_PRINT, "(GRADING1D 1)\n");
            return 0;
        }

        int do_deadlock(kshell_t *kshell, int argc, char **argv) {
            KASSERT(kshell != NULL);
            int rv;
            proc_t *proc = proc_create("deadlock");
            kthread_t *thr = kthread_create(proc, sunghan_deadlock_test, 0, NULL);
            KASSERT(proc && thr && "Cannot create thread or process");
            sched_make_runnable(thr);
            do_waitpid(proc->p_pid, 0, &rv);
            dbg(DBG_PRINT, "(GRADING1D 2)\n");
            return 0;
        }

#endif /* __DRIVERS__ */


/**
 * This function is called from kmain, however it is not running in a
 * thread context yet. It should create the idle process which will
 * start executing idleproc_run() in a real thread context.  To start
 * executing in the new process's context call context_make_active(),
 * passing in the appropriate context. This function should _NOT_
 * return.
 *
 * Note: Don't forget to set curproc and curthr appropriately.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
bootstrap(int arg1, void *arg2)
{
        /* If the next line is removed/altered in your submission, 20 points will be deducted. */
        dbgq(DBG_TEST, "SIGNATURE: 53616c7465645f5f01bbab796ed47c2ba53d403c2eb773558e7857a5070e63a978fe1e6adef1f419e1694676014b75f8\n");
        /* necessary to finalize page table information */
        pt_template_init();

        curproc= proc_create("IDLE");
        KASSERT(NULL != curproc);
        dbg(DBG_PRINT, "(GRADING1A 1.a)\n");
        KASSERT(PID_IDLE == curproc->p_pid);
        dbg(DBG_PRINT, "(GRADING1A 1.a)\n");
        kthread_func_t firstProced=(kthread_func_t)&idleproc_run;
        curthr= kthread_create(curproc,firstProced,0,NULL);
        context_t *pointToThreadContext=&(curthr->kt_ctx);
        KASSERT(NULL != curthr);
        dbg(DBG_PRINT, "(GRADING1A 1.a)\n");
        context_make_active(pointToThreadContext);

        panic("weenix returned to bootstrap()!!! BAD!!!\n");
        return NULL;
}

/**
 * Once we're inside of idleproc_run(), we are executing in the context of the
 * first process-- a real context, so we can finally begin running
 * meaningful code.
 *
 * This is the body of process 0. It should initialize all that we didn't
 * already initialize in kmain(), launch the init process (initproc_run),
 * wait for the init process to exit, then halt the machine.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
idleproc_run(int arg1, void *arg2)
{
        int status;
        pid_t child;

        /* create init proc */
        kthread_t *initthr = initproc_create();
        init_call_all();
        GDB_CALL_HOOK(initialized);

        /* Create other kernel threads (in order) */

#ifdef __VFS__
        /* Once you have VFS remember to set the current working directory
         * of the idle and init processes */
        NOT_YET_IMPLEMENTED("VFS: idleproc_run");

        /* Here you need to make the null, zero, and tty devices using mknod */
        /* You can't do this until you have VFS, check the include/drivers/dev.h
         * file for macros with the device ID's you will need to pass to mknod */
        NOT_YET_IMPLEMENTED("VFS: idleproc_run");
#endif

        /* Finally, enable interrupts (we want to make sure interrupts
         * are enabled AFTER all drivers are initialized) */
        intr_enable();

        /* Run initproc */
        sched_make_runnable(initthr);
        /* Now wait for it */
        child = do_waitpid(-1, 0, &status);
        KASSERT(PID_INIT == child);

        return final_shutdown();
}

/**
 * This function, called by the idle process (within 'idleproc_run'), creates the
 * process commonly refered to as the "init" process, which should have PID 1.
 *
 * The init process should contain a thread which begins execution in
 * initproc_run().
 *
 * @return a pointer to a newly created thread which will execute
 * initproc_run when it begins executing
 */
kthread_t *
initproc_create(void)
{
<<<<<<< HEAD

        proc_t* pointToInitProc=proc_create("INIT");
        KASSERT(NULL != pointToInitProc);
=======
        proc_t* p = proc_create("INIT");
        KASSERT(NULL != p);
>>>>>>> 6595d2356bd351c98a03e20add31c5a333183f7c
        dbg(DBG_PRINT, "(GRADING1A 1.b)\n");
        KASSERT(PID_INIT == p->p_pid);
        dbg(DBG_PRINT, "(GRADING1A 1.b)\n");
        kthread_func_t firstProced=(kthread_func_t)&initproc_run;
        kthread_t* thr = kthread_create(p,firstProced,0,0);
        KASSERT(NULL != thr);
        dbg(DBG_PRINT, "(GRADING1A 1.b)\n");
<<<<<<< HEAD
        return pointToThread;

=======
        dbg(DBG_PRINT, "(GRADING1A)\n");
        return thr;
>>>>>>> 6595d2356bd351c98a03e20add31c5a333183f7c
}

#ifdef __DRIVERS__
static void* do_faber()
{
    proc_t* newProc=proc_create("new faber process");
    kthread_t* newThr=kthread_create(newProc,(kthread_func_t)&faber_thread_test,0,0);
    sched_make_runnable(newThr);
    int exitStatusOfNewThr;
    do_waitpid(newProc->p_pid,0,&exitStatusOfNewThr);
    return NULL;
}
#endif

/**
 * The init thread's function changes depending on how far along your Weenix is
 * developed. Before VM/FI, you'll probably just want to have this run whatever
 * tests you've written (possibly in a new process). After VM/FI, you'll just
 * exec "/sbin/init".
 *
 * Both arguments are unused.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)faber_thread_test(0, NULL);
 */
void *
initproc_run(int arg1, void *arg2)
{
    //faber_thread_test(0, NULL);

    #ifdef __DRIVERS__
        kshell_add_command("faber", do_faber, "Run faber_thread_test().");
        kshell_add_command("sunghan", do_sunghan, "Run sunghan_test().");
        kshell_add_command("deadlock", do_deadlock, "Run sunghan_deadlock_test().");

        kshell_t *kshell = kshell_create(0);
        if (NULL == kshell) panic("init: Couldn't create kernel shell\n");
        dbg(DBG_PRINT, "(GRADING1B)\n");
        while (kshell_execute_next(kshell));
        dbg(DBG_PRINT, "(GRADING1B)\n");
        kshell_destroy(kshell);


    #endif /* __DRIVERS__ */

        int returnStatus;
        while(do_waitpid(-1, 0, &returnStatus) != -ECHILD);
        dbg(DBG_PRINT, "(GRADING1A)\n");

    return NULL;
}
