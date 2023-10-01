C implementation of the Lox language specified in [Crafting Interpreters](https://craftinginterpreters.com) by Robert Nystrom.

In progress.

# Local build and development

There's an example file `CMakeUserPresets.example.json` with sane defaults.
Copy that file to `CMakeUserPresets.json` and modify anything if needed. It's not tracked by git.
There are presets for each platform, and a default "dev" one which you can modify for yourself.
To build the project, use CMake with a given preset from that file, for example:
```sh
cmake --preset=dev
cmake --build --preset=dev
```

To run tests:
```sh
ctest --preset=dev
```