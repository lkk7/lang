from __future__ import annotations

from typing import Any, Optional

from error.runtime_err import LangRuntimeError
from parsing.tokens import Token


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

    def assign_at(self, distance: int, name: Token, val: Any):
        self.get_ancestor(distance)[name.lexeme] = val

    def get_at(self, distance: int, name: Token):
        return self.get_ancestor(distance)[name]

    def get_ancestor(self, distance: int):
        env: Environment | None = self
        for _ in range(distance):
            env = env.enclosing if env is not None else None
        return env

    def __getitem__(self, key: Token):
        try:
            return super().__getitem__(key.lexeme)
        except KeyError:
            if self.enclosing is not None:
                return self.enclosing[key]
            raise LangRuntimeError(key, f"Undefined variable '{key.lexeme}'")
