#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"
#define check(exp, msg) if(exp) {} else {\
  printf(1, "%s:%d check (" #exp ") failed: %s\n", __FILE__, __LINE__, msg);}

void busywork() {
  int i = 0;
  for (int j=0; j<10000000;j++)
    ++i;
}

int
main(int argc, char *argv[])
{
  int pid1 = fork();
  if (!pid1) {
    busywork(); // child is in pri 3, so it should never be preempted
    exit();
  }

  int pid2 = fork();
  if (!pid2) {
    busywork();
    exit();
  }

  int pid3 = fork();
  if (!pid3) {
    busywork();
    exit();
  }

   int pid4 = fork();
  if (!pid4) {
    busywork();
    exit();
  }

  check(!setpri(pid2, 2), "setpri() returns nonzero code");
  check(!setpri(pid3, 1), "setpri() returns nonzero code");
  check(!setpri(pid4, 0), "setpri() returns nonzero code");
  // child 1 should never run on priority 2
  // because child 2 is always running

  struct pstat st;
  int i, j, k;
  for (i = 0; i < 10; ++i) { // repeat 10 times
    sleep(10);
    check(getpinfo(&st) == 0, "getpinfo");
    for (j = 0; j < NPROC; ++j) {
      if (st.inuse[j] && st.pid[j] == pid1 && st.priority[j] == 2)
        break;
    }
    check(j < NPROC, "cannot find child 1");
    for (k = 0; k < NPROC; ++k) {
      if (st.inuse[k] && st.pid[k] == pid2 && st.priority[k] == 3)
        break;
    }
    check(k < NPROC, "cannot find child 2");
  }

  kill(pid1);
  kill(pid2);
    kill(pid3);
  kill(pid4);

  printf(1, "TEST PASSED");
  exit();
}
