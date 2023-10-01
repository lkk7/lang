from __future__ import annotations

from typing import Any, Optional

from error.runtime_err import LangRuntimeError
from parsing.tokens import Token


class Environment:
    def __init__(self, enclosing: Optional[Environment] = None):
        super().__init__()
        self.enclosing = enclosing
        self.values: dict[str, Any] = {}

    def define(self, name: str, value: Any):
        self.values[name] = value

    def assign(self, name: Token, val: Any):
        if name.lexeme in self.values:
            self.values[name.lexeme] = val
            return
        if self.enclosing is not None:
            self.enclosing.assign(name, val)
            return
        raise LangRuntimeError(name, f"Undefined variable {name.lexeme}")

    def get_ancestor(self, distance: int):
        env: Environment | None = self
        for _ in range(distance):
            env = env.enclosing if env is not None else None
        return env

    def assign_at(self, distance: int, name: Token, val: Any):
        self.get_ancestor(distance).values[name.lexeme] = val

    def get(self, key: Token):
        try:
            return self.values[key.lexeme]
        except KeyError:
            if self.enclosing is not None:
                return self.enclosing.values[key.lexeme]
            raise LangRuntimeError(key, f"Undefined variable '{key.lexeme}'")

    def get_at(self, distance: int, name: Token):
        return self.get_ancestor(distance).values[name.lexeme]

    def get_sure(self, key: str):
        """Get an item by key WITHOUT any error handling."""
        return self.values[key]
