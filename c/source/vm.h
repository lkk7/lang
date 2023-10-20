#pragma once

#include "bytecode.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
  ByteSequence* seq;
  /** Instruction pointer */
  uint8_t* ip;
  Value stack[STACK_MAX];
  Value* top;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void init_vm(void);
void free_vm(void);
InterpretResult interpret(const char* src);
void push(Value val);
Value pop(void);
