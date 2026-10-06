#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <fcntl.h>
#include <unistd.h>

#include "builtin_commands.h"
#include "process_dispatch.h"
#include "string_helpers.h"

#define BUFSIZE 100
#define TOKENCOUNT 256

typedef struct {
  int child_fd;
  int input_fd;
  bool is_interactive;
} MYSH_Options;

static MYSH_Options getOptions(int argc, char** argv) {
  MYSH_Options options = {
    .child_fd = -1 ,
    .input_fd = 0 ,
    .is_interactive = false ,
  };
  if (argc > 2) {
    fprintf(stderr, "Too many arguments provided.\n");
    exit(EXIT_SUCCESS);
  }
  if (argc == 2) {
    options.input_fd = open(argv[1], O_RDONLY);
    if (options.input_fd < 0) {
      perror(argv[1]);
      exit(EXIT_FAILURE);
    }
    options.child_fd = open("/dev/null", O_RDONLY);
  } else {
    if (isatty(STDIN_FILENO)) {
      options.is_interactive = true;
    } else {
      options.child_fd = open("/dev/null", O_RDONLY);
    }
  }
  return options;
}

static void prompt(void) {
  char* cwd = getcwd(NULL, 0);
  if (cwd == NULL) {
    return;
  }
  char* home = getenv("HOME");
  if (startsWith(cwd, home)) {
    printf("~%s$ ", cwd + strlen(home));
  } else {
    printf("%s$ ", cwd);
  }
  fflush(stdout);
  free(cwd);
}

static int tokenize(const char* line, char** tokens, int max) {
  int ingested = 0;
  const char* ptr = line;
  while (*ptr && ingested < max) {
    while (*ptr == ' ' || *ptr == '\t') {
      ++ptr;
    }
    if (*ptr == '\0' || *ptr == '#') {
      break;
    }
    if (*ptr == '<' || *ptr == '>' || *ptr == '|') {
      tokens[ingested++] = strndup(ptr, 1);
      ++ptr;
    } else {
      const char* start = ptr;
      while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '#' && *ptr != '<' && *ptr != '>' && *
             ptr != '|') {
        ++ptr;
      }
      tokens[ingested++] = strndup(start, ptr - start);
    }
  }
  return ingested;
}

static void globber(const char* token, StrVec* strv) {
  if (strchr(token, '*') == NULL) {
    StrVec_Push(strv, token);
    return;
  }
  glob_t results;
  int result = glob(token, GLOB_NOCHECK, NULL, &results);
  if (result != 0) {
    StrVec_Push(strv, token);
    return;
  }
  for (size_t i = 0; i < results.gl_pathc; ++i) {
    StrVec_Push(strv, results.gl_pathv[i]);
  }
  globfree(&results);
}

static void build_argv(char** tokens, int start, int end, StrVec* argv) {
  for (int i = start; i < end; ++i) {
    if (strchr(tokens[i], '*')) {
      globber(tokens[i], argv);
    } else {
      StrVec_Push(argv, tokens[i]);
    }
  }
}

