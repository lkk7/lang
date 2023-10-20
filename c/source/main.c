#include "bytecode.h"
#include "common.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char *argv[]) {
  (void)argc;
  (void)argv;

  init_vm();
  ByteSequence bseq;
  init_bsequence(&bseq);

  int constant = add_const(&bseq, 1);
  write_bsequence(&bseq, OP_CONSTANT, 123);
  write_bsequence(&bseq, (uint8_t)constant, 123);
  constant = add_const(&bseq, 2);
  write_bsequence(&bseq, OP_CONSTANT, 123);
  write_bsequence(&bseq, (uint8_t)constant, 123);
  constant = add_const(&bseq, 3);
  write_bsequence(&bseq, OP_CONSTANT, 123);
  write_bsequence(&bseq, (uint8_t)constant, 123);
  write_bsequence(&bseq, OP_MULTIPLY, 123);
  write_bsequence(&bseq, OP_ADD, 123);
  constant = add_const(&bseq, 4);
  write_bsequence(&bseq, OP_CONSTANT, 123);
  write_bsequence(&bseq, (uint8_t)constant, 123);
  constant = add_const(&bseq, 5);
  write_bsequence(&bseq, OP_CONSTANT, 123);
  write_bsequence(&bseq, (uint8_t)constant, 123);
  write_bsequence(&bseq, OP_NEGATE, 123);
  write_bsequence(&bseq, OP_DIVIDE, 123);
  write_bsequence(&bseq, OP_SUBTRACT, 123);
  write_bsequence(&bseq, OP_RETURN, 123);

  disassemble_bseq(&bseq, "test sequence");
  interpret(&bseq);

  free_vm();
  free_bsequence(&bseq);

  return 0;
}
