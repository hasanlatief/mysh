#include "process_dispatch.h"
#include "builtin_commands.h"

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

static void redirect(fd old_fd, fd new_fd) {
  if (old_fd != new_fd) {
    dup2(old_fd, new_fd);
    close(old_fd);
  }
}

pid_t dispatch(const Subprocess* subprocess, fd in, fd out) {
  assert(subprocess);
  enum MYSHCommandType type = commandToEnum(subprocess->executable);
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return -1;
  }
  if (pid == CHILD) {
    redirect(in, STDIN_FILENO);
    redirect(out, STDOUT_FILENO);
    if (type == MYSH_PWD) {
      exit(pwd());
    }
    if (type == MYSH_WHICH) {
      if (subprocess->args[1] == NULL) {
        exit(1);
      }
      exit(which(subprocess->args[1]));
    }
    if (type == MYSH_CD || type == MYSH_EXIT) {
      exit(0);
    }
    execv(subprocess->executable, subprocess->args);
    perror("exec");
    exit(1);
  }
  return pid;
}

int createPipeline(const Pipeline* head, fd in, fd out, pid_t* pids, int index) {
  if (!head) {
    return index;
  }
  if (!head->next) {
    pids[index] = dispatch(&head->subprocess, in, out);
    return index + 1;
  }
  fd pipe_fd[2];
  if (pipe(pipe_fd) < 0) {
    perror("pipe");
    return -1;
  }
  pids[index] = dispatch(&head->subprocess, in, pipe_fd[PIPE_WRITE]);
  close(pipe_fd[PIPE_WRITE]);
  if (in != STDIN_FILENO) {
    close(in);
  }
  int result = createPipeline(head->next, pipe_fd[PIPE_READ], out, pids, index + 1);
  close(pipe_fd[PIPE_READ]);
  return result;
}

int executePipeline(const Pipeline* head, fd in, fd out, bool is_interactive) {
  int count = 0;
  const Pipeline* node = head;
  while (node) {
    ++count;
    node = node->next;
  }
  pid_t pids[256];
  int num_procs = createPipeline(head, in, out, pids, 0);
  if (num_procs < 0) {
    return 1;
  }
  int last_status = 0;
  for (int c = 0; c < num_procs; ++c) {
    if (pids[c] <= 0) {
      continue;
    }
    int exit_code = waitAndPrintExitStatus(pids[c], (c == num_procs - 1) && is_interactive);
    if (c == num_procs - 1) {
      last_status = exit_code;
    }
  }
  return last_status;
}

int waitAndPrintExitStatus(pid_t pid, bool is_interactive) {
  int status;
  waitpid(pid, &status, 0);
  if (WIFEXITED(status)) {
    int code = WEXITSTATUS(status);
    if (code != 0 && is_interactive) {
      printf("Exited with status %d\n", code);
    }
    return code;
  }
  if (WIFSIGNALED(status)) {
    if (is_interactive) {
      psignal(WTERMSIG(status), "Terminated by signal");
    }
    return 1;
  }
  return 1;
}