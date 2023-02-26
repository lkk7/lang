from typing import Callable
from tokens import Token, TokenType
from ast_defs import (Assign, BlockStmt, Binary, Grouping, Literal, Stmt,
                      Unary, Ternary, Variable,
                      PrintStmt, ExpressionStmt, VarStmt)


class ParseError(SyntaxError):
    pass


class Parser:
    def __init__(self, tokens: list[Token],
                 on_error: Callable[[Token, str], None]):
        self.tokens = tokens
        self.on_error = on_error
        self.current = 0

    def parse(self):
        statements: list[Stmt] = []
        while not self.is_end():
            statements.append(self.declaration())
        return statements

    def declaration(self):
        try:
            if self.match(TokenType.VAR):
                return self.var_declaration()
            return self.statement()
        except ParseError:
            self.synchronize()
            return None

    def var_declaration(self):
        name = self.consume(TokenType.IDENTIFIER, "Expected variable name")
        initializer = None
        if self.match(TokenType.EQUAL):
            initializer = self.expression()
        self.consume(TokenType.SEMICOLON,
                     "Expected ';' after variable declaration")
        return VarStmt(name, initializer)

    def statement(self):
        if self.match(TokenType.PRINT):
            return self.print_statement()
        if self.match(TokenType.LEFT_BRACE):
            return BlockStmt(self.block())
        return self.expr_statement()

    def block(self):
        statements = []
        while not self.check(TokenType.RIGHT_BRACE) and not self.is_end():
            statements.append(self.declaration())
        self.consume(TokenType.RIGHT_BRACE, "Expected '}' at after block")
        return statements

    def expr_statement(self):
        expr = self.expression()
        self.consume(TokenType.SEMICOLON, "Expected ';' after expression")
        return ExpressionStmt(expr)

    def print_statement(self):
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
        expr = self.equality()
        if self.match(TokenType.QUESTION):
            operator = self.previous()
            if_true = self.ternary()
            self.consume(TokenType.COLON,
                         "Expected ':' in the ternary '?:' operator")
            if_false = self.ternary()
            return Ternary(operator, expr, if_true, if_false)
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
        while self.match(TokenType.GREATER,
                         TokenType.GREATER_EQUAL,
                         TokenType.LESS,
                         TokenType.LESS_EQUAL):
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
        return self.primary()

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
            self.consume(TokenType.RIGHT_PAREN,
                         "Expected ')' after expression")
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
        self.advance()
        while not self.is_end():
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
