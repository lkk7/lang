#include "debug.h"

#include <stdio.h>

#include "bytecode.h"
#include "value.h"

void disassemble_bseq(ByteSequence *seq, const char *name) {
  printf("- %s -\n", name);

  for (int offset = 0; offset < seq->size;) {
    offset = disassemble_instr(seq, offset);
  }
}

static int simple_instr(const char *name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

static int const_instr(const char *name, ByteSequence *seq, int offset) {
  uint8_t const_offset = seq->code[offset + 1];
  printf("%-16s %4d '", name, const_offset);
  print_val(seq->consts.vals[const_offset]);
  printf("'\n");
  return offset + 2;
}

int disassemble_instr(ByteSequence *seq, int offset) {
  printf("%04d ", offset);
  if (offset > 0 && seq->lines[offset] == seq->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", seq->lines[offset]);
  }

  uint8_t instr = seq->code[offset];
  switch (instr) {
    case OP_RETURN:
      return simple_instr("OP_RETURN", offset);
    case OP_ADD:
      return simple_instr("OP_ADD", offset);
    case OP_SUBTRACT:
      return simple_instr("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
      return simple_instr("OP_MULTIPLY", offset);
    case OP_DIVIDE:
      return simple_instr("OP_DIVIDE", offset);
    case OP_NEGATE:
      return simple_instr("OP_NEGATE", offset);
    case OP_NOT:
      return simple_instr("OP_NOT", offset);
    case OP_CONSTANT:
      return const_instr("OP_CONSTANT", seq, offset);
    case OP_NIL:
      return simple_instr("OP_NIL", offset);
    case OP_TRUE:
      return simple_instr("OP_TRUE", offset);
    case OP_FALSE:
      return simple_instr("OP_FALSE", offset);
    case OP_EQUAL:
      return simple_instr("OP_EQUAL", offset);
    case OP_GREATER:
      return simple_instr("OP_GREATER", offset);
    case OP_LESS:
      return simple_instr("OP_LESS", offset);
    default:
      printf("Unknown opcode %d\n", instr);
      return offset + 1;
  }
}
