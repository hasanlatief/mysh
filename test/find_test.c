#include "../src/builtin_commands.h"

#include <assert.h>
#include <string.h>

int main(void) {
  const char* ls_path = findExecutable("ls");
  assert(ls_path != NULL);
  assert(!strcmp(ls_path, "/usr/bin/ls"));
  const char* apt_path = findExecutable("apt");
  assert(apt_path != NULL);
  assert(!strcmp(apt_path,"/usr/local/bin/apt"));
  assert(!findExecutable("nonexistent_command"));
  return 0;
}
