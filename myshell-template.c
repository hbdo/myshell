/**
 * myshell interface program
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>


#define MAX_LINE       80 /* 80 chars per line, per command, should be enough. */
#define MAX_HIST       10
int parseCommand(char inputBuffer[], char *args[],int *background, int length);
void printArgs(char *args[]){
  int i = 0;
  while(args[i] != NULL){
    printf("%d %s\n", i, args[i]);
    i++;
  }
}
typedef struct hist {
  char command[MAX_LINE]; // command
  int commnum; // call id
  int length;
} hist_t;

hist_t HISTORY[MAX_HIST];
int histcount = 0;

int main(void)
{
  char inputBuffer[MAX_LINE]; 	        /* buffer to hold the command entered */
  int background;             	        /* equals 1 if a command is followed by '&' */
  char *args[MAX_LINE/2 + 1];	        /* command line (of 80) has max of 40 arguments */
  pid_t child;            		/* process id of the child process */
  int status;           		/* result from execv system call*/
  int shouldrun = 1;
	char path[] = "/bin/";
  int i, upper, length;

  while (shouldrun){            		/* Program terminates normally inside setup */
    background = 0;
		memset(&inputBuffer[0], 0, sizeof(inputBuffer));
     /* read what the user enters on the command line */
    do {
	  printf("myshell>");
	  fflush(stdout);
	  length = read(STDIN_FILENO,inputBuffer,MAX_LINE); 
    }
    while (inputBuffer[0] == '\n'); /* swallow newline characters */
    shouldrun = parseCommand(inputBuffer,args,&background, length);       /* get next command */
		
    if (strncmp(args[0], "exit", 4) == 0)
      shouldrun = 0;     /* Exiting from myshell*/
  
    if (shouldrun) {
    
      if(strncmp(args[0], "history", 7) == 0){
        /*
        * HISTORY FUNCTION
        *
        */
        
        if(histcount < MAX_HIST){
          for(i=histcount;i>0;i--){
            printf("%d %s\n", HISTORY[i].commnum, HISTORY[i].command);
          }
        } else {
          for(i=0;i<MAX_HIST;i++){
            printf("%d %s\n", HISTORY[(histcount-i) % MAX_HIST].commnum, HISTORY[(histcount-i) % MAX_HIST].command);
          }
        }
      } else {
         
          child = fork();
          if(child == 0){
            // Child process
            strcat(path, args[0]);
            execv(path, args);
            
            return 0;

          } else if(child<0){
            printf("Error creating child process. Shell is terminated");
            return -1;
          
          } else {
            // Parent process
            if(!background){
              wait(NULL);  
            } else {
              printf("[1] A process is created with pid %d \n", child);
            }
          }
      }


      /*
	After reading user input, the steps are 
	(1) Fork a child process using fork()
	(2) the child process will invoke execv()
	(3) if command included &, parent will invoke wait()
       */
    }
  }
  return 0;
}

/** 
 * The parseCommand function below will not return any value, but it will just: read
 * in the next command line; separate it into distinct arguments (using blanks as
 * delimiters), and set the args array entries to point to the beginning of what
 * will become null-terminated, C-style strings. 
 */

int parseCommand(char inputBuffer[], char *args[],int *background, int length)
{
    int i,		/* loop index for accessing inputBuffer array */
      start,		/* index where beginning of next command parameter is */
      ct,	        /* index of where to place the next parameter into args[] */
      command_number;	/* index of requested command number */
    char origInput[MAX_LINE];
    ct = 0;
    
//    printf("Parsing\n");
   
	
    /**
     *  0 is the system predefined file descriptor for stdin (standard input),
     *  which is the user's screen in this case. inputBuffer by itself is the
     *  same as &inputBuffer[0], i.e. the starting address of where to store
     *  the command that is read, and length holds the number of characters
     *  read in. inputBuffer is not a null terminated C-string. 
     */
      
    start = -1;
    if (length == 0){
      printf("\n");
      exit(0);            /* ^d was entered, end of user command stream */
    }
    /** 
     * the <control><d> signal interrupted the read system call 
     * if the process is in the read() system call, read returns -1
     * However, if this occurs, errno is set to EINTR. We can check this  value
     * and disregard the -1 value 
     */

    if ( (length < 0) && (errno != EINTR) ) {
      perror("error reading the command\n");
      printf("%s\n", strerror(errno));
      exit(-1);           /* terminate with error code of -1 */
    }
    
    /**
     * Parse the contents of inputBuffer
     */
    if ((strncmp(inputBuffer, "history", 7) != 0) && (strncmp(inputBuffer, "!", 1) != 0)){
      hist_t newcomm;
      strcpy(newcomm.command, inputBuffer);
      newcomm.command[strlen(newcomm.command)] = '\0';
      newcomm.commnum = (++histcount);
      newcomm.length = length;
      HISTORY[newcomm.commnum % MAX_HIST] = newcomm;
    }
     
    strcpy(origInput, inputBuffer);
     
    
    for (i=0;i<length;i++) { 
      /* examine every character in the inputBuffer */
      
      switch (inputBuffer[i]){
      case ' ':
      case '\t' :               /* argument separators */
	if(start != -1){
	  args[ct] = &inputBuffer[start];    /* set up pointer */
	  ct++;
	}
	inputBuffer[i] = '\0'; /* add a null char; make a C string */
	start = -1;
	break;
	
      case '\n':                 /* should be the final char examined */
	if (start != -1){
	  args[ct] = &inputBuffer[start];     
	  ct++;
	}
	inputBuffer[i] = '\0';
	args[ct] = NULL; /* no more arguments to this command */
	break;
	
      default :             /* some other character */
	if (start == -1)
	  start = i;
	if (inputBuffer[i] == '&') {
	  *background  = 1;
	  inputBuffer[i-1] = '\0';
	}
      } /* end of switch */
    }    /* end of for */
    
    /**
     * If we get &, don't enter it in the args array
     */
    
    if (*background)
      args[--ct] = NULL;
    
    args[ct] = NULL; /* just in case the input line was > 80 */

    if ((strncmp(inputBuffer, "!!", 2) == 0)){
      memset(&inputBuffer[0], 0, MAX_LINE);
      printf("%d %s\n", (histcount)%MAX_HIST, HISTORY[(histcount)%MAX_HIST].command);
      strcpy(inputBuffer, HISTORY[(histcount)%MAX_HIST].command);
      //printf("%s\n", inputBuffer);
      //printArgs(args);
      return parseCommand(inputBuffer, args, background, HISTORY[histcount%MAX_HIST].length);
    }
    
    
    return 1;
    
} /* end of parseCommand routine */

