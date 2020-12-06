#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int i = getwritecount();
  printf(1,"Write() has been invoked %d times.\n", i);
  exit();
}
