from __future__ import annotations

from typing import Any, Optional

from runtime_err import LangRuntimeError
from tokens import Token


class Environment(dict):
    def __init__(self, enclosing: Optional[Environment] = None):
        super().__init__()
        self.enclosing = enclosing

    def define(self, name: str, value: Any):
        self[name] = value

    def assign(self, name: Token, val: Any):
        if name.lexeme in self:
            self[name.lexeme] = val
            return
        if self.enclosing is not None:
            self.enclosing.assign(name, val)
            return
        raise LangRuntimeError(name, f"Undefined variable {name.lexeme}")

    def __getitem__(self, key: Token):
        try:
            return super().__getitem__(key.lexeme)
        except KeyError:
            if self.enclosing is not None:
                return self.enclosing[key]
            raise LangRuntimeError(key, f"Undefined variable '{key.lexeme}'")
