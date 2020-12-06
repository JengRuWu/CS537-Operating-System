#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "pstat.h"

//the queue of every priority level
struct proc* pri0[64];
struct proc* pri1[64];
struct proc* pri2[64];
struct proc* pri3[64];
//number of processes at each queue level
int c0=-1;
int c1=-1;
int c2=-1;
int c3=-1;

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  //set the tail and tick to 0
  for(int i=0;i<4;i++){
    p->qtail[i] = 0;
    p->ticks[i] = 0;
  }
  
  release(&ptable.lock);

  // Allocate kernel stack if possible.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;
  
  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;
  
  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
  
  p = allocproc();
  //initial process priority = 3
  p->priority = 3;
  c3++;
  pri3[c3] = p;

  acquire(&ptable.lock);
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  
  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;
  //get the priority from parent
  np->priority = proc->priority;
  //put process into queue
  if(np->priority==0){
    c0++;
    pri0[c0]=np;
    np->qtail[0]++;
  }else if(np->priority==1){
    c1++;
    pri1[c1]=np;
    np->qtail[1]++;
  }else if(np->priority==2){
    c2++;
    pri2[c2]=np;
    np->qtail[2]++;
  }else{
    c3++;
    pri3[c3]=np;
    np->qtail[3]++;
  }

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);
 
  pid = np->pid;
  np->state = RUNNABLE;
  safestrcpy(np->name, proc->name, sizeof(proc->name));
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  iput(proc->cwd);
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  int i;
  int j;
  for(;;){
    // Enable interrupts on this processor.
    int highprirunnable = 0;

    sti();
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    if(c3!=-1){
      for(i=0;i<=c3;i++){
        if(pri3[i]->state !=RUNNABLE){
          continue;
        }
        p=pri3[i];
        proc = pri3[i];

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
        switchuvm(p);
        p->state = RUNNING;
        swtch(&cpu->scheduler, proc->context);
        switchkvm();
        //cprintf("process %d (priority %d)just ran: %d (%d)\n", proc->pid, p->priority, p->timecheck, p->qtail[p->priority]);
        // Process is done running for now.
      // It should have changed its p->state before coming back.

      //look for other ready processes and put them in the queue
        for(int s=0;s<i;s++){
          if(pri3[s]->state!=RUNNABLE){
            continue;
          }else{
            pri3[c3+1]=pri3[s];
            for(int y=s;y<=c3;y++){
              pri3[y]=pri3[y+1];
            }
            pri3[c3+1]=NULL;
            i--;
          }
        }
        //if the process finishes its time slice
        if(p->timecheck>=8){
          p->timecheck=0;
          p->qtail[3]++;
          pri3[c3+1]=p;
          for(j=i;j<=c3;j++){
            pri3[j]=pri3[j+1];
          }
          pri3[c3+1]=NULL;
        }
        proc = 0;
        i--;
      }
    }
    if(c2!=-1){
      for(i=0;i<=c2;i++){
        if(highprirunnable==1){
          break;
        }
        if(pri2[i]->state !=RUNNABLE){
          continue;
        }
        p=pri2[i];
        proc = pri2[i];

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
        switchuvm(p);
        p->state = RUNNING;
        swtch(&cpu->scheduler, proc->context);
        switchkvm();
        //cprintf("process %d (priority %d)just ran: %d (%d)\n", proc->pid, p->priority, p->timecheck, p->qtail[p->priority]);
        // Process is done running for now.
      // It should have changed its p->state before coming back.

        //look for other ready processes and put them in the queue
        for(int s=0;s<i;s++){
          if(pri2[s]->state!=RUNNABLE){
            continue;
          }else{
            pri2[c2+1]=pri2[s];
            for(int y=s;y<=c2;y++){
              pri2[y]=pri2[y+1];
            }
            pri2[c2+1]=NULL;
            i--;
          }
        }
        //See if any priority 3 process is runnable
        for(int s=0;s<=c3;s++){
          if(pri3[s]->state!=RUNNABLE){
            continue;
          }else{
            highprirunnable=1;
          }
        }
        //if the process finishes its time slice
        if(p->timecheck>=12){
          p->timecheck=0;
          p->qtail[2]++;
          pri2[c2+1]=p;
          for(j=i;j<=c2;j++){
            pri2[j]=pri2[j+1];
          }
          pri2[c2+1]=NULL;
        }
        proc = 0;
        i--;
      }
    }
    if(c1!=-1 && highprirunnable==0){
      for(i=0;i<=c1;i++){
        if(highprirunnable==1){
          break;
        }        
        if(pri1[i]->state !=RUNNABLE){
          continue;
        }
        p=pri1[i];
        proc = pri1[i];

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
        switchuvm(p);
        p->state = RUNNING;
        swtch(&cpu->scheduler, proc->context);
        switchkvm();
        //cprintf("process %d (priority %d)just ran: %d (%d)\n", proc->pid, p->priority, p->timecheck, p->qtail[p->priority]);
        // Process is done running for now.
      // It should have changed its p->state before coming back.
              //look for other ready processes and put them in the queue
        for(int s=0;s<i;s++){
          if(pri1[s]->state!=RUNNABLE){
            continue;
          }else{
            pri1[c1+1]=pri1[s];
            for(int y=s;y<=c1;y++){
              pri1[y]=pri1[y+1];
            }
            pri1[c1+1]=NULL;
            i--;
          }
        }
        //See if any priority 2 or 3 process is runnable
        for(int s=0;s<=c2;s++){
          if(pri2[s]->state!=RUNNABLE){
            continue;
          }else{
            highprirunnable=1;
          }
        }
        for(int s=0;s<=c3;s++){
          if(pri3[s]->state!=RUNNABLE){
            continue;
          }else{
            highprirunnable=1;
          }
        }
        
        //if the process finishes its time slice
        if(p->timecheck>=16){
          p->timecheck=0;
          p->qtail[1]++;
          pri1[c1+1]=p;
          for(j=i;j<=c1;j++){
            pri1[j]=pri1[j+1];
          }
          pri1[c1+1]=NULL;
        }
        proc = 0;
        i--;
      }
    }
    if(c0!=-1 && highprirunnable==0){
      for(i=0;i<=c0;i++){
        if(highprirunnable==1){break;}
        if(pri0[i]->state !=RUNNABLE){
          continue;
        }
        p=pri0[i];
        proc = pri0[i];
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
        switchuvm(p);
        p->state = RUNNING;
        swtch(&cpu->scheduler, proc->context);
        switchkvm();
        //cprintf("process %d (priority %d)just ran: %d (%d)\n", proc->pid, p->priority, p->timecheck, p->qtail[p->priority]);
        // Process is done running for now.
      // It should have changed its p->state before coming back.
        //See if any priority 2 or 3 process is runnable
        for(int s=0;s<=c2;s++){
          if(pri2[s]->state!=RUNNABLE){
            continue;
          }else{
            highprirunnable=1;
          }
        }
        for(int s=0;s<=c3;s++){
          if(pri3[s]->state!=RUNNABLE){
            continue;
          }else{
            highprirunnable=1;
          }
        }
        for(int s=0;s<=c1;s++){
          if(pri1[s]->state!=RUNNABLE){
            continue;
          }else{
            highprirunnable=1;
          }
        }

      //look for other ready processes and put them in the queue
        for(int s=0;s<i;s++){
          if(pri0[s]->state!=RUNNABLE){
            continue;
          }else{
            pri0[c0+1]=pri0[s];
            for(int y=s;y<=c3;y++){
              pri0[y]=pri0[y+1];
            }
            pri0[c3+1]=NULL;
            i--;
          }
        }
        proc = 0;
        i--;
      }
    }
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  proc->timecheck++;
  proc->ticks[proc->priority]++;
  //cprintf("yield: %s[pid %d]\n", proc->name, proc->pid);
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);
  
  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];
  
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

