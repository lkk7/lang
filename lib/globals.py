from __future__ import annotations

from typing import TYPE_CHECKING, Any

from lib.native_func import NativeFunc


if TYPE_CHECKING:
    from runtime.interpreter import Interpreter


class PrintGlobals(NativeFunc):
    def arity(self) -> int:
        return 0

    def call(self, interpreter: Interpreter, arguments: list[Any]) -> Any:
        for name, obj in interpreter.globals.values.items():
            print(f"{name}: {interpreter.stringify(obj)}")
