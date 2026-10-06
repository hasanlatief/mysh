#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "string_helpers.h"

#include <stdlib.h>

bool startsWith(const char* str, const char* prefix) {
  assert(str && prefix);
  return strncmp(str, prefix, strlen(prefix)) == 0;
}

bool endsWith(const char* str, const char* suffix) {
  assert(str && suffix);
  size_t len = strlen(str);
  size_t suffix_len = strlen(suffix);
  return strncmp(str + len - suffix_len, suffix, suffix_len) == 0;
}

static void StrVecRealloc(StrVec* vec) {
  if (vec->size == vec->capacity) {
    vec->capacity *= 2;
    vec->data = realloc(vec->data, vec->capacity * sizeof(char*));
  }
}

void StrVec_Push(StrVec* vec, const char* str) {
  StrVecRealloc(vec);
  vec->data[vec->size++] = strdup(str);
}

void StrVec_Init(StrVec* vec) {
  vec->size = 0;
  vec->capacity = 4;
  vec->data = malloc(vec->capacity * sizeof(char*));
  assert(vec->data);
}

void StrVec_Destroy(StrVec* vec) {
  if (vec->data) {
    free(vec->data);
  }
}

void StrVec_nullTerminate(StrVec* vec) {
  vec->data = realloc(vec->data, (vec->size + 1) * sizeof(char*));
  vec->data[vec->size] = NULL;
}

void StrVec_freeAll(StrVec* vec) {
  for (size_t i = 0; i < vec->size; i++) {
    free(vec->data[i]);
  }
  StrVec_Destroy(vec);
}

void free_tokens(char** tokens, int count) {
  for (int i = 0; i < count; ++i) {
    free(tokens[i]);
  }
}