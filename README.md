Python implementation of the Lox language specified in [Crafting Interpreters](https://craftinginterpreters.com) by Robert Nystrom.

### Usage

To run the interpreter, use Python to execute `main.py` with a source code file as an argument:

```
python main.py examples/clock.lang
```

The interpreter also runs in interactive mode. End the line with a space to keep the input open for another line.

```
>>> python3 main.py
>>> print 10 + 10;
20.0
>>> fun f(x) {
>>>   return x * 10;
>>> }
>>> print f(10);
100.0
```

The interpreter will execute the code in the specified file and display any output or errors.

### Project Structure

- `asts/`: defining and generating abstract syntax tree node definitions,
- `classes/`: classes and instances,
- `error/`: error definitions,
- `examples/`: example code demonstrating the syntax and usage,
- `function/`: functions and callable objects,
- `lib/`: "standard library",
- `parsing/`: lexical analysis and parsing,
- `runtime/`: interpreting and execution,
- `scope`/: variable scoping and resolution.
