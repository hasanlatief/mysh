#ifndef MYSH_PROCESS_DISPATCH_H
#define MYSH_PROCESS_DISPATCH_H

#include <stdbool.h>
#include <sys/types.h>

typedef int fd;

#define CHILD 0
#define PIPE_READ 0
#define PIPE_WRITE 1

typedef struct {
  const char* executable;
  char** args;
} Subprocess;

typedef struct Pipeline {
  Subprocess subprocess;
  struct Pipeline* next;
} Pipeline;

int executePipeline(const Pipeline* head, fd in, fd out, bool is_interactive);

int waitAndPrintExitStatus(pid_t pid, bool is_interactive);

pid_t dispatch(const Subprocess* subprocess, fd in, fd out);

int createPipeline(const Pipeline* head, fd in, fd out, pid_t* pids, int index);

#endif // MYSH_PROCESS_DISPATCH_H