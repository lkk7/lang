#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"

/** A primitive REPL with a hardcoded line length limit */
static void repl(void) {
  char line[1024];
  for (;;) {
    printf("> ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    interpret(line);
  }
}

static char* read_file(const char* path) {
  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    (void)fprintf(stderr, "Could not open file \"%s\".\n", path);
    exit(74);
  }

  (void)fseek(file, 0L, SEEK_END);
  size_t file_size = (size_t)ftell(file);
  rewind(file);

  char* buffer = (char*)malloc(file_size + 1);
  if (buffer == NULL) {
    (void)fprintf(stderr, "Could not allocate memory to read \"%s\".\n", path);
    exit(74);
  }
  size_t bytesRead = fread(buffer, sizeof(char), file_size, file);
  if (bytesRead < file_size) {
    (void)fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(74);
  }
  buffer[bytesRead] = '\0';

  (void)fclose(file);
  return buffer;
}

static void run_file(const char* path) {
  char* src = read_file(path);
  InterpretResult result = interpret(src);
  free(src);

  if (result == INTERPRET_COMPILE_ERROR) {
    exit(65);
  }
  if (result == INTERPRET_RUNTIME_ERROR) {
    exit(70);
  }
}

int main(int argc, const char* argv[]) {
  init_vm();

  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    run_file(argv[1]);
  } else {
    (void)fprintf(stderr, "Usage: lang [path]\n");
    exit(64);
  }

  free_vm();
  return 0;
}
