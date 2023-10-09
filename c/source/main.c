#include "bytecode.h"
#include "common.h"
#include "debug.h"

int main(int argc, const char *argv[]) {
  (void)argc;
  (void)argv;

  ByteSequence bseq;
  init_bsequence(&bseq);
  int const_1 = add_const(&bseq, 1.23);
  int const_2 = add_const(&bseq, 3.21);
  write_bsequence(&bseq, OP_CONSTANT, 123);
  write_bsequence(&bseq, (uint8_t)const_1, 123);
  write_bsequence(&bseq, OP_CONSTANT, 123);
  write_bsequence(&bseq, (uint8_t)const_2, 123);
  write_bsequence(&bseq, OP_RETURN, 123);

  disassemble_bseq(&bseq, "test sequence");
  free_bsequence(&bseq);

  return 0;
}
