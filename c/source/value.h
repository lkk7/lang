#pragma once

#include "common.h"

typedef double Value;

typedef struct {
  int size;
  int capacity;
  Value* vals;
} ValueArray;

void init_valarr(ValueArray* arr);
void write_valarr(ValueArray* arr, Value val);
void free_valarr(ValueArray* arr);

void print_val(Value val);
