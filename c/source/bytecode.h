#pragma once

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NEGATE,
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
