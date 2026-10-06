#include "builtin_commands.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

const char* paths_to_search[] = {"/usr/local/bin", "/usr/bin", "/bin", NULL};

char* findExecutable(const char* command) {
  if (command == NULL) {
    return NULL;
  }
  size_t name_len = strlen(command);
  for (int i = 0; paths_to_search[i] != NULL; i++) {
    size_t path_len = strlen(paths_to_search[i]);
    char* full_path = malloc(name_len + path_len + 2); // 1 for '/', 1 for '\0'
    strncpy(full_path, paths_to_search[i], path_len);
    strcat(full_path, "/");
    strncat(full_path, command, name_len);
    if (access(full_path, X_OK) == 0) {
      return full_path;
    }
    free(full_path);
  }
  return NULL;
}

static int strcmpSort(const void* lhs, const void* rhs) {
  return strcmp(*(const char**)lhs, *(const char**)rhs);
}

// Abandoned method for loop, used in tests
StrVec wildcardExpand(const char* pattern, DIR* dir) {
  assert(pattern);
  char* pattern_dup = strdup(pattern);
  size_t pattern_len = strlen(pattern_dup);
  char* wild_pos = memchr(pattern_dup, '*', pattern_len);
  StrVec matches;
  StrVec_Init(&matches);
  if (wild_pos == NULL) {
    free(pattern_dup);
    return matches;
  }
  *wild_pos = '\0';
  const char* prefix = pattern_dup;
  const char* suffix = wild_pos + 1;
  const struct dirent* entry;
  while ((entry = readdir(dir))) {
    const char* const d_name = entry->d_name;
    if (startsWith(d_name, prefix) && endsWith(d_name, suffix) && !startsWith(d_name, ".")) {
      StrVec_Push(&matches, d_name);
    }
  }
  qsort((void*)matches.data, matches.size, sizeof(char*), strcmpSort);
  return matches;
}

enum MYSHCommandType commandToEnum(const char* command) {
  if (strcmp(command, "cd") == 0) {
    return MYSH_CD;
  }
  if (strcmp(command, "pwd") == 0) {
    return MYSH_PWD;
  }
  if (strcmp(command, "which") == 0) {
    return MYSH_WHICH;
  }
  if (strcmp(command, "exit") == 0) {
    return MYSH_EXIT;
  }
  return MYSH_EXECUTABLE;
}

int cd(const char* path) {
  if (path == NULL) {
    path = getenv("HOME");
    if (path == NULL) {
      (void)fprintf(stderr, "HOME environment variable not set\n");
      return EXIT_FAILURE;
    }
  }
  int ret = chdir(path);
  if (ret == -1) {
    perror("cd");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int pwd(void) {
  char* path_name = getcwd(NULL, 0);
  if (path_name == NULL) {
    perror("getcwd");
    free(path_name);
    return EXIT_FAILURE;
  }
  printf("%s\n", path_name);
  free(path_name);
  return EXIT_SUCCESS;
}

int which(const char* executable_name) {
  if (commandToEnum(executable_name) != MYSH_EXECUTABLE) {
    return EXIT_FAILURE; // "which prints nothing" if given built-in
  }
  char* executable_path = findExecutable(executable_name);
  if (executable_path == NULL) {
    (void)fprintf(stderr, "No executable found with the name %s\n", executable_name);
    return EXIT_FAILURE;
  }
  printf("%s\n", executable_path);
  free(executable_path);
  return EXIT_SUCCESS;
}