int
getpinfo(struct pstat* ps)
{

  if (ps == NULL)
    return -1;

  struct proc *p;
  int i = 0;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->state == UNUSED) {
      ps->inuse[i] = 0;
    }
    else {
      ps->inuse[i] = 1;
    }
    ps->pid[i] = p->pid;
    ps->priority[i] = p->priority;
    ps->state[i] = p->state;
    int j;
    for(j = 0; j < 4; j++) {
      ps->ticks[i][j] = p->ticks[j];
      ps->qtail[i][j] = p->qtail[j];
    }
    i++;
  }
  release(&ptable.lock);
  return 0;
}

int
setpri(int pid, int pri)
{ 
  if(pid<=0){
    return -1;
  }
  if(pri>3||pri<0){
    return -1;
  }
  struct proc *p;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->pid == pid) {

        if(pri==3){
          c3++;
          pri3[c3]=p;
          p->qtail[3]++;
        }else if(pri==2){
          c2++;
          pri2[c2]=p;
          p->qtail[2]++;
        }else if(pri==1){
          c1++;
          pri1[c1]=p;
          p->qtail[1]++;
        }else{
          c0++;
          pri0[c0]=p;
          p->qtail[0]++;
        }

        if(p->priority==3){
          int i;
          for(i=0;i<=c3;i++){
            if(pri3[i]==p){
              break;
            }
          }
          for(int j=i;j<c3;j++){
            pri3[j]=pri3[j+1];
          }
          pri3[c3]=NULL;
          c3--;
        }else if(p->priority==2){
          int i;
          for(i=0;i<=c2;i++){
            if(pri2[i]==p){
              break;
            }
          }
          for(int j=i;j<c2;j++){
            pri2[j]=pri2[j+1];
          }
          pri2[c2]=NULL;
          c2--;
        }else if(p->priority==1){
          int i;
          for(i=0;i<=c1;i++){
            if(pri1[i]==p){
              break;
            }
          }
          for(int j=i;j<c1;j++){
            pri1[j]=pri1[j+1]; 
          }
          pri1[c1]=NULL;
          c1--;
        }else if(p->priority==0){
          int i;
          for(i=0;i<=c0;i++){
            if(pri0[i]==p){
              break;
            }
          }
          for(int j=i;j<c0;j++){
            pri0[j]=pri0[j+1];
          }
          pri0[c0]=NULL;
          c0--;
        }
      p->priority=pri;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

int
getpri(int pid)
{
  if(pid<=0){
    return -1;
  }
  struct proc *p;
  int rtn = -1;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->pid == pid) {
      rtn = p->priority;
      break;
    }
  }
  release(&ptable.lock);
  return rtn;
}