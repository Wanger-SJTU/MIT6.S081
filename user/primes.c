
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

typedef int pid_t;
int main(int argc, char *argv[]) {
  int fd[2];
  int numbers[34];
  int index = 0;
  pid_t pid;

  for (int i = 0; i < 34; i++) {
    numbers[i] = i + 2;
    index++;
  }
  while (index > 0) {
    pipe(fd);
    pid = fork();
    if (pid < 0) {
      fprintf(2, "fork error\n");
      exit(-1);
    } else if (pid > 0) {
      close(fd[0]);  // close read
      for (int i = 0; i < index; ++i) {
        write(fd[1], &numbers[i], sizeof(numbers[i]));
      }
      close(fd[1]);
      wait((int *)0);
      exit(0);
    } else {
      close(fd[1]);
      int prime = 0;
      int temp = 0;
      index = -1;

      while (read(fd[0], &temp, sizeof(temp)) != 0) {
        // the first number must be prime
        if (index < 0)
          prime = temp, index++;
        else {
          if (temp % prime != 0) {
            numbers[index++] = temp;
          }
        }
      }
      printf("prime %d\n", prime);
      // fork again until no prime
      close(fd[0]);
    }
  }
  exit(0);
}