from __future__ import annotations

from typing import TYPE_CHECKING, Any

from asts.ast_defs import FunctionStmt
from error.return_val import ReturnVal
from function.callable_obj import CallableObj
from scope.environment import Environment

if TYPE_CHECKING:
    from runtime.interpreter import Interpreter


class FunctionObj(CallableObj):
    def __init__(self, declaration: FunctionStmt, closure: Environment):
        self.declaration = declaration
        self.closure = closure

    def arity(self) -> int:
        return len(self.declaration.params)

    def call(self, interpreter: Interpreter, arguments: list[Any]) -> Any:
        env = Environment(self.closure)
        for i, param in enumerate(self.declaration.params):
            env.define(param.lexeme, arguments[i])
        try:
            interpreter.execute_block(self.declaration.body, env)
        except ReturnVal as ret_val:
            return ret_val.val
        return None

    def __str__(self) -> str:
        return f"<fn {self.declaration.name.lexeme}>"