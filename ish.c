#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>

void ish_loop(void);
char *ish_read_line(void);
char **ish_split_line(char *);
int ish_execute(char **);

int main(int argc, char **argv)
{
  // Load config files, if any

  // Run command loop
  ish_loop();

  // Perform any shutdown/cleanup
  return EXIT_SUCCESS;
}

void ish_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    line = ish_read_line();
    args = ish_split_line(line);
    status = ish_execute(args);

    free(line);
    free(args);
  } while(status);
}

#define ISH_RL_BUFSIZE 1024
char *ish_read_line(void)
{
  int bufsize = ISH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (buffer == 0) {
    fprintf(stderr, "ish: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while(1) {
    // Read a character
    c = getchar();

    // If we hit EOF, replace it with a null character and return
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate
    if (position >= bufsize) {
      bufsize += ISH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (buffer == 0) {
        fprintf(stderr, "ish: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

#define ISH_TOK_BUFSIZE 64
#define ISH_TOK_DELIM " \t\r\n\a"
char **ish_split_line(char *line)
{
  int bufsize = ISH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (tokens == 0) {
    fprintf(stderr, "ish: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, ISH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position ++;

    if (position >= bufsize) {
      bufsize += ISH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (tokens == 0) {
        fprintf(stderr, "ish: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, ISH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

int ish_launch(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("ish");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("ish");
  } else {
    // Parent process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/*
 Function declarations for builtin shell commands
 */
int ish_cd(char **args);
int ish_ls(char **args);
int ish_help(char **args);
int ish_exit(char **args);

/*
 List of builtin commands, followed by their
 corresponding functions
 */
char *builtin_str[] = {"cd", "ls", "help", "exit"};

int (*builtin_func[]) (char **) = {&ish_cd, &ish_ls, &ish_help, &ish_exit};

int ish_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
 Builtin function implementations
 */
int ish_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "ish: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("ish");
    }
  }
  return 1;
}

int ish_ls(char **args)
{
  DIR *dp;
  struct dirent *ep;

  if (args[1] == NULL) {
    dp = opendir("./");
    if (dp != NULL) {
      while ((ep = readdir(dp))) {
        puts(ep->d_name);
      }
      (void) closedir(dp);
    } else {
      perror("Couldn't open the directory");
    }
  } else {
    dp = opendir(args[1]);
    if (dp != NULL) {
      while ((ep = readdir(dp))) {
        puts(ep->d_name);
      }
      (void) closedir(dp);
    } else {
      perror("Couldn't open the directory");
    }
  }

  return 1;
}

int ish_help(char **args)
{
  int i;
  printf("Liam Sean Brady's ISH\n");
  printf("Type program names and arguments, and hit enter\n");
  printf("The following are built in:\n");

  for (i = 0; i < ish_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int ish_exit(char **args)
{
  return 0;
}

int ish_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered
    return 1;
  }

  for (i = 0; i < ish_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return ish_launch(args);
}
