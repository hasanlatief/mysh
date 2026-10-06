#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include "../src/builtin_commands.h"
#include "../src/string_helpers.h"

static void assertVecContains(const StrVec* vec, const char* str) {
  for (size_t i = 0; i < vec->size; i++) {
    if (strcmp(vec->data[i], str) == 0) {
      return;
    }
  }
  fprintf(stderr, "%s\n", str);
  assert(!"Expected string not found in vector");
}

static void testBar(DIR* dir) {
  StrVec expanded = wildcardExpand("bar*", dir);
  assert(expanded.size == 1);
  assertVecContains(&expanded, "bar1");
}

static void testFoo(DIR* dir) {
  StrVec expanded = wildcardExpand("f*1", dir);
  assert(expanded.size == 3);
  assert(strcmp(expanded.data[0], "foo1") == 0);
  assert(strcmp(expanded.data[1], "foobar1") == 0);
  assert(strcmp(expanded.data[2], "foobarbaz1") == 0);
}

int main(void) {
  DIR* dir = opendir("../../test/wildcard_test");
  assert(dir);
  testBar(dir);
  rewinddir(dir);
  testFoo(dir);
}
