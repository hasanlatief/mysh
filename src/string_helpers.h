#ifndef MYSH_STRING_HELPERS_H
#define MYSH_STRING_HELPERS_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
  char** data;
  size_t size;
  size_t capacity;
} StrVec;

bool startsWith(const char* str, const char* prefix);

bool endsWith(const char* str, const char* suffix);

void StrVec_Push(StrVec* vec, const char* str);

void StrVec_Init(StrVec* vec);

void StrVec_Destroy(StrVec* vec);

void StrVec_nullTerminate(StrVec* vec);

void StrVec_freeAll(StrVec* vec);

void free_tokens(char** tokens, int count);

#endif //MYSH_STRING_HELPERS_H
