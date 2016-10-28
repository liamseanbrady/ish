#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

char **ish_split_line(char *line);

int main()
{
  char my_string[16] = "Hello from butt";
  ish_split_line(my_string);
  return 0;
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
    printf("Token is %p\n", token);
    tokens[position] = token;
    position ++;
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
