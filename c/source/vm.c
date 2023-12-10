#include "vm.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "bytecode.h"
#include "common.h"
#include "compile.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

VM vm;

static Value clock_native(int argc, Value* args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static void reset_stack(void) {
  vm.top = vm.stack;
  vm.frame_count = 0;
}

static void runtime_error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  (void)vfprintf(stderr, format, args);
  va_end(args);
  (void)fputs("\n", stderr);

  for (int i = vm.frame_count - 1; i >= 0; i--) {
    CallFrame* frame = &vm.frames[i];
    ObjFunction* function = frame->function;
    size_t instruction = frame->ip - function->bseq.code - 1;
    (void)fprintf(stderr, "[line %d] in ", function->bseq.lines[instruction]);
    if (function->name == NULL) {
      (void)fprintf(stderr, "script\n");
    } else {
      (void)fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  reset_stack();
}

static void define_native(const char* name, NativeFn function) {
  push(OBJ_VAL(copy_str(name, (int)strlen(name))));
  push(OBJ_VAL(new_native(function)));
  table_set(&vm.globals, AS_STR(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

static Value peek(int distance) { return vm.top[-1 - distance]; }

static bool call(ObjFunction* function, int argCount) {
  if (argCount != function->arity) {
    runtime_error("Expected %d arguments but got %d", function->arity,
                  argCount);
    return false;
  }
  if (vm.frame_count == FRAMES_MAX) {
    runtime_error("Stack overflow");
    return false;
  }

  CallFrame* frame = &vm.frames[vm.frame_count++];
  frame->function = function;
  frame->ip = function->bseq.code;
  frame->slots = vm.top - argCount - 1;
  return true;
}

static bool call_value(Value callee, int argCount) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_FUNCTION:
        return call(AS_FUNCTION(callee), argCount);
      case OBJ_NATIVE: {
        NativeFn native = AS_NATIVE(callee);
        Value result = native(argCount, vm.top - argCount);
        vm.top -= argCount + 1;
        push(result);
        return true;
      }
      default:
        break;
    }
  }
  runtime_error("Can only call functions and classes");
  return false;
}

static bool is_falsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concat_str(void) {
  ObjStr* b = AS_STR(pop());
  ObjStr* a = AS_STR(pop());

  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjStr* result = take_str(chars, length);
  push(OBJ_VAL(result));
}

void init_vm(void) {
  reset_stack();
  vm.objects = NULL;
  init_table(&vm.globals);
  init_table(&vm.strings);

  define_native("clock", clock_native);
}

void free_vm(void) {
  free_table(&vm.strings);
  free_table(&vm.globals);
  free_objects();
}

#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT() (frame->function->bseq.consts.vals[READ_BYTE()])
#define READ_SHORT() \
  (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_STR() AS_STR(READ_CONSTANT())
#define BINARY_OP(value_type, op)                     \
  do {                                                \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
      runtime_error("Operands must be numbers");      \
      return INTERPRET_RUNTIME_ERROR;                 \
    }                                                 \
    double b = AS_NUMBER(pop());                      \
    double a = AS_NUMBER(pop());                      \
    push(value_type(a op b));                         \
  } while (false)

static InterpretResult run(void) {
  CallFrame* frame = &vm.frames[vm.frame_count - 1];
#ifdef DEBUG_TRACE_EXECUTION
  printf("--- execution ---");
#endif
  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value* val = vm.stack; val < vm.top; val++) {
      printf("[ ");
      print_val(*val);
      printf(" ]");
    }
    printf("\n");
    disassemble_instr(&frame->function->bseq,
                      (int)(frame->ip - frame->function->bseq.code));
#endif
    uint8_t instr;
    switch (instr = READ_BYTE()) {
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_NIL:
        push(NIL_VAL);
        break;
      case OP_TRUE:
        push(BOOL_VAL(true));
        break;
      case OP_FALSE:
        push(BOOL_VAL(false));
        break;
      case OP_POP:
        pop();
        break;
      case OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
        push(frame->slots[slot]);
        break;
      }
      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        frame->slots[slot] = peek(0);
        break;
      }
      case OP_GET_GLOBAL: {
        ObjStr* name = READ_STR();
        Value value;
        if (!table_get(&vm.globals, name, &value)) {
          runtime_error("Undefined variable '%s'", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }
      case OP_DEFINE_GLOBAL: {
        ObjStr* name = READ_STR();
        table_set(&vm.globals, name, peek(0));
        pop();
        break;
      }
      case OP_SET_GLOBAL: {
        ObjStr* name = READ_STR();
        if (table_set(&vm.globals, name, peek(0))) {
          table_delete(&vm.globals, name);
          runtime_error("Undefined variable '%s'", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(are_equal(a, b)));
        break;
      }
      case OP_GREATER:
        BINARY_OP(BOOL_VAL, >);
        break;
      case OP_LESS:
        BINARY_OP(BOOL_VAL, <);
        break;
      case OP_ADD: {
        if (IS_STR(peek(0)) && IS_STR(peek(1))) {
          concat_str();
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(a + b));
        } else {
          runtime_error("Operands must be two numbers or strings");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_SUBTRACT:
        BINARY_OP(NUMBER_VAL, -);
        break;
      case OP_MULTIPLY:
        BINARY_OP(NUMBER_VAL, *);
        break;
      case OP_DIVIDE:
        BINARY_OP(NUMBER_VAL, /);
        break;
      case OP_NOT:
        push(BOOL_VAL(is_falsey(pop())));
        break;
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          runtime_error("Operand must be a number");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(NUMBER_VAL(-AS_NUMBER(pop())));
        break;
      case OP_PRINT: {
        print_val(pop());
        printf("\n");
        break;
      }
      case OP_JUMP: {
        uint16_t offset = READ_SHORT();
        frame->ip += offset;
        break;
      }
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        if (is_falsey(peek(0))) {
          frame->ip += offset;
        }
        break;
      }
      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
        frame->ip -= offset;
        break;
      }
      case OP_CALL: {
        int arg_count = READ_BYTE();
        if (!call_value(peek(arg_count), arg_count)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frame_count - 1];
        break;
      }
      case OP_RETURN: {
        Value result = pop();
        vm.frame_count--;
        if (vm.frame_count == 0) {
          pop();
          return INTERPRET_OK;
        }
        vm.top = frame->slots;
        push(result);
        frame = &vm.frames[vm.frame_count - 1];
        break;
      }
    }
  }
}

InterpretResult interpret(const char* src) {
  ObjFunction* function = compile(src);
  if (function == NULL) {
    return INTERPRET_COMPILE_ERROR;
  }

  push(OBJ_VAL(function));
  call(function, 0);

  return run();
}

void push(Value val) {
  *vm.top = val;
  ++vm.top;
}

Value pop(void) {
  --vm.top;
  return *vm.top;
}
