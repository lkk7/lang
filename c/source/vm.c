#include "vm.h"

#include <stdio.h>

#include "bytecode.h"
#include "common.h"
#include "debug.h"
#include "value.h"

VM vm;

static void reset_stack(void) { vm.top = vm.stack; }

void init_vm(void) { reset_stack(); }

void free_vm(void) {}

#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.seq->consts.vals[READ_BYTE()])
#define BINARY_OP(op) \
  do {                \
    double b = pop(); \
    double a = pop(); \
    push(a op b);     \
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
      case OP_ADD:
        BINARY_OP(+);
        break;
      case OP_SUBTRACT:
        BINARY_OP(-);
        break;
      case OP_MULTIPLY:
        BINARY_OP(*);
        break;
      case OP_DIVIDE:
        BINARY_OP(/);
        break;
      case OP_NEGATE:
        push(-pop());
        break;
      case OP_RETURN: {
        print_val(pop());
        printf("\n");
        return INTERPRET_OK;
      }
    }
  }
}

InterpretResult interpret(ByteSequence* seq) {
  vm.seq = seq;
  vm.ip = seq->code;
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
