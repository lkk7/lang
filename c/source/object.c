#include "object.h"

#include <stdio.h>
#include <string.h>

#include "memory.h"
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

static ObjStr* allocate_str(char* chars, int length) {
  ObjStr* string = ALLOCATE_OBJ(ObjStr, OBJ_STR);
  string->length = length;
  string->chars = chars;
  return string;
}

ObjStr* take_str(char* chars, int length) {
  return allocate_str(chars, length);
}

ObjStr* copy_str(const char* chars, int length) {
  char* heap_chars = ALLOCATE(char, length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';
  return allocate_str(heap_chars, length);
}

void print_obj(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STR:
      printf("%s", AS_CSTR(value));
      break;
  }
}
