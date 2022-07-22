#define _POSIX_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>

#define MAX_LINE 80 /* The maximum length command */
#define HIST_SIZE 5 /* The history size */

/* global variable for dealing with history feature */
char history[HIST_SIZE][MAX_LINE];
char histCount = 0;

void printHistory() {
  for (int i = 0; i < HIST_SIZE; i++) {
        int hisNum = histCount - i;
        if (hisNum <= 0) {
          break;
        }
        printf("%d  ", hisNum);
        int j = 0;
        while (history[i][j] != '\n' && history[i][j] != '\0') {
          printf("%c", history[i][j]);
          j++;
        }
        printf("\n");
  }
  printf("\n");
}

int translate(char userInput[], char* args[], int* isBackground, int isHistoryCall) {
  // do this only if NOT called by a history command (i.e. not "!!" or "! [num]")
  if (isHistoryCall == 0) {
    if (strncmp(userInput, "!!\n", 3) == 0) {
      if (histCount == 0) {
        fprintf(stderr, "ERROR: No commands in history.\n");
        return -1;
      }
      //************** "!!" command case **************
      strcpy(userInput, history[0]);
      /* instead call translate on the historic userInput */
      translate(userInput, args, &isBackground, 1);
      /* then just return */
      return 0;
    } else if (strncmp(userInput, "! [num]", 2) == 0) {
      //************** "! [num]" case **************
      /* scan for num following "!" */
      char* p = userInput;
      int num;
      while (*p) { /* while there are more characters to process */
        if (isdigit(*p)) {
          num = strtol(p, &p, 10);
        } else {
          p++;
        }
      } 
      /* now, the "num" holds the hist num */
      int isFind = 0;
      /* search the history buffer for corresponding history num */
      for (int id = 0; id < HIST_SIZE; id++) {
        int hisNum = histCount - id;
        if (hisNum == num) {
          strcpy(userInput, history[id]);
          translate(userInput, args, &isBackground, 1);

          /* place the command in the history buffer as the next command. */
          char temp[sizeof(history[id])];
          strcpy(temp, history[id]);
          for(int i = (HIST_SIZE - 1); i > 0; i--) {
              strcpy(history[i], history[i-1]);
          }
          strcpy(history[0], temp);
          histCount++;
          return 0;
        }
      }
      
      if (isFind == 0) {
        fprintf(stderr, "ERROR: No such command in history.\n");
        return -1;
      }

    } else {
      //************** normal command case **************
      /* shift history buffer to the right by one */
      for(int i = (HIST_SIZE - 1); i > 0; i--) {
          strcpy(history[i], history[i-1]);
      }
      /* append current command to the most left */
      strcpy(history[0], userInput);
      histCount++;
    } 
  }

  int inputLen = strlen(userInput);

  /* loop through the userInput and put args into args[] */
  int uPointer = -1; /* index of the start of next command in userInput */ /* -1 indicates current unknown */
  int aPointer = 0; /* index where the next command should be put in args[] */
  for (int i = 0; i < inputLen; i++) {
    switch (userInput[i]) {
      // separtor case
      case '\t':
      case ' ':
        if (uPointer != -1) {
          args[aPointer] = &userInput[uPointer];
          aPointer++;
        }
        userInput[i] = '\0';
        uPointer = -1;
        break;

      // backgroud case
      case '&':
        *isBackground = 1;
        userInput[i] = '\0';
        break;

      // termination case
      case '\n':
        if (uPointer != -1) {
          args[aPointer] = &userInput[uPointer];
          aPointer++;
        }
        userInput[i] = '\0';
        args[aPointer] = NULL;
        break;

      // normal character case
      default:
        if (uPointer == -1) {
          uPointer = i;
        }
        break;
    }
    args[aPointer] = NULL;
  }

  // **************** test code ****************
  // printf("------- Arguments ---------\n");
  // for (int i = 0; i < sizeof(args); i++) {
  //   printf(args[i]);
  //   printf("\n");
  // }
  // printf("---------------------------\n");

  return 0;
}

int main(void) {
  char userInput[MAX_LINE];
  char* args[MAX_LINE/2 + 1]; /* command line arguments */
  int should_run = 1; /* flag to determine when to exit program */
  int isBackground; /* flag to determine whether the parent process should run in background */
  int c_status = 1;

  memset(userInput, '\0', sizeof(userInput));

  while (should_run) {
    isBackground = 0;
    printf("osh>");
    fflush(stdout);
    /* read from stdin to userInput */
    for (int i = 0; i < MAX_LINE; i++) {
      userInput[i] = '\0';
    }
    read(STDIN_FILENO, userInput, MAX_LINE);

    /* detect special command */
    if (strncmp(userInput, "exit\n", 5) == 0) {
      exit(0);
    }
    if (strncmp(userInput, "history\n", 8) == 0) {
      printHistory();
      fflush(stdout);
      continue;
    }

    int error = translate(userInput, args, &isBackground, 0);

    // handle error
    if (error != 0) {continue;}
  
    pid_t pid = fork();
    if (pid < 0) {
      fprintf(stderr, "ERROR: fork() fails.\n");
      exit(-1);
    } else if (pid == 0) {
      int status;
      printf("%d is running",getpid());
      fflush(stdout);
      status = execvp(*args, args);
      if (status == -1) {
        printf("%d is running",getpid());
        fflush(stdout);
        fprintf(stderr, "ERROR: execvp() fails.\n");
        exit(-1);
      }
      return 0;
    } else {
      if (isBackground == 0) {
        printf("waiting for %d\n", pid);
        fflush(stdout);
        waitpid(pid, &c_status, WNOHANG);
      }
    }
  }  

  return 0;
}