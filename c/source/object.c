#include "object.h"

#include <stdio.h>
#include <string.h>

#include "bytecode.h"
#include "memory.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, obj_type) \
  (type*)allocate_object(sizeof(type), obj_type)

static Obj* allocate_object(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;
  object->is_marked = false;
  object->next = vm.objects;
  vm.objects = object;

#ifdef DEBUG_LOG_GC
  printf("%p allocating %zu for %d\n", (void*)object, size, type);
#endif

  return object;
}

ObjClosure* new_closure(ObjFunction* function) {
  ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalue_cnt);
  for (int i = 0; i < function->upvalue_cnt; i++) {
    upvalues[i] = NULL;
  }
  ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalue_cnt = function->upvalue_cnt;
  return closure;
}

ObjFunction* new_function() {
  ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
  function->arity = 0;
  function->upvalue_cnt = 0;
  function->name = NULL;
  init_bsequence(&function->bseq);
  return function;
}

ObjNative* new_native(NativeFn function) {
  ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}

static ObjStr* allocate_str(char* chars, int length, uint32_t hash) {
  ObjStr* string = ALLOCATE_OBJ(ObjStr, OBJ_STR);
  string->length = length;
  string->chars = chars;
  string->hash = hash;
  push(OBJ_VAL(string));
  table_set(&vm.strings, string, NIL_VAL);
  pop();
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

ObjUpvalue* new_upvalue(Value* slot) {
  ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->location = slot;
  upvalue->next = NULL;
  return upvalue;
}

static void print_function(ObjFunction* f) {
  if (f->name == NULL) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", f->name->chars);
}

void print_obj(Value val) {
  switch (OBJ_TYPE(val)) {
    case OBJ_STR:
      printf("%s", AS_CSTR(val));
      break;
    case OBJ_CLOSURE:
      print_function(AS_CLOSURE(val)->function);
      break;
    case OBJ_FUNCTION:
      print_function(AS_FUNCTION(val));
      break;
    case OBJ_NATIVE:
      printf("<native fn>");
      break;
  }
}
