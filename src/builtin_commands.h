#ifndef MYSH_BUILTIN_COMMANDS_H
#define MYSH_BUILTIN_COMMANDS_H
#include "string_helpers.h"

#include <dirent.h>

char* findExecutable(const char* command);

StrVec wildcardExpand(const char* pattern, DIR* dir);

extern const char* paths_to_search[];

enum MYSHCommandType {
  MYSH_CD,
  MYSH_PWD,
  MYSH_WHICH,
  MYSH_EXIT,
  MYSH_EXECUTABLE,
};

enum MYSHCommandType commandToEnum(const char* command);

int cd(const char* path);

int pwd(void);

int which(const char* command);

#endif // MYSH_BUILTIN_COMMANDS_H
