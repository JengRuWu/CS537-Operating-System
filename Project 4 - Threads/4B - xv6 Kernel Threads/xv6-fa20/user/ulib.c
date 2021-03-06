#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"
#define PGSIZE 4096

void* userStack[64];
int threadID[64];

char*
strcpy(char *s, char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, void *vsrc, int n)
{
  char *dst, *src;
  
  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}

int 
thread_create(void (*start_routine)(void *), void *arg)
{
  void *stack, *stackfree;
  stackfree = malloc(2*PGSIZE);
  if(stackfree == 0){
    return -1;
  }
  //we want memory in pages
  uint offset = (uint)stackfree % PGSIZE;
  stack = stackfree + PGSIZE - offset;
  int tid = clone(start_routine, arg, stack);
  if (tid != -1) {
    for(int i=0; i<64; i++ ) {
      if(userStack[i] == NULL) {
        userStack[i] = stack;
        threadID[i] = tid;
        break;
      }
    }
  }
  else {
    free(stack);
  }
  return tid;
}

int thread_join()
{
  void *stack;
  int tid;
  tid = join(&stack);
  if (tid != -1) {
    for(int i=0; i<64; i++) {
      if(threadID[i] == tid) {
        free(userStack[i]);
        threadID[i] = -1;
        userStack[i] = NULL;
      }
    }
  }
  return tid;
}

void
lock_init(lock_t *lk)
{
  lk->locked = 0;
}

void
lock_acquire(lock_t *lk)
{
  while(xchg(&lk->locked, 1) != 0)
    ;
}

void
lock_release(lock_t *lk)
{
   xchg(&lk->locked, 0);
}

void
cond_init(cond_t *cv)
{
  cvinit(cv);
}
void
cond_wait(cond_t *cv, lock_t *lk)
{ 
  tsleep(cv,lk);
}
void
cond_signal(cond_t *cv)
{
  twake(cv);
}