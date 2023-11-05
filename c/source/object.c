#include "object.h"

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, obj_type) \
  (type*)allocate_object(sizeof(type), obj_type)

static Obj* allocate_object(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;
  object->next = vm.objects;
  vm.objects = object;
  return object;
}

static ObjStr* allocate_str(char* chars, int length, uint32_t hash) {
  ObjStr* string = ALLOCATE_OBJ(ObjStr, OBJ_STR);
  string->length = length;
  string->chars = chars;
  string->hash = hash;
  table_set(&vm.strings, string, NIL_VAL);
  return string;
}

static uint32_t hash_str(const char* key, int length) {
  uint32_t hash = 2166136261U;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

ObjStr* take_str(char* chars, int length) {
  uint32_t hash = hash_str(chars, length);
  ObjStr* interned = table_find_str(&vm.strings, chars, length, hash);
  if (interned != NULL) {
    FREE_ARR(char, chars, length + 1);
    return interned;
  }
  return allocate_str(chars, length, hash);
}

ObjStr* copy_str(const char* chars, int length) {
  uint32_t hash = hash_str(chars, length);
  ObjStr* interned = table_find_str(&vm.strings, chars, length, hash);
  if (interned != NULL) {
    return interned;
  }
  char* heap_chars = ALLOCATE(char, length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';
  return allocate_str(heap_chars, length, hash);
}

void print_obj(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STR:
      printf("%s", AS_CSTR(value));
      break;
  }
}
