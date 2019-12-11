/*************

   To compile: gcc -pthread mtFileStats.c -o mtFileStats.o
   To run: ./mtFilestats <any number of file names seperated with spaces>

*************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXCHARS 80

pthread_mutex_t mutex1;

struct FileInfo {
    char *name;   /* name of file */
    int numLines; /* number of lines in file */
    int numWords; /* number of words in file */
    int numChars; /* number of characters in file */
};

void *runThread(void *);

int countWords(char *);

char **globalargv;
int globalargc;

struct FileInfo *info; /* array of counts for each file */

int main(int argc, char **argv) {
    int numThread;
    pthread_t *thread;
    int iret, i;

    globalargv = argv;
    globalargc = argc;

    numThread = argc - 1;
    thread = (pthread_t *) malloc(numThread * sizeof(pthread_t));

    for (i = 0; i < numThread; i++) {
        if ((iret = pthread_create(&thread[i], NULL, runThread, (void *) i))) {
            printf("Error creating thread %d\n", i);
            exit(0);
        }
    }      

    for (i = 0; i < numThread; i++) {
        if (((iret = pthread_join(thread[i], NULL)))) {
            printf("Error terminating thread %d\n", i);
            exit(0);
        }
    }

    int numLines = 0, numWords = 0, numChars = 0; /* total counts */
    for (int i = 0; i < argc - 1; i++) {
        numLines += info[i].numLines;
        numWords += info[i].numWords;
        numChars += info[i].numChars;
    }

    printf("Total: %d lines, %d words, %d characters\n", numLines, numWords,
           numChars);
}

void *runThread(void *num) {
  
    char inString[MAXCHARS];
    char *rs;
    long n = (long) (num);
    FILE *fp;
    int argc = globalargc;


    info = (struct FileInfo *) malloc(argc * sizeof(struct FileInfo));
    pthread_mutex_lock(&mutex1);
    printf("Thread %u %d\n", (int) pthread_self(), num);

    /* open an input file, initialize struct of counts */
    fp = fopen(globalargv[n + 1], "r");
    if (fp == NULL) {
        printf("Error: cannot open file\n");
        return 0;
    }

    info[n].name = (char *) malloc(MAXCHARS * sizeof(char));
    strncpy(info[n].name, globalargv[n + 1], MAXCHARS);
    info[n].numLines = 0;
    info[n].numWords = 0;
    info[n].numChars = 0;

    /* read each line, update counts */
    rs = fgets(inString, MAXCHARS, fp);


    while (rs != NULL) {
        info[n].numLines++;
        info[n].numChars += strlen(inString);
        info[n].numWords += countWords(inString);
        rs = fgets(inString, MAXCHARS, fp);
    }
    
    printf("TID: %u %d Process: %d - %s: %d lines, %d words, %d characters\n", pthread_self(), num, getpid(),
           info[n].name, info[n].numLines, info[n].numWords,
           info[n].numChars);
    pthread_mutex_unlock(&mutex1);
    pthread_exit(0);
}

int countWords(char *inS) {
    int numTokens = 0;
    int i = 0;

    for (i = 1; i < strlen(inS); i++) {
        if ((isalnum(inS[i - 1]) || ispunct(inS[i - 1])) && (inS[i] == ' ')) {
            numTokens++;
        }
    }

    if (isalnum(inS[strlen(inS) - 2]) || ispunct(inS[strlen(inS) - 2])) {
        numTokens++;
    }
    return numTokens;
}