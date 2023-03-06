from time import time
from typing import Any

from callable_obj import CallableObj
from interpreter import Interpreter


class Clock(CallableObj):
    def arity(self) -> int:
        return 0

    def call(self, interpreter: Interpreter, arguments: list[Any]) -> Any:
        return time()

    def __str__(self):
        return "<native fn>"
