/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: fps.c - An application that will fork twice to run 3 processes, use pipes to
-- communicate and use signals to end normal processing.
--
-- PROGRAM: fps (forks, pipes and signals) 
--
-- FUNCTIONS:
-- int main (int argc, char *argv[]) 
-- void inputProcess();
-- void outputProcess();
-- void translateProcess();
--
--
-- DATE: January 22, 2020
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Justin Cervantes
--
-- PROGRAMMER: Justin Cervantes
--
-- NOTES:
-- The program will create 2 additional processes. The processes will communicate with each other
-- using pipes. The program starts by initially disabling all Linux terminal processing. One process 
-- runs an inputProcess loop which gathers what has been typed, then pipes the information using two different
-- pipes to a process which only deals with outputting to stdout and a process which translates all received input.
-- The output process will display each character as it is typed, whereas the translate process will only
-- display (by piping its buffer to the output process) once 'E' has been entered by the user.
--
--    'E' will act as the return key
--    'a' will be replaced with 'z'
--    'X' will act as backspace
--    'K' will kill all characters proceeding it's call
--    'T' will kill the program
--    'ctrl+k' will force kill the program without restoring sanity
--
-- Note that the application terminates on either a 'T' or 'ctrl+k' (ASCII 11), however 'ctr; + k' does not
-- return sanity to the program (return normal Linux terminal processing).
----------------------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>

#define MSGSIZE 255

// Function prototypes
void inputProcess();
void outputProcess();
void translateProcess();

