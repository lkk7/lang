#include "compile.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bytecode.h"
#include "common.h"
#include "memory.h"
#include "object.h"
#include "scanner.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  Token current;
  Token previous;
  bool had_err;
  bool panic_mode;
} Parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,
  PREC_OR,
  PREC_AND,
  PREC_EQUALITY,
  PREC_COMPARISON,
  PREC_TERM,
  PREC_FACTOR,
  PREC_UNARY,
  PREC_CALL,
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool can_assign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  Token name;
  int depth;
  bool is_captured;
} Local;

typedef struct {
  uint8_t index;
  bool is_local;
} Upvalue;

typedef enum { TYPE_FUNCTION, TYPE_SCRIPT } FunctionType;

typedef struct Compiler {
  struct Compiler* enclosing;
  ObjFunction* function;
  FunctionType type;

  Local locals[UINT8_COUNT];
  int local_cnt;
  Upvalue upvalues[UINT8_COUNT];
  int scope_depth;
} Compiler;

Parser parser;
Compiler* current = NULL;
ByteSequence* compiling_bseq;

static ByteSequence* current_bseq(void) { return &current->function->bseq; }

static void error_at(Token* token, const char* msg) {
  if (parser.panic_mode) {
    return;
  }
  parser.panic_mode = true;
  (void)fprintf(stderr, "[line %d] Error", token->line);
  if (token->type == TOKEN_EOF) {
    (void)fprintf(stderr, " at EOF");
  } else if (token->type == TOKEN_ERROR) {
    //
  } else {
    (void)fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  (void)fprintf(stderr, ": %s\n", msg);
  parser.had_err = true;
}

static void error(const char* msg) { error_at(&parser.previous, msg); }

static void error_at_current(const char* msg) {
  error_at(&parser.current, msg);
}

static void advance(void) {
  parser.previous = parser.current;
  for (;;) {
    parser.current = scan_token();
    if (parser.current.type != TOKEN_ERROR) {
      break;
    }
    error_at_current(parser.current.start);
  }
}

static void consume(TokenType type, const char* msg) {
  if (parser.current.type == type) {
    advance();
    return;
  }
  error_at_current(msg);
}

static bool check(TokenType type) { return parser.current.type == type; }

static bool match(TokenType type) {
  if (!check(type)) {
    return false;
  }
  advance();
  return true;
}

static void emit_byte(uint8_t byte) {
  write_bsequence(current_bseq(), byte, parser.previous.line);
}

static void emit_bytes(uint8_t byte1, uint8_t byte2) {
  emit_byte(byte1);
  emit_byte(byte2);
}

static void emit_loop(int loopStart) {
  emit_byte(OP_LOOP);
  int offset = current_bseq()->size - loopStart + 2;
  if (offset > UINT16_MAX) {
    error("Loop body too large");
  }
  emit_byte((offset >> 8) & 0xff);
  emit_byte(offset & 0xff);
}

static int emit_jump(uint8_t instruction) {
  emit_byte(instruction);
  emit_byte(0xff);
  emit_byte(0xff);
  return current_bseq()->size - 2;
}

static void emit_return(void) {
  emit_byte(OP_NIL);
  emit_byte(OP_RETURN);
}

static uint8_t make_constant(Value val) {
  int constant = add_const(current_bseq(), val);
  if (constant > UINT8_MAX) {
    error("Too many constants in one byte sequence");
    return 0;
  }
  return (uint8_t)constant;
}

static void emit_constant(Value val) {
  emit_bytes(OP_CONSTANT, make_constant(val));
}

static void patch_jump(int offset) {
  int jump = current_bseq()->size - offset - 2;
  if (jump > UINT16_MAX) {
    error("Too much code to jump over");
  }
  current_bseq()->code[offset] = (jump >> 8) & 0xff;
  current_bseq()->code[offset + 1] = jump & 0xff;
}

static void init_compiler(Compiler* compiler, FunctionType type) {
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;
  compiler->local_cnt = 0;
  compiler->scope_depth = 0;
  compiler->function = new_function();
  current = compiler;
  if (type != TYPE_SCRIPT) {
    current->function->name =
        copy_str(parser.previous.start, parser.previous.length);
  }

  Local* local = &current->locals[current->local_cnt++];
  local->depth = 0;
  local->name.start = "";
  local->name.length = 0;
}

static ObjFunction* end_compiler(void) {
  emit_return();
  ObjFunction* function = current->function;
#ifdef DEBUG_PRINT_CODE
  if (!parser.had_err) {
    disassemble_bseq(current_bseq(), function->name != NULL
                                         ? function->name->chars
                                         : "<script>");
  }
#endif

  current = current->enclosing;
  return function;
}

static void begin_scope(void) { current->scope_depth++; }
static void end_scope(void) {
  current->scope_depth--;
  while (current->local_cnt > 0 &&
         current->locals[current->local_cnt - 1].depth > current->scope_depth) {
    if (current->locals[current->local_cnt - 1].is_captured) {
      emit_byte(OP_CLOSE_UPVALUE);
    } else {
      emit_byte(OP_POP);
    }
    current->local_cnt--;
  }
}

static void expression(void);
static void statement(void);
static void declaration(void);
static ParseRule* get_rule(TokenType type);
static void parse_precedence(Precedence precedence);

static void binary(bool can_assign) {
  (void)can_assign;
  TokenType op_type = parser.previous.type;
  ParseRule* rule = get_rule(op_type);
  parse_precedence((Precedence)(rule->precedence + 1));

  switch (op_type) {
    case TOKEN_BANG_EQUAL:
      emit_bytes(OP_EQUAL, OP_NOT);
      break;
    case TOKEN_EQUAL_EQUAL:
      emit_byte(OP_EQUAL);
      break;
    case TOKEN_GREATER:
      emit_byte(OP_GREATER);
      break;
    case TOKEN_GREATER_EQUAL:
      emit_bytes(OP_LESS, OP_NOT);
      break;
    case TOKEN_LESS:
      emit_byte(OP_LESS);
      break;
    case TOKEN_LESS_EQUAL:
      emit_bytes(OP_GREATER, OP_NOT);
      break;
    case TOKEN_PLUS:
      emit_byte(OP_ADD);
      break;
    case TOKEN_MINUS:
      emit_byte(OP_SUBTRACT);
      break;
    case TOKEN_STAR:
      emit_byte(OP_MULTIPLY);
      break;
    case TOKEN_SLASH:
      emit_byte(OP_DIVIDE);
      break;
    default:
      return;
  }
}

static uint8_t argument_list(void) {
  uint8_t arg_count = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();
      if (arg_count == 255) {
        error("Can't have more than 255 arguments");
      }
      arg_count++;
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after arguments");
  return arg_count;
}

static void call(bool can_assign) {
  uint8_t arg_count = argument_list();
  emit_bytes(OP_CALL, arg_count);
}

static void literal(bool can_assign) {
  (void)can_assign;
  switch (parser.previous.type) {
    case TOKEN_FALSE:
      emit_byte(OP_FALSE);
      break;
    case TOKEN_NIL:
      emit_byte(OP_NIL);
      break;
    case TOKEN_TRUE:
      emit_byte(OP_TRUE);
      break;
    default:
      return;
  }
}

static void parse_precedence(Precedence precedence) {
  advance();
  ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
  if (prefix_rule == NULL) {
    error("Expected expression");
    return;
  }
  bool can_assign = precedence <= PREC_ASSIGNMENT;
  prefix_rule(can_assign);

  while (precedence <= get_rule(parser.current.type)->precedence) {
    advance();
    ParseFn infix_rule = get_rule(parser.previous.type)->infix;
    infix_rule(can_assign);
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target");
  }
}

static uint8_t identifier_constant(Token* name) {
  return make_constant(OBJ_VAL(copy_str(name->start, name->length)));
}

static bool identifiers_equal(Token* a, Token* b) {
  if (a->length != b->length) {
    return false;
  }
  return memcmp(a->start, b->start, a->length) == 0;
}

static int resolve_local(Compiler* compiler, Token* name) {
  for (int i = compiler->local_cnt - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiers_equal(name, &local->name)) {
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer");
      }
      return i;
    }
  }
  return -1;
}

