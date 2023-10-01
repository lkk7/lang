from typing import Any, Callable

from parsing.tokens import KEYWORDS, Token, TokenType


class Scanner:
    def __init__(self, source: str, on_error: Callable[[int, str], None]):
        self.source = source
        self.tokens: list[Token] = []
        self.start: int = 0
        self.current: int = 0
        self.line: int = 1
        self.on_error = on_error

    def scan_tokens(self):
        while not self.is_end():
            self.start = self.current
            self.scan_token()
        self.tokens.append(Token(TokenType.EOF, "", None, self.line))
        return self.tokens

    def scan_token(self):
        c = self.advance()
        match c:
            case "(":
                self.add_token(TokenType.LEFT_PAREN)
            case ")":
                self.add_token(TokenType.RIGHT_PAREN)
            case "{":
                self.add_token(TokenType.LEFT_BRACE)
            case "}":
                self.add_token(TokenType.RIGHT_BRACE)
            case ",":
                self.add_token(TokenType.COMMA)
            case ".":
                self.add_token(TokenType.DOT)
            case "-":
                self.add_token(TokenType.MINUS)
            case "+":
                self.add_token(TokenType.PLUS)
            case ":":
                self.add_token(TokenType.COLON)
            case ";":
                self.add_token(TokenType.SEMICOLON)
            case "*":
                self.add_token(TokenType.STAR)
            case "?":
                self.add_token(TokenType.QUESTION)
            case "!":
                self.add_token(
                    TokenType.BANG_EQUAL if self.match("=") else TokenType.BANG
                )
            case "=":
                self.add_token(
                    TokenType.EQUAL_EQUAL
                    if self.match("=")
                    else TokenType.EQUAL
                )
            case "<":
                self.add_token(
                    TokenType.LESS_EQUAL if self.match("=") else TokenType.LESS
                )
            case ">":
                self.add_token(
                    TokenType.GREATER_EQUAL
                    if self.match("=")
                    else TokenType.GREATER
                )
            case "/":
                next_char = self.peek()
                if next_char == "/":
                    self.current += 1
                    while self.peek() != "\n" and not self.is_end():
                        self.current += 1
                elif next_char == "*":
                    self.current += 1
                    while not self.is_end() and (
                        self.peek() != "*" or self.peek_next() != "/"
                    ):
                        if self.peek() == "\n":
                            self.line += 1
                        self.current += 1
                    if self.is_end():
                        self.on_error(self.line, "Unterminated comment")
                        return
                    self.current += 2
                else:
                    self.add_token(TokenType.SLASH)
            case " " | "\r" | "\t":
                pass
            case "\n":
                self.line += 1
            case '"':
                self.string()
            case "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9":
                self.number()
            case _:
                if c.isalpha():
                    self.identifier()
                else:
                    self.on_error(self.line, "Unexpected character")

    def advance(self):
        self.current += 1
        return self.source[self.current - 1]

    def is_end(self):
        return self.current >= len(self.source)

    def peek(self):
        if self.is_end():
            return "\0"
        return self.source[self.current]

    def peek_next(self):
        if self.current + 1 >= len(self.source):
            return "\0"
        return self.source[self.current + 1]

    def add_token(self, type: TokenType, literal: Any = None):
        lexeme = self.source[self.start : self.current]
        self.tokens.append(Token(type, lexeme, literal, self.line))

    def match(self, expected: str):
        if self.is_end():
            return False
        if self.source[self.current] != expected:
            return False
        self.current += 1
        return True

    def string(self):
        while self.peek() != '"' and not self.is_end():
            if self.peek() == "\n":
                self.line += 1
            self.current += 1

        if self.is_end():
            self.on_error(self.line, "Unterminated string")
            return

        self.current += 1
        string_val = self.source[(self.start + 1) : (self.current - 1)]
        self.add_token(TokenType.STRING, string_val)

    def number(self):
        while self.peek().isdigit():
            self.current += 1
        if self.peek() == "." and self.peek_next().isdigit():
            self.current += 1
            while self.peek().isdigit():
                self.current += 1
        self.add_token(
            TokenType.NUMBER, float(self.source[self.start : self.current])
        )

    def identifier(self):
        while self.peek().isidentifier():
            self.current += 1
        token_type = KEYWORDS.get(self.source[self.start : self.current])
        self.add_token(token_type if token_type else TokenType.IDENTIFIER)
