#include "bytecode.h"

#include "memory.h"
#include "value.h"
#include "vm.h"

void init_bsequence(ByteSequence *seq) {
  seq->size = 0;
  seq->capacity = 0;
  seq->code = NULL;
  seq->lines = NULL;
  init_valarr(&seq->consts);
}

void write_bsequence(ByteSequence *seq, uint8_t byte, int line) {
  if (seq->capacity < seq->size + 1) {
    int capacity = seq->capacity;
    seq->capacity = GROW_CAPACITY(capacity);
    seq->code =
        GROW_ARR(uint8_t, seq->code, (size_t)capacity, (size_t)seq->capacity);
    seq->lines =
        GROW_ARR(int, seq->lines, (size_t)capacity, (size_t)seq->capacity);
  }
  seq->code[seq->size] = byte;
  seq->lines[seq->size] = line;
  ++(seq->size);
}

void free_bsequence(ByteSequence *seq) {
  FREE_ARR(uint8_t, seq->code, (size_t)seq->capacity);
  FREE_ARR(int, seq->lines, (size_t)seq->capacity);
  free_valarr(&seq->consts);
  init_bsequence(seq);
}

int add_const(ByteSequence *seq, Value val) {
  push(val);
  write_valarr(&seq->consts, val);
  pop();
  return seq->consts.size - 1;
}
