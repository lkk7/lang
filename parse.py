from typing import Callable

from ast_defs import (
    Assign,
    Binary,
    BlockStmt,
    Call,
    Expr,
    ExpressionStmt,
    FunctionStmt,
    Grouping,
    IfStmt,
    Literal,
    Logical,
    PrintStmt,
    ReturnStmt,
    Stmt,
    Ternary,
    Unary,
    Variable,
    VarStmt,
    WhileStmt,
)
from tokens import Token, TokenType


class ParseError(SyntaxError):
    pass


class Parser:
    def __init__(
        self, tokens: list[Token], on_error: Callable[[Token, str], None]
    ):
        self.tokens = tokens
        self.on_error = on_error
        self.current = 0

    def parse(self) -> list[Stmt]:
        statements: list[Stmt] = []
        while not self.is_end():
            statements.append(self.decl_stmt())
        return statements

    def decl_stmt(self):
        try:
            if self.match(TokenType.VAR):
                return self.var_declaration()
            if self.match(TokenType.FUN):
                return self.function_stmt("function")
            return self.statement()
        except ParseError:
            self.synchronize()
            return None

    def statement(self):
        if self.match(TokenType.PRINT):
            return self.print_stmt()
        if self.match(TokenType.LEFT_BRACE):
            return BlockStmt(self.block())
        if self.match(TokenType.IF):
            return self.if_stmt()
        if self.match(TokenType.WHILE):
            return self.while_stmt()
        if self.match(TokenType.FOR):
            return self.for_stmt()
        if self.match(TokenType.RETURN):
            return self.return_stmt()
        return self.expr_stmt()

    def if_stmt(self):
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'if'")
        condition = self.expression()
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after 'if' condition")
        then_branch = self.statement()
        else_branch = None
        if self.match(TokenType.ELSE):
            else_branch = self.statement()
        return IfStmt(condition, then_branch, else_branch)

    def while_stmt(self):
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'while'")
        condition = self.expression()
        self.consume(
            TokenType.RIGHT_PAREN, "Expected ')' after 'while' condition"
        )
        body = self.statement()
        return WhileStmt(condition, body)

    def for_stmt(self):
        self.consume(TokenType.LEFT_PAREN, "Expected '(' after 'for'")

        if self.match(TokenType.SEMICOLON):
            initializer = None
        elif self.match(TokenType.VAR):
            initializer = self.var_declaration()
        else:
            initializer = self.expr_stmt()

        condition = None
        if not self.check(TokenType.SEMICOLON):
            condition = self.expression()
        self.consume(TokenType.SEMICOLON, "Expected ';' after loop condition")

        increment = None
        if not self.check(TokenType.RIGHT_PAREN):
            increment = self.expression()
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after 'for' clause")

        body = self.statement()
        if increment is not None:
            body = BlockStmt([body, ExpressionStmt(increment)])

        if condition is None:
            condition = Literal(True)
        body = WhileStmt(condition, body)

        if initializer is not None:
            body = BlockStmt([initializer, body])

        return body

    def var_declaration(self):
        name = self.consume(TokenType.IDENTIFIER, "Expected variable name")
        initializer = None
        if self.match(TokenType.EQUAL):
            initializer = self.expression()
        self.consume(
            TokenType.SEMICOLON, "Expected ';' after variable declaration"
        )
        return VarStmt(name, initializer)

    def return_stmt(self):
        keyword = self.previous()
        val = None
        if not self.check(TokenType.SEMICOLON):
            val = self.expression()
        self.consume(TokenType.SEMICOLON, "Expected ; after a return value")
        return ReturnStmt(keyword, val)

    def block(self):
        statements = []
        while not self.check(TokenType.RIGHT_BRACE) and not self.is_end():
            statements.append(self.decl_stmt())
        self.consume(TokenType.RIGHT_BRACE, "Expected '}' at after block")
        return tuple(statements)

    def expr_stmt(self):
        expr = self.expression()
        self.consume(TokenType.SEMICOLON, "Expected ';' after expression")
        return ExpressionStmt(expr)

    def function_stmt(self, kind: str):
        name = self.consume(TokenType.IDENTIFIER, f"Expected {kind} name")
        self.consume(
            TokenType.LEFT_PAREN, "Expected '(' after " + kind + " name"
        )
        parameters: list[Token] = []
        if not self.check(TokenType.RIGHT_PAREN):
            while True:
                if len(parameters) >= 255:
                    self.error(
                        self.peek(), "Can't have more than 255 parameters"
                    )
                parameters.append(
                    self.consume(
                        TokenType.IDENTIFIER, "Expected parameter name"
                    )
                )
                if not self.match(TokenType.COMMA):
                    break
        self.consume(TokenType.RIGHT_PAREN, "Expected ')' after parameters")
        self.consume(TokenType.LEFT_BRACE, f"Expected '{{' before {kind} body")
        body = self.block()
        return FunctionStmt(name, tuple(parameters), body)

    def print_stmt(self):
        expr = self.expression()
        self.consume(TokenType.SEMICOLON, "Expected ';' after value.")
        return PrintStmt(expr)

    def expression(self):
        return self.assignment()

    def assignment(self):
        expr = self.ternary()

        if self.match(TokenType.EQUAL):
            equals = self.previous()
            val = self.assignment()
            if isinstance(expr, Variable):
                return Assign(expr.name, val)
            self.error(equals, "Invalid assignment target")
        return expr

    def ternary(self):
        expr = self.or_expr()
        if self.match(TokenType.QUESTION):
            operator = self.previous()
            if_true = self.ternary()
            self.consume(
                TokenType.COLON, "Expected ':' in the ternary '?:' operator"
            )
            if_false = self.ternary()
            return Ternary(operator, expr, if_true, if_false)
        return expr

    def or_expr(self):
        expr = self.and_expr()
        while self.match(TokenType.OR):
            operator = self.previous()
            right = self.and_expr()
            expr = Logical(expr, operator, right)
        return expr

    def and_expr(self):
        expr = self.equality()
        while self.match(TokenType.AND):
            operator = self.previous()
            right = self.equality()
            expr = Logical(expr, operator, right)
        return expr

    def equality(self):
        expr = self.comparison()

        while self.match(TokenType.EQUAL_EQUAL, TokenType.BANG_EQUAL):
            operator = self.previous()
            right = self.comparison()
            expr = Binary(expr, operator, right)
        return expr

    def comparison(self):
        expr = self.term()
        while self.match(
            TokenType.GREATER,
            TokenType.GREATER_EQUAL,
            TokenType.LESS,
            TokenType.LESS_EQUAL,
        ):
            operator = self.previous()
            right = self.comparison()
            expr = Binary(expr, operator, right)
        return expr

    def term(self):
        expr = self.factor()
        while self.match(TokenType.MINUS, TokenType.PLUS):
            operator = self.previous()
            right = self.factor()
            expr = Binary(expr, operator, right)
        return expr

    def factor(self):
        expr = self.unary()
        while self.match(TokenType.STAR, TokenType.SLASH):
            operator = self.previous()
            right = self.unary()
            expr = Binary(expr, operator, right)
        return expr

    def unary(self):
        if self.match(TokenType.BANG, TokenType.MINUS):
            operator = self.previous()
            right = self.unary()
            return Unary(operator, right)
        return self.call()

    def call(self):
        expr = self.primary()
        while True:
            if self.match(TokenType.LEFT_PAREN):
                expr = self.finish_call(expr)
            else:
                break
        return expr

    def finish_call(self, callee: Expr):
        args: list[Expr] = []
        if not self.check(TokenType.RIGHT_PAREN):
            args.append(self.expression())
            while self.match(TokenType.COMMA):
                args.append(self.expression())
        if len(args) >= 255:
            self.error(
                self.peek(), "A function can't have more than 255 arguments"
            )
        paren = self.consume(
            TokenType.RIGHT_PAREN, "Expected ')' after arguments"
        )
        return Call(callee, paren, tuple(args))

    def primary(self):
        if self.match(TokenType.FALSE):
            return Literal(False)
        if self.match(TokenType.TRUE):
            return Literal(True)
        if self.match(TokenType.NIL):
            return Literal(None)
        if self.match(TokenType.NUMBER, TokenType.STRING):
            return Literal(self.previous().literal)
        if self.match(TokenType.IDENTIFIER):
            return Variable(self.previous())
        if self.match(TokenType.LEFT_PAREN):
            expr = self.expression()
            self.consume(TokenType.RIGHT_PAREN, "Expected ')' after expression")
            return Grouping(expr)
        raise self.error(self.peek(), "Expected expression")

    def consume(self, type: TokenType, msg: str):
        if self.check(type):
            return self.advance()
        raise self.error(self.peek(), msg)

    def match(self, *types: TokenType):
        for t in types:
            if self.check(t):
                self.advance()
                return True
        return False

    def check(self, type: TokenType):
        if self.is_end():
            return False
        return self.peek().type == type

    def advance(self):
        if not self.is_end():
            self.current += 1
        return self.previous()

    def is_end(self):
        return self.peek().type == TokenType.EOF

    def previous(self):
        return self.tokens[self.current - 1]

    def peek(self):
        return self.tokens[self.current]

    def synchronize(self):
        while not self.is_end():
            self.advance()
            if self.previous().type == TokenType.SEMICOLON:
                return
            match self.peek().type:
                case TokenType.CLASS:
                    return
                case TokenType.FUN:
                    return
                case TokenType.VAR:
                    return
                case TokenType.FOR:
                    return
                case TokenType.IF:
                    return
                case TokenType.WHILE:
                    return
                case TokenType.RETURN:
                    return

    def error(self, token: Token, msg: str):
        self.on_error(token, msg)
        return ParseError()
