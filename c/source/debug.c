#include "debug.h"

#include <stdio.h>

#include "bytecode.h"
#include "object.h"
#include "value.h"

void disassemble_bseq(ByteSequence *seq, const char *name) {
  printf("--- %s ---\n", name);

  for (int offset = 0; offset < seq->size;) {
    offset = disassemble_instr(seq, offset);
  }
}

static int simple_instr(const char *name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

static int byte_instr(const char *name, ByteSequence *seq, int offset) {
  uint8_t slot = seq->code[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2;
}

static int jump_instr(const char *name, int sign, ByteSequence *bseq,
                      int offset) {
  uint16_t jump = (uint16_t)(bseq->code[offset + 1] << 8);
  jump |= bseq->code[offset + 2];
  printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
  return offset + 3;
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
    case OP_PRINT:
      return simple_instr("OP_PRINT", offset);
    case OP_JUMP:
      return jump_instr("OP_JUMP", 1, seq, offset);
    case OP_JUMP_IF_FALSE:
      return jump_instr("OP_JUMP_IF_FALSE", 1, seq, offset);
    case OP_LOOP:
      return jump_instr("OP_LOOP", -1, seq, offset);
    case OP_CALL:
      return byte_instr("OP_CALL", seq, offset);
    case OP_CLOSURE: {
      offset++;
      uint8_t constant = seq->code[offset++];
      printf("%-16s %4d ", "OP_CLOSURE", constant);
      print_val(seq->consts.vals[constant]);
      printf("\n");

      ObjFunction *func = AS_FUNCTION(seq->consts.vals[constant]);
      for (int j = 0; j < func->upvalue_cnt; j++) {
        int is_local = seq->code[offset++];
        int index = seq->code[offset++];
        printf("%04d      |                     %s %d\n", offset - 2,
               is_local ? "local" : "upvalue", index);
      }

      return offset;
    }
    case OP_CLOSE_UPVALUE:
      return simple_instr("OP_CLOSE_UPVALUE", offset);
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
    case OP_POP:
      return simple_instr("OP_POP", offset);
    case OP_GET_LOCAL:
      return byte_instr("OP_GET_LOCAL", seq, offset);
    case OP_SET_LOCAL:
      return byte_instr("OP_SET_LOCAL", seq, offset);
    case OP_GET_GLOBAL:
      return const_instr("OP_GET_GLOBAL", seq, offset);
    case OP_DEFINE_GLOBAL:
      return const_instr("OP_DEFINE_GLOBAL", seq, offset);
    case OP_SET_GLOBAL:
      return const_instr("OP_SET_GLOBAL", seq, offset);
    case OP_GET_UPVALUE:
      return byte_instr("OP_GET_UPVALUE", seq, offset);
    case OP_SET_UPVALUE:
      return byte_instr("OP_SET_UPVALUE", seq, offset);
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