static int add_upvalue(Compiler* compiler, uint8_t index, bool is_local) {
  int upvalue_cnt = compiler->function->upvalue_cnt;
  for (int i = 0; i < upvalue_cnt; i++) {
    Upvalue* upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->is_local == is_local) {
      return i;
    }
  }
  if (upvalue_cnt == UINT8_COUNT) {
    error("Too many closure variables in a function");
    return 0;
  }
  compiler->upvalues[upvalue_cnt].is_local = is_local;
  compiler->upvalues[upvalue_cnt].index = index;
  return compiler->function->upvalue_cnt++;
}

static int resolve_upvalue(Compiler* compiler, Token* name) {
  if (compiler->enclosing == NULL) {
    return -1;
  }
  int local = resolve_local(compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->locals[local].is_captured = true;
    return add_upvalue(compiler, (uint8_t)local, true);
  }
  int upvalue = resolve_upvalue(compiler->enclosing, name);
  if (upvalue != -1) {
    return add_upvalue(compiler, (uint8_t)upvalue, false);
  }
  return -1;
}

static void add_local(Token name) {
  if (current->local_cnt == UINT8_COUNT) {
    error("Too many local variables in function");
    return;
  }
  Local* local = &current->locals[current->local_cnt++];
  local->name = name;
  local->depth = -1;
  local->is_captured = false;
}

