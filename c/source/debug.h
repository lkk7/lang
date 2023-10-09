#pragma once

#include "bytecode.h"

void disassemble_bseq(ByteSequence* seq, const char* name);

int disassemble_instr(ByteSequence* seq, int offset);
