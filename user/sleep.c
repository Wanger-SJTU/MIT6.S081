// #include "kernel/stat.h"
#include "kernel/types.h"
#include "user/user.h"

void main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(1, "Usage: sleep num_seconds...\n");
    exit(1);
  }
  int num = atoi(argv[1]);
  sleep(num);
  exit(0);
}
