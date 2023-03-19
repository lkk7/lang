from __future__ import annotations

from typing import TYPE_CHECKING, Any

from error.runtime_err import LangRuntimeError
from parsing.tokens import Token

if TYPE_CHECKING:
    from classes.classdef import ClassObj


class InstanceObj:
    def __init__(self, class_obj: ClassObj):
        self.class_obj = class_obj
        self.fields: dict[str, Any] = {}

    def get(self, name: Token):
        if name.lexeme in self.fields:
            return self.fields[name.lexeme]
        method = self.class_obj.find_method(name.lexeme)
        if method is not None:
            return method.bind(self)
        raise LangRuntimeError(name, f"Undefined property '{name.lexeme}'")

    def set(self, name: Token, val: Any):
        self.fields[name.lexeme] = val

    def __str__(self):
        return f"<{self.class_obj.name} instance>"
