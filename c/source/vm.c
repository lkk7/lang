#include "vm.h"

#include <stdarg.h>
#include <stdio.h>

#include "bytecode.h"
#include "common.h"
#include "compile.h"
#include "debug.h"
#include "value.h"

VM vm;

static void reset_stack(void) { vm.top = vm.stack; }

static void runtime_error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  (void)vfprintf(stderr, format, args);
  va_end(args);
  (void)fputs("\n", stderr);

  size_t instruction = (size_t)(vm.ip - vm.seq->code - 1);
  int line = vm.seq->lines[instruction];
  (void)fprintf(stderr, "[line %d] in script\n", line);
  reset_stack();
}

static Value peek(int distance) { return vm.top[-1 - distance]; }

static bool is_falsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

void init_vm(void) { reset_stack(); }

void free_vm(void) {}

#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.seq->consts.vals[READ_BYTE()])
#define BINARY_OP(value_type, op)                     \
  do {                                                \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
      runtime_error("Operands must be numbers.");     \
      return INTERPRET_RUNTIME_ERROR;                 \
    }                                                 \
    double b = AS_NUMBER(pop());                      \
    double a = AS_NUMBER(pop());                      \
    push(value_type(a op b));                         \
  } while (false)

static InterpretResult run(void) {
  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value* val = vm.stack; val < vm.top; val++) {
      printf("[ ");
      print_val(*val);
      printf(" ]");
    }
    printf("\n");
    disassemble_instr(vm.seq, (int)(vm.ip - vm.seq->code));
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
      case OP_ADD:
        BINARY_OP(NUMBER_VAL, +);
        break;
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
      case OP_RETURN: {
        print_val(pop());
        printf("\n");
        return INTERPRET_OK;
      }
    }
  }
}

InterpretResult interpret(const char* src) {
  ByteSequence seq;
  init_bsequence(&seq);

  if (!compile(src, &seq)) {
    free_bsequence(&seq);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.seq = &seq;
  vm.ip = seq.code;
  InterpretResult result = run();

  free_bsequence(&seq);
  return result;
}

void push(Value val) {
  *vm.top = val;
  ++vm.top;
}

Value pop(void) {
  --vm.top;
  return *vm.top;
}