static int run_command(char** args, int argc, const char* redirect_in, const char* redirect_out,
                       MYSH_Options* options) {
  if (argc == 0) {
    return 0;
  }
  enum MYSHCommandType type = commandToEnum(args[0]);
  if (type == MYSH_EXIT) {
    return -1;
  }
  if (type == MYSH_CD) {
    if (argc > 2) {
      fprintf(stderr, "Too many arguments provided to cd\n");
      return 1;
    }
    return cd((argc >= 2) ? args[1] : NULL);
  }
  if (type == MYSH_PWD && redirect_out == NULL && redirect_in == NULL) {
    return pwd();
  }
  if (type == MYSH_WHICH && redirect_out == NULL && redirect_in == NULL) {
    if (argc != 2) {
      fprintf(stderr, "Incorrect number of arguments provided to which\n");
      return 1;
    }
    return which(args[1]);
  }
  char* executable_path = NULL;
  if (type == MYSH_EXECUTABLE) {
    executable_path = strchr(args[0], '/') ? strdup(args[0]) : findExecutable(args[0]);
    if (executable_path == NULL) {
      fprintf(stderr, "%s: command not found\n", args[0]);
      return 1;
    }
  }
  int fd_in = -1;
  int fd_out = -1;
  if (redirect_in) {
    fd_in = open(redirect_in, O_RDONLY);
    if (fd_in < 0) {
      perror(redirect_in);
      free(executable_path);
      return 1;
    }
  }
  if (redirect_out) {
    fd_out = open(redirect_out, O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (fd_out < 0) {
      perror(redirect_out);
      if (fd_in >= 0) {
        close(fd_in);
      }
      free(executable_path);
      return 1;
    }
  }
  int child_in;
  if (fd_in >= 0) {
    child_in = fd_in;
  } else if (redirect_out != NULL || options->child_fd >= 0) {
    child_in = (options->child_fd >= 0) ? options->child_fd : open("/dev/null", O_RDONLY);
  } else {
    child_in = STDIN_FILENO;
  }
  int child_out = (fd_out >= 0) ? fd_out : STDOUT_FILENO;
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    free(executable_path);
    if (fd_in >= 0) {
      close(fd_in);
    }
    if (fd_out >= 0) {
      close(fd_out);
    }
    return 1;
  }
  if (pid == 0) {
    if (child_in != STDIN_FILENO) {
      dup2(child_in, STDIN_FILENO);
      close(child_in);
    }
    if (child_out != STDOUT_FILENO) {
      dup2(child_out, STDOUT_FILENO);
      close(child_out);
    }
    if (type == MYSH_PWD) {
      pwd();
      exit(EXIT_SUCCESS);
    }
    if (type == MYSH_WHICH) {
      if (argc != 2) {
        exit(EXIT_SUCCESS);
      }
      exit(which(args[1]));
    }
    execv(executable_path, args);
    perror("exec");
    exit(EXIT_SUCCESS);
  }
  if (fd_in >= 0) {
    close(fd_in);
  }
  if (fd_out >= 0) {
    close(fd_out);
  }
  free(executable_path);
  return waitAndPrintExitStatus(pid, options->is_interactive);
}

static int parse_simple_command(char** tokens, int num_tokens, MYSH_Options* options) {
  StrVec argv;
  StrVec_Init(&argv);
  const char* redirect_in = NULL;
  const char* redirect_out = NULL;
  int syntax_err = 0;
  for (int i = 0; i < num_tokens; ++i) {
    if (strcmp(tokens[i], "<") == 0) {
      ++i;
      if (i >= num_tokens || strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 ||
          strcmp(tokens[i], "|") == 0) {
        fprintf(stderr, "Syntax error in command\n");
        syntax_err = 1;
        break;
      }
      redirect_in = tokens[i];
    } else if (strcmp(tokens[i], ">") == 0) {
      ++i;
      if (i >= num_tokens || strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 ||
          strcmp(tokens[i], "|") == 0) {
        fprintf(stderr, "Syntax error in command\n");
        syntax_err = 1;
        break;
      }
      redirect_out = tokens[i];
    } else if (strchr(tokens[i], '*')) {
      globber(tokens[i], &argv);
    } else {
      StrVec_Push(&argv, tokens[i]);
    }
  }
  int result;
  if (syntax_err) {
    result = 1;
  } else {
    StrVec_nullTerminate(&argv);
    result = run_command(argv.data, (int)argv.size, redirect_in, redirect_out, options);
  }
  StrVec_freeAll(&argv);
  return result;
}

