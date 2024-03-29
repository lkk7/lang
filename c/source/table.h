#pragma once

#include "common.h"
#include "value.h"

typedef struct {
  ObjStr* key;
  Value value;
} Entry;

typedef struct {
  int count;
  int capacity;
  Entry* entries;
} Table;

void init_table(Table* table);
void free_table(Table* table);
bool table_get(Table* table, ObjStr* key, Value* result);
bool table_set(Table* table, ObjStr* key, Value value);
bool table_delete(Table* table, ObjStr* key);
void table_add_all(Table* from, Table* to);
ObjStr* table_find_str(Table* table, const char* chars, int length,
                       uint32_t hash);
void table_remove_white(Table* table);
void mark_table(Table* table);
