from function.callable_obj import CallableObj


class NativeFunc(CallableObj):
    def __init__(self, name: str):
        self.name = name

    def __str__(self):
        return f"<native function '{self.name}'>"