static int execute_pipeline(char** tokens, const int* pipe_positions, int num_pipes, int num_tokens,
                            MYSH_Options* options) {
  int starts[TOKENCOUNT];
  int ends[TOKENCOUNT];
  int num_commands = 0;
  int s = 0;
  for (int p = 0; p < num_pipes; ++p) {
    starts[num_commands] = s;
    ends[num_commands] = pipe_positions[p];
    num_commands++;
    s = pipe_positions[p] + 1;
  }
  starts[num_commands] = s;
  ends[num_commands] = num_tokens;
  num_commands++;
  Pipeline stages[TOKENCOUNT];
  StrVec argvs[TOKENCOUNT];
  int found_exit = 0;
  int build_err = 0;
  for (int c = 0; c < num_commands; ++c) {
    StrVec_Init(&argvs[c]);
    build_argv(tokens, starts[c], ends[c], &argvs[c]);
    if (argvs[c].size == 0) {
      fprintf(stderr, "Syntax error in pipeline\n");
      build_err = 1;
      break;
    }
    enum MYSHCommandType type = commandToEnum(argvs[c].data[0]);
    if (type == MYSH_EXIT) {
      found_exit = 1;
    }
    if (type == MYSH_EXECUTABLE) {
      char* executable_path = strchr(argvs[c].data[0], '/')
                                ? strdup(argvs[c].data[0])
                                : findExecutable(argvs[c].data[0]);
      if (executable_path == NULL) {
        fprintf(stderr, "%s: command not found\n", argvs[c].data[0]);
        build_err = 1;
        break;
      }
      free(argvs[c].data[0]);
      argvs[c].data[0] = executable_path;
    }
    StrVec_nullTerminate(&argvs[c]);
    stages[c].subprocess.executable = argvs[c].data[0];
    stages[c].subprocess.args = argvs[c].data;
    stages[c].next = (c < num_commands - 1) ? &stages[c + 1] : NULL;
  }
  int result;
  if (build_err) {
    result = 1;
  } else {
    int child_in = (options->child_fd >= 0) ? options->child_fd : STDIN_FILENO;
    result = executePipeline(&stages[0], child_in, STDOUT_FILENO, options->is_interactive);
    if (found_exit) {
      result = -1;
    }
  }
  for (int c = 0; c < num_commands; ++c) {
    StrVec_freeAll(&argvs[c]);
  }
  return result;
}

static int execute_line(const char* line, MYSH_Options* options) {
  char* tokens[TOKENCOUNT];
  int num_tokens = tokenize(line, tokens, TOKENCOUNT);
  if (num_tokens == 0) {
    return 0;
  }
  int pipe_positions[TOKENCOUNT];
  int num_pipes = 0;
  for (int i = 0; i < num_tokens; ++i) {
    if (strcmp(tokens[i], "|") == 0) {
      pipe_positions[num_pipes++] = i;
    }
  }
  int result;
  if (num_pipes == 0) {
    result = parse_simple_command(tokens, num_tokens, options);
  } else {
    result = execute_pipeline(tokens, pipe_positions, num_pipes, num_tokens, options);
  }
  free_tokens(tokens, num_tokens);
  return result;
}

int main(int argc, char** argv) {
  MYSH_Options options = getOptions(argc, argv);
  if (options.is_interactive) {
    printf("Welcome to our shell!\n /\\_/\\\n( o.o )\n > ^ <\n");
    prompt();
  }
  char buf[BUFSIZE];
  char* line = NULL;
  size_t len = 0;
  bool exit_flag = false;
  size_t bytes_read;
  while (!exit_flag && (bytes_read = read(options.input_fd, buf, BUFSIZE)) > 0) {
    size_t start = 0;
    for (size_t pos = 0; pos < bytes_read; ++pos) {
      if (buf[pos] == '\n') {
        size_t seg_len = pos - start;
        char* tmp = realloc(line, len + seg_len + 1);
        assert(tmp);
        line = tmp;
        memmove(line + len, buf + start, seg_len);
        len += seg_len;
        line[len] = '\0';
        start = pos + 1;
        int rc = execute_line(line, &options);
        free(line);
        line = NULL;
        len = 0;
        if (rc == -1) {
          exit_flag = true;
          break;
        }
        if (options.is_interactive) {
          prompt();
        }
      }
    }
    if (!exit_flag && start < bytes_read) {
      size_t seg_len = bytes_read - start;
      char* tmp = realloc(line, len + seg_len + 1);
      assert(tmp);
      line = tmp;
      memmove(line + len, buf + start, seg_len);
      len += seg_len;
      line[len] = '\0';
    }
  }
  if (!exit_flag && line != NULL && len > 0) {
    execute_line(line, &options);
  }
  free(line);
  if (options.is_interactive) {
    printf("mysh: exiting\n");
  }
  if (options.child_fd >= 0) {
    close(options.child_fd);
  }
  if (options.input_fd != STDIN_FILENO) {
    close(options.input_fd);
  }
  return EXIT_SUCCESS;
}
