import sys

from error.runtime_err import LangRuntimeError
from parsing.parse import Parser
from parsing.scanner import Scanner
from parsing.tokens import Token, TokenType
from runtime.interpreter import Interpreter
from scope.resolver import Resolver


class Lang:
    def __init__(self) -> None:
        self.had_error: bool = False
        self.had_runtime_error: bool = False

    def run_file(self, filename: str):
        with open(filename, "r") as file:
            source = file.read()

        scanner = Scanner(source, self.err)
        tokens = scanner.scan_tokens()
        parser = Parser(tokens, self.err_token)
        statements = parser.parse()

        if self.had_error:
            sys.exit(65)

        interpreter = Interpreter(self.runtime_err)
        resolver = Resolver(interpreter, self.err_token)
        resolver.resolve_stmts(statements)

        if self.had_error:
            sys.exit(65)

        interpreter.interpret(statements)

        if self.had_runtime_error:
            sys.exit(70)

    def run_prompt(self):
        interpreter = Interpreter(self.runtime_err)
        resolver = Resolver(interpreter, self.err_token)

        code: str = ""
        for line in sys.stdin:
            if line[-2:] == " \n":
                code += line
                continue
            else:
                code += line

            self.had_error = False

            statements = Parser(
                Scanner(code, self.err).scan_tokens(), self.err_token
            ).parse()

            if self.had_error:
                code = ""
                continue

            resolver.resolve_stmts(statements)

            if self.had_error:
                code = ""
                continue

            interpreter.interpret(statements)
            code = ""

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
