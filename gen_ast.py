expressions: dict[str, list[str]] = {
    "Ternary": [
        "operator: Token",
        "first: Expr",
        "second: Expr",
        "third: Expr",
    ],
    "Binary": ["left: Expr", "operator: Token", "right: Expr"],
    "Grouping": ["expression: Expr"],
    "Literal": ["value: Any"],
    "Unary": ["operator: Token", "right: Expr"],
    "Variable": ["name: Token"],
    "Assign": ["name: Token", "value: Expr"],
    "Logical": ["left: Expr", "operator: Token", "right: Expr"],
    "Call": ["callee: Expr", "paren: Token", "arguments: tuple[Expr, ...]"],
}

statements: dict[str, list[str]] = {
    "BlockStmt": ["statements: tuple[Stmt, ...]"],
    "ExpressionStmt": ["expression: Expr"],
    "PrintStmt": ["expression: Expr"],
    "VarStmt": ["name: Token", "initializer: Expr"],
    "IfStmt": ["condition: Expr", "then_branch: Stmt", "else_branch: Stmt"],
    "WhileStmt": ["condition: Expr", "body: Stmt"],
    "FunctionStmt": [
        "name: Token",
        "params: tuple[Token, ...]",
        "body: tuple[Stmt, ...]",
    ],
    "ReturnStmt": ["keyword: Token", "val: Expr"],
}

setup_lines = """# Autogenerated by gen_ast.py
from __future__ import annotations
from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Any
from tokens import Token


class Expr(ABC):
    @abstractmethod
    def accept(self, visitor: ExprVisitor):
        raise NotImplementedError


class Stmt(ABC):
    @abstractmethod
    def accept(self, visitor: StmtVisitor):
        raise NotImplementedError
"""


class AstGenerator:
    def __init__(self) -> None:
        self.lines: list[str] = [setup_lines]

    def gen_visitors(self, name: str, specs: dict[str, list[str]]):
        self.lines.append(f"\n\nclass {name}Visitor(ABC):")
        for expr in specs:
            self.lines.append(
                "\n    @abstractmethod\n"
                f"    def visit_{expr.lower()}(self, {name.lower()}: {expr}):"
                "\n        raise NotImplementedError\n"
            )

    def gen_ast(self, name: str, specs: dict[str, list[str]]):
        for expr, fields in specs.items():
            expr_definition = (
                "\n\n@dataclass(eq=True, frozen=True)\n"
                f"class {expr}({name}):\n"
            )
            for field in fields:
                expr_definition += f"    {field}\n"
            expr_definition += (
                f"\n    def accept(self, visitor: {name}Visitor):"
                f"\n        return visitor.visit_{expr.lower()}(self)\n"
            )
            self.lines.append(expr_definition)

    def output(self, filename):
        with open(filename, "w") as file:
            file.write("".join(self.lines))


if __name__ == "__main__":
    generator = AstGenerator()
    generator.gen_visitors("Expr", expressions)
    generator.gen_visitors("Stmt", statements)
    generator.gen_ast("Expr", expressions)
    generator.gen_ast("Stmt", statements)
    generator.output("ast_defs.py")
