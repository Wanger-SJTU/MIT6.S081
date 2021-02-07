
// #include "kernel/stat.h"
#include "kernel/types.h"
#include "user/user.h"

void main (int argc, char *argv[])
{
  char c;
  int n;

  int child2parent[2];
  int parent2child[2];

  pipe (child2parent);
  pipe (parent2child);

  if (fork () == 0) {
    // child parent2child[1] refers write, child2parent[0] refers read
    close (parent2child[1]); //
    close (child2parent[0]);
    c = 'a';
    n = read (parent2child[0], &c, 1);
    if (n != 1) {
      fprintf (2, "child read error");
      exit (1);
    }
    printf ("%d: received ping\n", getpid ());
    write (child2parent[1], &c, 1);
    close (parent2child[0]);
    close (child2parent[1]);
    exit (0);
  } else {
    close (parent2child[0]);
    close (child2parent[1]);
    c = 'b';
    write (parent2child[1], &c, 1);
    n = read (child2parent[0], &c, 1);

    if (n != 1) {
      fprintf (2, "child read error");
      exit (1);
    }
    printf ("%d: received pong\n", getpid ());
    close (parent2child[1]);
    close (child2parent[0]);
    wait (0);
    exit (0);
  }
}