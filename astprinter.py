from expressions import (Binary, Grouping, Literal,
                         Ternary, Unary, Expr, Visitor)


class AstPrinter(Visitor):
    def print(self, expr: Expr):
        return expr.accept(self)

    def visit_ternary(self, expr: Ternary):
        return self.parenthesize(
            expr.operator.lexeme, expr.first, expr.second, expr.third
        )

    def visit_binary(self, expr: Binary):
        return self.parenthesize(expr.operator.lexeme, expr.left, expr.right)

    def visit_grouping(self, expr: Grouping):
        return self.parenthesize("group", expr.expression)

    def visit_literal(self, expr: Literal):
        return str(expr.value)

    def visit_unary(self, expr: Unary):
        return self.parenthesize(expr.operator.lexeme, expr.right)

    def parenthesize(self, name: str, *args: Expr) -> str:
        return f"({name}{''.join(' ' + expr.accept(self) for expr in args)})"
