#ifndef _TYPES_H_
#define _TYPES_H_

// Type definitions



typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;

typedef struct {
  uint locked;       // Is the lock held?
}lock_t;

typedef struct {
  uint locked;       // Is the lock held?
  struct proc* threadQueue[64];
  int front;
  int rear;
}cond_t;
#ifndef NULL
#define NULL (0)
#endif

#endif //_TYPES_H_
