import typing
from expressions import (Binary, Grouping, Literal,
                         Ternary, Unary, Expr, Visitor)
from runtime_err import LangRuntimeError
from tokens import Token, TokenType


class Interpreter(Visitor):
    def __init__(self, on_error: typing.Callable[[Exception], None]):
        self.on_error = on_error

    def interpret(self, expr: Expr):
        try:
            val = self.eval(expr)
            print(self.stringify(val))
        except LangRuntimeError as e:
            self.on_error(e)

    def visit_ternary(self, expr: Ternary):
        if bool(self.eval(expr.first)):
            return self.eval(expr.second)
        return self.eval(expr.third)

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
                if ((type(left) == str and type(right) == str)
                    or ((type(left) == int or type(left) == float)
                        and (type(right) == int or type(right) == float))):
                    return left + right
                raise LangRuntimeError(
                    expr.operator,
                    "Operands must be two numbers or two strings."
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

    def visit_literal(self, expr: Literal):
        return expr.value

    def check_num_operand(self, operator: Token, operand: typing.Any):
        if type(operand) == int or type(operand) == float:
            return
        raise LangRuntimeError(operator, "Operand must be a number")

    def check_num_operands(self, operator: Token,
                           left: typing.Any,
                           right: typing.Any):
        if ((type(left) == int or type(left) == float)
                and (type(right) == int or type(right) == float)):
            return
        raise LangRuntimeError(
            operator, "Operands must be numbers"
        )

    def eval(self, expr: Expr):
        return expr.accept(self)

    def stringify(self, expr: typing.Any):
        if expr is None:
            return "nil"
        if isinstance(expr, bool):
            return str(expr).lower()
        if isinstance(expr, str):
            return f'"{expr}"'
        return str(expr)