int pipeInOut[2], pipeInTranslate[2], pipeTranslateOut[2];
pid_t childpid = 0; 


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: main
--
-- DATE: March 16, 2008
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Justin Cervantes
--
-- PROGRAMMER: Justin Cervantes
--
-- INTERFACE: int main (int argc, char *argv[]) 
-- int argc: unused
-- char *argv[]: unused
--
-- RETURNS: Returns 0 if the program exits successfully, and a non-zero integer for irregular termination.
--
-- NOTES:
-- This function drives the program and sets up the 2 additional processes. The initial terminal processing
-- is also declared here.
----------------------------------------------------------------------------------------------------------------------*/
int main (int argc, char *argv[]) 
{

    // Onload usage instructions
    printf("Usage:\n");
    printf("Type any message and enter 'E' in order to submit your text.\n");
    printf("The translated message will be printed out in green.\n\n");
    printf("\t'E' will act as the return key\n");
    printf("\t'a' will be replaced with 'z'\n");
    printf("\t'X' will act as backspace\n");
    printf("\t'K' will kill all characters proceeding it's call\n");
    printf("\t'T' will kill the program\n");
    printf("\t'ctrl+k' will force kill the program without restoring sanity\n\n");

    // Create the pipes that will connect the processes
    if(pipe(pipeInOut) < 0) {perror("Input to Output pipe creation failed.");}
    if(pipe(pipeInTranslate) < 0) {perror("Input to Translate pipe creation failed.");}
    if(pipe(pipeTranslateOut) < 0) {perror("Translate to Output pipe creation failed.");}
    
    // Disable all Linux terminal processing
    system("stty raw igncr -echo");

    // Create the processes
    // When fork is called, the parent process will create a non-zero 
    // return, whereas the child will return 0
    // Non-zeroes in C are represented as true
    if((childpid = fork())) {
        inputProcess();
    } else if((childpid = fork())) {
        outputProcess();
    } else {
        translateProcess();
    }   
   	return 0; 
}


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: inputProcess
--
-- DATE: January 22, 2020
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Justin Cervantes
--
-- PROGRAMMER: Justin Cervantes
--
-- INTERFACE: void inputProcess()
-- no params
--
-- RETURNS: void
--
-- NOTES:
-- This function puts the original process into an infinite loop which will constantly wait for a keyboard input
-- and then pipe the received value into the translate and output processes via its input buffer.
----------------------------------------------------------------------------------------------------------------------*/
void inputProcess() {

    printf("\rInput Process - process ID:%ld  parent ID:%ld  child ID:%ld\n", (long)getpid(), (long)getppid(), (long)childpid);

    // Constantly read from the pipe
    while(1) {

        char inputChar = getchar();
        char outbuf[MSGSIZE] = {0};

        outbuf[0] = inputChar;
        write(pipeInOut[1], outbuf, MSGSIZE);
        write(pipeInTranslate[1], outbuf, MSGSIZE);
    
    }
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: translateProcess
--
-- DATE: January 22, 2020
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Justin Cervantes
--
-- PROGRAMMER: Justin Cervantes
--
-- INTERFACE: void translateProcess()
-- no params
--
-- RETURNS: void
--
-- NOTES:
-- This function receives the payload from the input process and adds it to its output buffer translatedMessage.
-- As input is received, the contents are analyzed and processed based to the codes outlined in the program notes at
-- the top of this code. Once 'E' is detected, the translatedMessage buffer is piped to the output process for
-- printing to stdout.
----------------------------------------------------------------------------------------------------------------------*/
void translateProcess() {
     fprintf(stderr, "\rTranslation Process - process ID:%ld  parent ID:%ld  child ID:%ld\n\r", (long)getpid(), (long)getppid(), (long)childpid);
    
    close(pipeInTranslate[1]);
    
    // translatedMessage is the output buffer
    char translatedMessage[MSGSIZE] = {0};
    int cursor = 0;
    
    while(1) {
    
        char inbuf[MSGSIZE] = {0};
        read(pipeInTranslate[0], inbuf, MSGSIZE);
        char curChar = inbuf[0];

    
        switch(curChar) {
            case 'a':
                translatedMessage[cursor] = 'z';
                cursor++;
                break;
            case 'X':
                translatedMessage[cursor] = 0;
                if(cursor > 0) cursor--;
                break;
            case 'K':
                cursor = 0;
                translatedMessage[cursor] = curChar;
                cursor++;
                for(int i = 1; i < MSGSIZE; i++) {
                    translatedMessage[i] = 0;
                }
                break;
            case 'E':
                translatedMessage[cursor] = curChar;
                write(pipeTranslateOut[1], translatedMessage, MSGSIZE);                
                for(int i = 0; i < MSGSIZE; i++) {
                    translatedMessage[i] = 0;
                }
                cursor = 0;
                break;
            case 'T':
                system("stty -raw -igncr echo");
                printf("\rDefault linux terminal processing has been restored... exiting program\n\r");
                kill(0,9);
                exit(0);
                break;
            case 11:
                printf("\rAbnormal interruption detected!");
                kill(0,9);
                exit(0);
            default:    
                translatedMessage[cursor] = curChar;
                cursor++;
                break;
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: outputProcess
--
-- DATE: January 22, 2020
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Justin Cervantes
--
-- PROGRAMMER: Justin Cervantes
--
-- INTERFACE: void outputProcess()
-- no params
--
-- RETURNS: void
--
-- NOTES:
-- This function receives the payload from the input process and prints it out - if it detects 'E', it knows
-- that there will be contents in the pipe from the translate process, so it checks it to print out the translated
-- payload.
----------------------------------------------------------------------------------------------------------------------*/
void outputProcess() {
    
    fprintf(stderr, "\rOutput process - ID:%ld  parent ID:%ld  child ID:%ld\n\r", (long)getpid(), (long)getppid(), (long)childpid);

    close(pipeInOut[1]);
    
    while(1) {
        char inbuf[MSGSIZE];
        char translatedbuf[MSGSIZE];
        read(pipeInOut[0], inbuf, MSGSIZE);
        printf("%s", inbuf);

        if(inbuf[0] == 'E') {
            read(pipeTranslateOut[0], translatedbuf, MSGSIZE);
            printf("\n\r\033[32;1mTranslated message: %s\033[0m\n\r", translatedbuf);
        }

        fflush(stdout);

    }    
}
