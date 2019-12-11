/*

    File: client.c
    Compile: gcc client.c -o client
    Run: ./client [hostname] [filename]

    Description:

    adapted from Brian "Beej Jorgensen" Hall
    https://beej.us/guide/bgnet/html/#a-simple-stream-client
    as well as Bill Hsu
    http://unixlab.sfsu.edu/~whsu/csc415/Sockets/client.c

    [hostname] is the name of the server host
    [filename] is the name of the text file to have
    its lines, words and characters counted

    IPv4 only!
    Client sends the text contained within a text file to a server
    Client receives the total word count, line count, and character count
    from the server

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "5001" // the port client will be connecting to
#define MAXDATASIZE 5000 // max number of bytes we can get at once

int main(int argc, char *argv[]) {
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    FILE *fp;
    char s[INET_ADDRSTRLEN];

    if (argc != 3) {
        fprintf(stderr, "usage: client hostname message\n");
        exit(1);
    }

    /* get info for server */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        /* try to create socket */
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        /* connect socket to server */
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(sockfd);
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    struct sockaddr_in *sa = (struct sockaddr_in *) p->ai_addr;
    inet_ntop(AF_INET, &(sa->sin_addr), s, sizeof(s));

    freeaddrinfo(servinfo); // all done with this structure

    //get info from the file
    char mess[5000];
    fp = fopen(argv[2], "r");
    if (fp == NULL) {
        printf("Error: cannot open file\n");
        return 0;
    }

    int i = 0;
    while (1) {
        char c = fgetc(fp);
        if (feof(fp))
            break;
        mess[i] = c;
        i++;
    }

    /* send message to server */
    numbytes = write(sockfd, mess, strlen(mess) + 1);
    if (numbytes != strlen(mess) + 1) {
        perror("write");
        close(sockfd);
        exit(0);
    }

    /* receive message from server */
    numbytes = read(sockfd, buf, MAXDATASIZE - 1);

    buf[numbytes] = '\0';
    printf("%s: %s\n", argv[2], buf);

    close(sockfd);
    return 0;
}
