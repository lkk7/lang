#pragma once

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_POP,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_GET_GLOBAL,
  OP_DEFINE_GLOBAL,
  OP_SET_GLOBAL,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_PRINT,
  OP_RETURN,
} OpCode;

typedef struct {
  int size;
  int capacity;
  uint8_t *code;
  int *lines;
  ValueArray consts;
} ByteSequence;

void init_bsequence(ByteSequence *seq);
void write_bsequence(ByteSequence *seq, uint8_t byte, int line);
void free_bsequence(ByteSequence *seq);
int add_const(ByteSequence *seq, Value val);
