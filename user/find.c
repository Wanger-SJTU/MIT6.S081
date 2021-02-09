
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

const char curDir = '.';
const char parentDir[3] = "..\0";

const int BUFFER_SIZE = 512;

void strcat(char *str1, char *str2) {
  int str1len = strlen(str1);
  int str2len = strlen(str2);
  if (str1len + str2len > BUFFER_SIZE) {
    return;
  }
  str1[str1len + str2len] = 0;
  while (str2len-- > 0) {
    str1[str1len + str2len] = str2[str2len];
  }
}

void find(char *path, char *name) {
  char buf[BUFFER_SIZE], *p;
  int fd;
  struct dirent de;
  struct stat st;
  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch (st.type) {
    case T_FILE:
      if (strcmp(path, name) == 0) {
        printf("%s\n", path);
      }
      break;
    case T_DIR:
      if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
        printf("find: path too long\n");
        break;
      }
      strcpy(buf, path);
      // strcat(buf, path);
      p = buf + strlen(buf);
      *p++ = '/';
      while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0 || strcmp(de.name, &curDir) == 0 || strcmp(de.name, parentDir) == 0) {
          continue;
        }
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if (stat(buf, &st) < 0) {
          printf("find: cannot stat %s\n", buf);
          continue;
        }
        if (strcmp(de.name, name) == 0) {
          printf("%s\n", buf);
        }
        find(buf, name);
      }
      break;
  }
  close(fd);
}
int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(2, "find ");
  }
  find(argv[1], argv[2]);
  exit(0);
}