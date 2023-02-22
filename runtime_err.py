from tokens import Token


class LangRuntimeError(RuntimeError):
    def __init__(self, token: Token, msg: str):
        super().__init__(msg)
        self.token = token
