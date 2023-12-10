#include "memory.h"

#include <stdlib.h>

#include "bytecode.h"
#include "object.h"
#include "vm.h"

void* reallocate(void* ptr, size_t old_size, size_t new_size) {
  (void)old_size;
  if (new_size == 0) {
    free(ptr);
    return NULL;
  }

  void* memory = realloc(ptr, new_size);
  if (memory == NULL) {
    exit(1);
  }
  return memory;
}

static void free_object(Obj* object) {
  switch (object->type) {
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      free_bsequence(&function->bseq);
      FREE(ObjFunction, object);
      break;
    }
    case OBJ_STR: {
      ObjStr* string = (ObjStr*)object;
      FREE_ARR(char, string->chars, string->length + 1);
      FREE(ObjStr, object);
      break;
    }
    case OBJ_NATIVE:
      FREE(ObjNative, object);
      break;
  }
}

void free_objects(void) {
  Obj* obj = vm.objects;
  while (obj != NULL) {
    Obj* next = obj->next;
    free_object(obj);
    obj = next;
  }
}