static void declare_variable(void) {
  if (current->scope_depth == 0) {
    return;
  }

  Token* name = &parser.previous;
  for (int i = current->local_cnt - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scope_depth) {
      break;
    }
    if (identifiers_equal(name, &local->name)) {
      error("A variable with this name in this scope already exists");
    }
  }

  add_local(*name);
}

static uint8_t parse_variable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);

  declare_variable();
  if (current->scope_depth > 0) {
    return 0;
  }

  return identifier_constant(&parser.previous);
}

static void mark_initialized(void) {
  if (current->scope_depth == 0) {
    return;
  }
  current->locals[current->local_cnt - 1].depth = current->scope_depth;
}

static void define_variable(uint8_t global) {
  if (current->scope_depth > 0) {
    mark_initialized();
    return;
  }
  emit_bytes(OP_DEFINE_GLOBAL, global);
}

static void and_(bool can_assign) {
  int endJump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  parse_precedence(PREC_AND);
  patch_jump(endJump);
}

static void expression(void) { parse_precedence(PREC_ASSIGNMENT); }

static void block(void) {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }
  consume(TOKEN_RIGHT_BRACE, "Expected '}' after block");
}

static void function(FunctionType type) {
  Compiler compiler;
  init_compiler(&compiler, type);
  begin_scope();

  consume(TOKEN_LEFT_PAREN, "Expected '(' after function name");
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        error_at_current("Can't have more than 255 parameters");
      }
      uint8_t constant = parse_variable("Expected parameter name");
      define_variable(constant);
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after parameters");

  consume(TOKEN_LEFT_BRACE, "Expected '{' before function body");
  block();

  ObjFunction* func = end_compiler();
  emit_bytes(OP_CLOSURE, make_constant(OBJ_VAL(func)));
  for (int i = 0; i < func->upvalue_cnt; i++) {
    emit_byte(compiler.upvalues[i].is_local ? 1 : 0);
    emit_byte(compiler.upvalues[i].index);
  }
}

static void fun_declaration() {
  uint8_t global = parse_variable("Expected function name");
  mark_initialized();
  function(TYPE_FUNCTION);
  define_variable(global);
}

static void var_declaration(void) {
  uint8_t global = parse_variable("Expected variable name");
  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emit_byte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration");
  define_variable(global);
}

static void expr_statement(void) {
  expression();
  consume(TOKEN_SEMICOLON, "Expected ';' after expression");
  emit_byte(OP_POP);
}

static void for_statement(void) {
  begin_scope();
  consume(TOKEN_LEFT_PAREN, "Expected '(' after 'for'");
  if (match(TOKEN_SEMICOLON)) {
    // Do nothing
  } else if (match(TOKEN_VAR)) {
    var_declaration();
  } else {
    expr_statement();
  }

  int loop_start = current_bseq()->size;
  int exit_jump = -1;
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after loop condition");

    exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
  }

  if (!match(TOKEN_RIGHT_PAREN)) {
    int body_jump = emit_jump(OP_JUMP);
    int increment_start = current_bseq()->size;
    expression();
    emit_byte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after for clauses");
    emit_loop(loop_start);
    loop_start = increment_start;
    patch_jump(body_jump);
  }

  statement();
  emit_loop(loop_start);

  if (exit_jump != -1) {
    patch_jump(exit_jump);
    emit_byte(OP_POP);
  }

  end_scope();
}

