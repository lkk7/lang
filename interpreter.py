from typing import Any, Callable, cast
import builtins

from ast_defs import (
    Assign,
    Binary,
    BlockStmt,
    Call,
    Expr,
    ExpressionStmt,
    ExprVisitor,
    FunctionStmt,
    Grouping,
    IfStmt,
    Literal,
    Logical,
    PrintStmt,
    ReturnStmt,
    Stmt,
    StmtVisitor,
    Ternary,
    Unary,
    Variable,
    VarStmt,
    WhileStmt,
)

from callable_obj import CallableObj
from environment import Environment
from function import FunctionObj
from return_val import ReturnVal
from runtime_err import LangRuntimeError
from tokens import Token, TokenType


class Interpreter(ExprVisitor, StmtVisitor):
    def __init__(self, on_error: Callable[[LangRuntimeError], None]):
        self.globals = Environment()
        self.locals: dict[Expr, int] = {}
        self.environment = self.globals
        self.on_error = on_error
        self.globals.define("clock", CallableObj())

    def interpret(self, statements: list[Stmt]):
        try:
            for stmt in statements:
                self.execute(stmt)
        except LangRuntimeError as e:
            self.on_error(e)

    def execute(self, statement: Stmt):
        statement.accept(self)

    def visit_blockstmt(self, stmt: BlockStmt):
        self.execute_block(stmt.statements, Environment(self.environment))
        return None

    def execute_block(self, statements: tuple[Stmt, ...], environment: Environment):
        previous = self.environment
        try:
            self.environment = environment
            for stmt in statements:
                self.execute(stmt)
        finally:
            self.environment = previous

    def visit_functionstmt(self, stmt: FunctionStmt):
        function = FunctionObj(stmt, self.environment)
        self.environment.define(stmt.name.lexeme, function)
        return None

    def visit_ifstmt(self, stmt: IfStmt):
        if self.eval(stmt.condition):
            self.execute(stmt.then_branch)
        elif stmt.else_branch:
            self.execute(stmt.else_branch)
        return None

    def visit_whilestmt(self, stmt: WhileStmt):
        while self.eval(stmt.condition):
            self.execute(stmt.body)
        return None

    def visit_assign(self, expr: Assign):
        val = self.eval(expr.value)
        if expr in self.locals:
            self.environment.assign_at(self.locals[expr], expr.name, val)
        else:
            self.globals.assign(expr.name, val)

        return val

    def visit_varstmt(self, stmt: VarStmt):
        value = None
        if stmt.initializer is not None:
            value = self.eval(stmt.initializer)
        self.environment.define(stmt.name.lexeme, value)
        return None

    def visit_expressionstmt(self, stmt: ExpressionStmt):
        self.eval(stmt.expression)
        return None

    def visit_printstmt(self, stmt: PrintStmt):
        val = self.eval(stmt.expression)
        print(self.stringify(val))
        return None

    def visit_returnstmt(self, stmt: ReturnStmt):
        val = None
        if stmt.val is not None:
            val = self.eval(stmt.val)
        raise ReturnVal(val)

    def visit_ternary(self, expr: Ternary):
        if bool(self.eval(expr.first)):
            return self.eval(expr.second)
        return self.eval(expr.third)

    def visit_logical(self, expr: Logical):
        left = self.eval(expr.left)
        if expr.operator == TokenType.OR:
            if left:
                return left
        else:
            if not left:
                return left
        return self.eval(expr.right)

    def visit_binary(self, expr: Binary):
        left = self.eval(expr.left)
        right = self.eval(expr.right)
        match expr.operator.type:
            case TokenType.STAR:
                self.check_num_operands(expr.operator, left, right)
                return left * right
            case TokenType.SLASH:
                self.check_num_operands(expr.operator, left, right)
                return left / right
            case TokenType.MINUS:
                self.check_num_operands(expr.operator, left, right)
                return left - right
            case TokenType.PLUS:
                match type(left), type(right):
                    case [builtins.float, builtins.float] | [
                        builtins.str,
                        builtins.str,
                    ] | [
                        builtins.int,
                        builtins.int,
                    ] | [
                        builtins.int,
                        builtins.float,
                    ] | [
                        builtins.float,
                        builtins.int,
                    ]:
                        return left + right
                raise LangRuntimeError(
                    expr.operator,
                    "Operands must be two numbers or two strings.",
                )
            case TokenType.GREATER:
                self.check_num_operands(expr.operator, left, right)
                return left > right
            case TokenType.GREATER_EQUAL:
                self.check_num_operands(expr.operator, left, right)
                return left >= right
            case TokenType.LESS:
                self.check_num_operands(expr.operator, left, right)
                return left < right
            case TokenType.LESS_EQUAL:
                self.check_num_operands(expr.operator, left, right)
                return left <= right
            case TokenType.BANG_EQUAL:
                return left != right
            case TokenType.EQUAL_EQUAL:
                return left == right

        return None

    def visit_grouping(self, expr: Grouping):
        return self.eval(expr.expression)

    def visit_unary(self, expr: Unary):
        val = self.eval(expr.right)
        match expr.operator.type:
            case TokenType.MINUS:
                self.check_num_operand(expr.operator, val)
                return -val
            case TokenType.BANG:
                return not bool(val)
        return None

    def visit_variable(self, expr: Variable):
        return self.lookup_variable(expr.name, expr)

    def lookup_variable(self, name: Token, expr: Expr):
        if expr in self.locals:
            return self.environment.get_at(self.locals[expr], name)
        return self.globals[name]

    def visit_call(self, expr: Call):
        args = []
        for arg in expr.arguments:
            args.append(self.eval(arg))
        function = cast(CallableObj, self.eval(expr.callee))
        if not isinstance(function, CallableObj):
            raise LangRuntimeError(expr.paren, "Non-callable called")
        if len(args) != function.arity():
            raise LangRuntimeError(
                expr.paren,
                f"Expected {function.arity()} arguments but got {len(args)}",
            )

        return function.call(self, args)

    def visit_literal(self, expr: Literal):
        return expr.value

    def check_num_operand(self, operator: Token, operand: Any):
        if type(operand) == int or type(operand) == float:
            return
        raise LangRuntimeError(operator, "Operand must be a number")

    def check_num_operands(self, operator: Token, left: Any, right: Any):
        if (type(left) == int or type(left) == float) and (
            type(right) == int or type(right) == float
        ):
            return
        raise LangRuntimeError(operator, "Operands must be numbers")

    def eval(self, expr: Expr | Stmt):
        return expr.accept(self)

    def stringify(self, expr: Any):
        if expr is None:
            return "nil"
        if isinstance(expr, bool):
            return str(expr).lower()
        if isinstance(expr, str):
            return str(expr)
        return str(expr)

    def resolve(self, expr: Expr, depth: int):
        self.locals[expr] = depth
