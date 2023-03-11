from __future__ import annotations

from time import time
from typing import TYPE_CHECKING, Any

from function.callable_obj import CallableObj

if TYPE_CHECKING:
    from runtime.interpreter import Interpreter


class Clock(CallableObj):
    def arity(self) -> int:
        return 0

    def call(self, interpreter: Interpreter, arguments: list[Any]) -> Any:
        return time()

    def __str__(self):
        return "<native fn>"
