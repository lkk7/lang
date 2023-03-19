from __future__ import annotations

from typing import TYPE_CHECKING, Any

from asts.ast_defs import FunctionStmt
from classes.instance import InstanceObj
from error.return_val import ReturnVal
from function.callable_obj import CallableObj
from scope.environment import Environment

if TYPE_CHECKING:
    from runtime.interpreter import Interpreter


class FunctionObj(CallableObj):
    def __init__(
        self,
        declaration: FunctionStmt,
        closure: Environment,
        is_initializer: bool,
    ):
        self.declaration = declaration
        self.closure = closure
        self.is_initializer = is_initializer

    def arity(self) -> int:
        return len(self.declaration.params)

    def call(self, interpreter: Interpreter, arguments: list[Any]) -> Any:
        env = Environment(self.closure)
        for i, param in enumerate(self.declaration.params):
            env.define(param.lexeme, arguments[i])
        try:
            interpreter.execute_block(self.declaration.body, env)
        except ReturnVal as ret_val:
            return (
                ret_val.val
                if not self.is_initializer
                else self.closure.get_this()
            )
        if self.is_initializer:
            return self.closure.get_this()
        return None

    def bind(self, instance: InstanceObj):
        env = Environment(self.closure)
        env.define("this", instance)
        return FunctionObj(self.declaration, env, self.is_initializer)

    def __str__(self) -> str:
        return f"<fn {self.declaration.name.lexeme}>"