static void if_statement(void) {
  consume(TOKEN_LEFT_PAREN, "Expected '(' after 'if'");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition");

  int then_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  statement();
  int elseJump = emit_jump(OP_JUMP);

  patch_jump(then_jump);
  emit_byte(OP_POP);
  if (match(TOKEN_ELSE)) {
    statement();
  }
  patch_jump(elseJump);
}

static void print_statement(void) {
  expression();
  consume(TOKEN_SEMICOLON, "Expected ';' after value");
  emit_byte(OP_PRINT);
}

static void return_statement(void) {
  if (current->type == TYPE_SCRIPT) {
    error("Can't return from top-level code");
  }

  if (match(TOKEN_SEMICOLON)) {
    emit_return();
  } else {
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after return value");
    emit_byte(OP_RETURN);
  }
}

static void while_statement(void) {
  int loop_start = current_bseq()->size;
  consume(TOKEN_LEFT_PAREN, "Expected '(' after 'while'");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition");

  int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  statement();
  emit_loop(loop_start);

  patch_jump(exit_jump);
  emit_byte(OP_POP);
}

static void synchronize(void) {
  parser.panic_mode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) {
      return;
    }
    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;
      default:;
    }
    advance();
  }
}

static void statement(void) {
  if (match(TOKEN_PRINT)) {
    print_statement();
  } else if (match(TOKEN_FOR)) {
    for_statement();
  } else if (match(TOKEN_IF)) {
    if_statement();
  } else if (match(TOKEN_RETURN)) {
    return_statement();
  } else if (match(TOKEN_WHILE)) {
    while_statement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    begin_scope();
    block();
    end_scope();
  } else {
    expr_statement();
  }
}

static void declaration(void) {
  if (match(TOKEN_FUN)) {
    fun_declaration();
  } else if (match(TOKEN_VAR)) {
    var_declaration();
  } else {
    statement();
  }
  if (parser.panic_mode) {
    synchronize();
  }
}

static void grouping(bool can_assign) {
  (void)can_assign;
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression");
}

static void number(bool can_assign) {
  (void)can_assign;
  double val = strtod(parser.previous.start, NULL);
  emit_constant(NUMBER_VAL(val));
}

static void or_(bool can_assign) {
  int else_jump = emit_jump(OP_JUMP_IF_FALSE);
  int end_jump = emit_jump(OP_JUMP);
  patch_jump(else_jump);
  emit_byte(OP_POP);
  parse_precedence(PREC_OR);
  patch_jump(end_jump);
}

static void string(bool can_assign) {
  (void)can_assign;
  emit_constant(
      OBJ_VAL(copy_str(parser.previous.start + 1, parser.previous.length - 2)));
}

static void named_variable(Token name, bool can_assign) {
  uint8_t get_op;
  uint8_t set_op;
  int arg = resolve_local(current, &name);
  if (arg != -1) {
    get_op = OP_GET_LOCAL;
    set_op = OP_SET_LOCAL;
  } else if ((arg = resolve_upvalue(current, &name)) != -1) {
    get_op = OP_GET_UPVALUE;
    set_op = OP_SET_UPVALUE;
  } else {
    arg = identifier_constant(&name);
    get_op = OP_GET_GLOBAL;
    set_op = OP_SET_GLOBAL;
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    expression();
    emit_bytes(set_op, (uint8_t)arg);
  } else {
    emit_bytes(get_op, (uint8_t)arg);
  }
}

static void variable(bool can_assign) {
  named_variable(parser.previous, can_assign);
}

static void unary(bool can_assign) {
  (void)can_assign;
  TokenType op_type = parser.previous.type;

  parse_precedence(PREC_UNARY);

  switch (op_type) {
    case TOKEN_BANG:
      emit_byte(OP_NOT);
      break;
    case TOKEN_MINUS:
      emit_byte(OP_NEGATE);
      break;
    default:
      return;
  }
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, and_, PREC_AND},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or_, PREC_OR},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static ParseRule* get_rule(TokenType type) { return &rules[type]; }

ObjFunction* compile(const char* src) {
  init_scanner(src);
  Compiler compiler;
  init_compiler(&compiler, TYPE_SCRIPT);
  parser.had_err = false;
  parser.panic_mode = false;

  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  ObjFunction* function = end_compiler();
  return parser.had_err ? NULL : function;
}

void mark_compiler_roots() {
  Compiler* compiler = current;
  while (compiler != NULL) {
    mark_object((Obj*)compiler->function);
    compiler = compiler->enclosing;
  }
}
