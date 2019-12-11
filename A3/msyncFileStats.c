/***********************

    To compile: gcc -pthread mtFileStats.c -o mtFileStats.o
    To run: ./mtFilestats <any number of file names seperated with spaces>

***********************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>

#define MAXCHARS 80

void *childFunc(void *ptr);

struct FileInfo {
    char *name;    /* name of file */
    int numLines;  /* number of lines in file */
    int numWords;  /* number of words in file */
    int numChars;  /* number of characters in file */
};

struct FileInfo *info;

int countWords(char *);

sem_t s;

int main(int argc, char **argv) {

    int numLines = 0, numWords = 0, numChars = 0; /* total counts */
    pthread_t *thread;
    int *threadid;
    sem_init(&s, 0, 0);

    info = (struct FileInfo *) malloc((argc - 1) * sizeof(struct FileInfo));

    for (int i = 0; i < argc - 1; i++) {
        info[i].name = (char *) malloc(20 * sizeof(char));
        strncpy(info[i].name, argv[i + 1], 20);
    }

    thread = (pthread_t *) malloc((argc - 1) * sizeof(pthread_t));

    threadid = (int *) malloc((argc - 1) * sizeof(int));
    for (int i = 0; i < argc - 1; i++) {
        threadid[i] = i;
    }

    for (int i = 0; i < argc - 1; i++) {
        /* spawn a thread */

        if (pthread_create(&thread[i], NULL, childFunc, &threadid[i])) {
            printf("Error creating thread\n");
            exit(0);
        }
        sem_wait(&s);
    }

    for (int i = 0; i < argc - 1; i++) {
        numLines += info[i].numLines;
        numWords += info[i].numWords;
        numChars += info[i].numChars;
    }

    printf("Total: %d lines, %d words, %d characters\n",
           numLines, numWords, numChars);
    free(info);
    free(threadid);
    free(thread);
}

/***********************

  int countWords(char *inS)

  inS: input null-terminated string

  returns number of words in string, delimited by spaces

***********************/

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

void *childFunc(void *ptr) {
    FILE *fp;
    char *rs;
    char inString[MAXCHARS];
    int myIndex = *((int *) ptr);
    /* open an input file, initialize struct of counts */
    fp = fopen(info[myIndex].name, "r");
    if (fp == NULL) {
        printf("Error: cannot open file\n");
        return 0;
    }

    info[myIndex].numLines = 0;
    info[myIndex].numWords = 0;
    info[myIndex].numChars = 0;

    /* read each line, update counts */
    rs = fgets(inString, MAXCHARS, fp);

    while (rs != NULL) {
        info[myIndex].numLines++;
        info[myIndex].numChars += strlen(inString);
        info[myIndex].numWords += countWords(inString);
        rs = fgets(inString, MAXCHARS, fp);
    }
    printf("Thread %ld %s: %d lines, %d words, %d characters\n",
           (long) pthread_self(), info[myIndex].name,
           info[myIndex].numLines, info[myIndex].numWords, info[myIndex].numChars);
    sem_post(&s);
    fclose(fp);
}
