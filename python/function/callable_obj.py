from __future__ import annotations

from abc import ABC
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    from runtime.interpreter import Interpreter


class CallableObj(ABC):
    def call(self, interpreter: Interpreter, arguments: list[Any]) -> Any:
        raise NotImplementedError

    def arity(self) -> int:
        raise NotImplementedError
