from __future__ import annotations

from typing import TYPE_CHECKING

from pyparsing import Any

from classes.instance import InstanceObj
from function.callable_obj import CallableObj
from function.function import FunctionObj

if TYPE_CHECKING:
    from runtime.interpreter import Interpreter


class ClassObj(CallableObj):
    def __init__(self, name: str, methods: dict[str, FunctionObj]):
        self.name = name
        self.methods = methods

    def call(self, interpreter: Interpreter, arguments: list[Any]) -> Any:
        instance = InstanceObj(self)
        initializer = self.find_method("init")
        if initializer is not None:
            initializer.bind(instance).call(interpreter, arguments)
        return instance

    def arity(self):
        initializer = self.find_method("init")
        if initializer is None:
            return 0
        return initializer.arity()

    def find_method(self, name: str):
        if name in self.methods:
            return self.methods[name]
        return None

    def __str__(self) -> str:
        return f"<class {self.name}>"
