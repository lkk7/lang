#pragma once
#include "bytecode.h"
#include "object.h"

ObjFunction* compile(const char* src);
void mark_compiler_roots(void);
