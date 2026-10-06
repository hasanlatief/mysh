#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/wait.h>

#include "../src/process_dispatch.h"

static void test_dispatch(void) {
  char* msg = strdup("hello, world!");
  const size_t msg_len = strlen(msg);
  Subprocess subprocess = {.executable = "/usr/bin/echo", .args = (char*[]){"echo",msg, NULL}};
  fd pipe_fd[2];
  pipe(pipe_fd);
  dispatch(&subprocess, STDIN_FILENO, pipe_fd[PIPE_WRITE]);
  char output[msg_len + 2];
  ssize_t bytes = read(pipe_fd[PIPE_READ], output, msg_len + 2);
  assert(bytes == msg_len + 1);
  output[bytes] = '\0';
  assert(strcmp(output, "hello, world!\n") == 0);
  wait(NULL);
  free(msg);
}

static void test_pipeline(void) {
  Subprocess first = {.executable = "/usr/bin/cat", .args = (char*[]){"cat", "../../test/pipeline_test.txt", NULL}};
  Subprocess second = {.executable = "/usr/bin/sort", (char*[]){"sort", NULL}};
  Subprocess third = {.executable = "/usr/bin/uniq", (char*[]){"uniq", NULL}};
  Pipeline pipeline_third = {.subprocess = third, .next = NULL};
  Pipeline pipeline_second = {.subprocess = second, .next = &pipeline_third};
  Pipeline pipeline_first = {.subprocess = first, .next = &pipeline_second};
  fd pipe_fd[2];
  pipe(pipe_fd);
  pid_t pids[3];
  createPipeline(&pipeline_first, STDIN_FILENO, pipe_fd[PIPE_WRITE], pids, 0);
  char output[7];
  ssize_t bytes = read(pipe_fd[PIPE_READ], output, 6);
  assert(bytes == 6);
  output[bytes] = '\0';
  assert(strcmp(output, "a\nb\nc\n") == 0);
  while (waitpid(-1, NULL, 0) > 0) {
    // wait for all children
  }
}
int main(void) {
  test_dispatch();
  test_pipeline();
}
