from enum import Enum, auto
from typing import Callable, Iterable

from asts.ast_defs import (
    Assign,
    Binary,
    BlockStmt,
    Call,
    ClassStmt,
    Expr,
    ExpressionStmt,
    ExprVisitor,
    FunctionStmt,
    Get,
    Grouping,
    IfStmt,
    Literal,
    Logical,
    PrintStmt,
    ReturnStmt,
    Set,
    Stmt,
    StmtVisitor,
    Super,
    Ternary,
    This,
    Unary,
    Variable,
    VarStmt,
    WhileStmt,
)
from parsing.tokens import Token
from runtime.interpreter import Interpreter


class FuncType(Enum):
    NONE = auto()
    FUNCTION = auto()
    INITIALIZER = auto()
    METHOD = auto()


class ClassType(Enum):
    NONE = auto()
    CLASS = auto()
    SUBCLASS = auto()


class Resolver(ExprVisitor, StmtVisitor):
    def __init__(
        self, interpreter: Interpreter, on_error: Callable[[Token, str], None]
    ) -> None:
        self.scopes: list[dict[str, bool]] = []
        self.interpreter = interpreter
        self.on_error = on_error
        self.current_func = FuncType.NONE
        self.current_class = ClassType.NONE

    def visit_blockstmt(self, stmt: BlockStmt):
        self.begin_scope()
        self.resolve_stmts(stmt.statements)
        self.end_scope()

    def resolve_stmts(self, stmts: Iterable[Stmt]):
        for stmt in stmts:
            self.resolve_stmt(stmt)

    def resolve_stmt(self, stmt: Stmt):
        stmt.accept(self)

    def resolve_expr(self, expr: Expr):
        expr.accept(self)

    def begin_scope(self):
        self.scopes.append({})

    def end_scope(self):
        self.scopes.pop()

    def visit_varstmt(self, stmt: VarStmt):
        self.declare(stmt.name)
        if stmt.initializer is not None:
            self.resolve_expr(stmt.initializer)
        self.define(stmt.name)

    def declare(self, name: Token):
        if len(self.scopes) == 0:
            return
        if name.lexeme in self.scopes[-1]:
            self.on_error(
                name, f"A variable {name.lexeme} is already in this scope"
            )
        self.scopes[-1][name.lexeme] = False

    def define(self, name: Token):
        if len(self.scopes) == 0:
            return
        self.scopes[-1][name.lexeme] = True

    def visit_variable(self, expr: Variable):
        if (
            len(self.scopes) != 0
            and expr.name.lexeme in self.scopes[-1]
            and not self.scopes[-1][expr.name.lexeme]
        ):
            self.on_error(
                expr.name, "Can't read local variable in its own initializer"
            )
        self.resolve_local(expr, expr.name)

    def visit_set(self, expr: Set):
        self.resolve_expr(expr.value)
        self.resolve_expr(expr.obj)

    def resolve_local(self, expr: Expr, name: Token):
        for i in range(len(self.scopes) - 1, -1, -1):
            if name.lexeme in self.scopes[i]:
                self.interpreter.resolve(expr, len(self.scopes) - 1 - i)
                return

    def visit_assign(self, expr: Assign):
        self.resolve_expr(expr.value)
        self.resolve_local(expr, expr.name)

    def visit_functionstmt(self, stmt: FunctionStmt):
        self.declare(stmt.name)
        self.define(stmt.name)
        self.resolve_function(stmt, FuncType.FUNCTION)

    def resolve_function(self, function: FunctionStmt, func_type: FuncType):
        enclosing_func = self.current_func
        self.current_func = func_type
        self.begin_scope()
        for param in function.params:
            self.declare(param)
            self.define(param)
        self.resolve_stmts(function.body)
        self.end_scope()
        self.current_func = enclosing_func

    def visit_classstmt(self, stmt: ClassStmt):
        enclosing_class = self.current_class
        self.current_class = ClassType.CLASS

        self.declare(stmt.name)
        self.define(stmt.name)

        if stmt.superclass is not None:
            if stmt.name.lexeme == stmt.superclass.name.lexeme:
                self.on_error(
                    stmt.name, "A class inheriting from itself is forbidden"
                )
            self.current_class = ClassType.SUBCLASS
            self.resolve_expr(stmt.superclass)
            self.begin_scope()  # 'super' scope
            self.scopes[-1]["super"] = True

        self.begin_scope()  # 'this' scope
        self.scopes[-1]["this"] = True
        for method in stmt.methods:
            self.resolve_function(
                method,
                FuncType.METHOD
                if method.name.lexeme != "init"
                else FuncType.INITIALIZER,
            )
        self.end_scope()  # 'this' scope
        if stmt.superclass is not None:
            self.end_scope()  # 'super' scope

        self.current_class = enclosing_class

    def visit_super(self, expr: Super):
        if self.current_class == ClassType.NONE:
            self.on_error(expr.keyword, "'super' outside of a class")
        elif self.current_class != ClassType.SUBCLASS:
            self.on_error(expr.keyword, "'super' in a class with no superclass")
        self.resolve_local(expr, expr.keyword)

    def visit_get(self, expr: Get):
        self.resolve_expr(expr.obj)

    def visit_expressionstmt(self, stmt: ExpressionStmt):
        self.resolve_expr(stmt.expression)

    def visit_ifstmt(self, stmt: IfStmt):
        self.resolve_expr(stmt.condition)
        self.resolve_stmt(stmt.then_branch)
        if stmt.else_branch is not None:
            self.resolve_stmt(stmt.else_branch)

    def visit_printstmt(self, stmt: PrintStmt):
        self.resolve_expr(stmt.expression)

    def visit_returnstmt(self, stmt: ReturnStmt):
        if self.current_func == FuncType.NONE:
            self.on_error(
                stmt.keyword, "Found 'return' outside of a function scope"
            )
        if stmt.val is not None:
            if self.current_func == FuncType.INITIALIZER:
                self.on_error(
                    stmt.keyword,
                    "Found non-empty 'return' inside an init method",
                )
            self.resolve_expr(stmt.val)

    def visit_whilestmt(self, stmt: WhileStmt):
        self.resolve_expr(stmt.condition)
        self.resolve_stmt(stmt.body)

    def visit_binary(self, expr: Binary):
        self.resolve_expr(expr.left)
        self.resolve_expr(expr.right)

    def visit_call(self, expr: Call):
        self.resolve_expr(expr.callee)
        for arg in expr.arguments:
            self.resolve_expr(arg)

    def visit_grouping(self, expr: Grouping):
        self.resolve_expr(expr.expression)

    def visit_literal(self, expr: Literal):
        pass

    def visit_logical(self, expr: Logical):
        self.resolve_expr(expr.left)
        self.resolve_expr(expr.right)

    def visit_unary(self, expr: Unary):
        self.resolve_expr(expr.right)

    def visit_ternary(self, expr: Ternary):
        self.resolve_expr(expr.first)
        self.resolve_expr(expr.second)
        self.resolve_expr(expr.third)

    def visit_this(self, expr: This):
        if self.current_class == ClassType.NONE:
            self.on_error(expr.keyword, "'this' expression outside of a class")
            return
        self.resolve_local(expr, expr.keyword)
