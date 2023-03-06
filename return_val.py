from typing import Any


class ReturnVal(RuntimeError):
    def __init__(self, val: Any):
        self.val = val
