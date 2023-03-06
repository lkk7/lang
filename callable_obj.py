from __future__ import annotations
from abc import ABC
from typing import Any, TYPE_CHECKING


if TYPE_CHECKING:
    from interpreter import Interpreter


class CallableObj(ABC):
    def call(self, interpreter: Interpreter, arguments: list[Any]) -> Any:
        raise NotImplementedError

    def arity(self) -> int:
        raise NotImplementedError
