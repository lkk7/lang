import sys

from interpreter import Interpreter
from parse import Parser
from resolver import Resolver
from runtime_err import LangRuntimeError
from scanner import Scanner
from tokens import Token, TokenType


class Lang:
    def __init__(self) -> None:
        self.had_error: bool = False
        self.had_runtime_error: bool = False

    def run_file(self, filename: str):
        with open(filename, "r") as file:
            bytes = file.read()
        self.run(bytes)
        if self.had_error:
            sys.exit(65)
        if self.had_runtime_error:
            sys.exit(70)

    def run_prompt(self):
        for line in sys.stdin:
            self.run(line.rstrip())
            self.had_error = False
            self.had_runtime_error = False

    def run(self, source: str):
        scanner = Scanner(source, self.err)
        tokens = scanner.scan_tokens()
        parser = Parser(tokens, self.err_token)
        statements = parser.parse()

        if self.had_error:
            return

        interpreter = Interpreter(self.runtime_err)
        resolver = Resolver(interpreter, self.err_token)
        resolver.resolve_stmts(statements)

        if self.had_error:
            return

        interpreter.interpret(statements)

    def err(self, line: int, msg: str):
        self.report_err(line, "", msg)

    def err_token(self, token: Token, msg: str):
        if token.type == TokenType.EOF:
            self.report_err(token.line, "at EOF", msg)
        else:
            self.report_err(token.line, f"at '{token.lexeme}'", msg)

    def runtime_err(self, err: LangRuntimeError):
        print(f"{str(err)}\n[line {err.token.line}]")
        self.had_runtime_error = True

    def report_err(self, line: int, where: str, msg: str):
        print(f"[line {line}] Error ({where}): {msg}", file=sys.stderr)
        self.had_error = True


if __name__ == "__main__":
    if len(sys.argv) > 2:
        print("Usage: lang [script]")
        sys.exit(64)
    lang = Lang()
    if len(sys.argv) == 2:
        lang.run_file(sys.argv[1])
    else:
        lang.run_prompt()
