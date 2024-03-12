#include "memory.h"

#include <stdlib.h>

#include "bytecode.h"
#include "compile.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>

#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(void* ptr, size_t old_size, size_t new_size) {
  vm.bytes_allocated += new_size - old_size;
  if (new_size > old_size) {
#ifdef DEBUG_STRESS_GC
    collect_garbage();
#endif
    if (vm.bytes_allocated > vm.next_gc) {
      collect_garbage();
    }
  }
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

void mark_object(Obj* object) {
  if (object == NULL) {
    return;
  }
  if (object->is_marked) {
    return;
  }
#ifdef DEBUG_LOG_GC
  printf("%p marking ", (void*)object);
  print_val(OBJ_VAL(object));
  printf("\n");
#endif
  object->is_marked = true;

  if (vm.gray_capacity < vm.gray_cnt + 1) {
    vm.gray_capacity = GROW_CAPACITY(vm.gray_capacity);
    vm.gray_stack =
        (Obj**)realloc(vm.gray_stack, sizeof(Obj*) * vm.gray_capacity);
    if (vm.gray_stack == NULL) {
      exit(1);
    }
  }
  vm.gray_stack[vm.gray_cnt++] = object;
}

void mark_value(Value value) {
  if (IS_OBJ(value)) {
    mark_object(AS_OBJ(value));
  }
}

static void mark_arr(ValueArray* array) {
  for (int i = 0; i < array->size; i++) {
    mark_value(array->vals[i]);
  }
}

static void blacken_object(Obj* object) {
#ifdef DEBUG_LOG_GC
  printf("%p blackening ", (void*)object);
  print_val(OBJ_VAL(object));
  printf("\n");
#endif
  switch (object->type) {
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      mark_object((Obj*)closure->function);
      for (int i = 0; i < closure->upvalue_cnt; i++) {
        mark_object((Obj*)closure->upvalues[i]);
      }
      break;
    }
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      mark_object((Obj*)function->name);
      mark_arr(&function->bseq.consts);
      break;
    }
    case OBJ_UPVALUE:
      mark_value(((ObjUpvalue*)object)->closed);
      break;
    case OBJ_NATIVE:
    case OBJ_STR:
      break;
  }
}

static void free_object(Obj* object) {
#ifdef DEBUG_LOG_GC
  printf("%p freeing type %d\n", (void*)object, object->type);
#endif
  switch (object->type) {
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      FREE_ARR(ObjUpvalue*, closure->upvalues, closure->upvalue_cnt);
      FREE(ObjClosure, object);
      break;
    }
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
    case OBJ_UPVALUE:
      FREE(ObjUpvalue, object);
      break;
  }
}

static void mark_roots(void) {
  for (Value* slot = vm.stack; slot < vm.top; slot++) {
    mark_value(*slot);
  }
  for (int i = 0; i < vm.frame_count; i++) {
    mark_object((Obj*)vm.frames[i].closure);
  }
  for (ObjUpvalue* up = vm.open_upvalues; up != NULL; up = up->next) {
    mark_object((Obj*)up);
  }
  mark_table(&vm.globals);
  mark_compiler_roots();
}

static void trace_references() {
  while (vm.gray_cnt > 0) {
    Obj* object = vm.gray_stack[--vm.gray_cnt];
    blacken_object(object);
  }
}

static void sweep(void) {
  Obj* previous = NULL;
  Obj* object = vm.objects;
  while (object != NULL) {
    if (object->is_marked) {
      object->is_marked = false;
      previous = object;
      object = object->next;
    } else {
      Obj* unreached = object;
      object = object->next;
      if (previous != NULL) {
        previous->next = object;
      } else {
        vm.objects = object;
      }
      free_object(unreached);
    }
  }
}

void collect_garbage() {
#ifdef DEBUG_LOG_GC
  printf("-- gc begin\n");
  size_t before = vm.bytes_allocated;
#endif

  mark_roots();
  trace_references();
  table_remove_white(&vm.strings);
  sweep();

  vm.next_gc = vm.bytes_allocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
  printf("-- gc end\n");
  printf("   collected %zu bytes (from %zu to %zu), next at %zu\n",
         before - vm.bytes_allocated, before, vm.bytes_allocated, vm.next_gc);
#endif
}

void free_objects(void) {
  Obj* obj = vm.objects;
  while (obj != NULL) {
    Obj* next = obj->next;
    free_object(obj);
    obj = next;
  }
  free(vm.gray_stack);
}
