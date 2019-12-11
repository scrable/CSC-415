/*

    File: server.c
    Compile: gcc server.c -o server
    Run: ./server

    Description:

    adapted from Brian "Beej Jorgensen" Hall
    https://beej.us/guide/bgnet/html/#a-simple-stream-client
    as well as Bill Hsu
    http://unixlab.sfsu.edu/~whsu/csc415/Sockets/client.c

    IPv4 only!
    Server receives a string from the client
    Server counts the words, lines, and characters and returns
    these values back to the client

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT "5001"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

struct FileInfo {
    int numLines;  /* number of lines in file */
    int numWords;  /* number of words in file */
    int numChars;  /* number of characters in file */
};

int main(void) {
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    int yes = 1;
    char s[INET_ADDRSTRLEN];
    int rv;
    struct FileInfo *info; /* array of counts for each file */
    info = (struct FileInfo *) malloc(sizeof(struct FileInfo));

    /* get info for self, to set up socket */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        /* try to create socket */
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        /* set socket options so socket can be reused */
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        /* bind socket to port number */
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while (1) {  // main accept() loop
        sin_size = sizeof their_addr;
        /* block until request arrives */
        new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        /* get information about connecting client */
        struct sockaddr_in *sa = (struct sockaddr_in *) &their_addr;
        inet_ntop(AF_INET, &(sa->sin_addr), s, sizeof(s));
        printf("server: got connection from %s\n", s);

        if (!fork()) { /* create child to handle new client */
            close(sockfd); /* child doesn't need the listener */

            /* read string from client */
            int numBytes = read(new_fd, s, 5000);
            if (numBytes > 5000) {
                perror("read");
                close(new_fd);
                exit(0);
            }

            /* calculate the totals */
            int i = 0;
            while ((void *) s[i] != NULL) {

                info->numChars++;
                if (s[i] == ' ') {
                    info->numWords++;
                }
                if (s[i] == '\n') {
                    info->numLines++;
                }
                i++;
            }

            //account for last word
            info->numWords++;
            printf("Total: %d lines, %d words, %d characters\n",
                   info->numLines, info->numWords, info->numChars);

            /* send reversed string back to client */

            char buffers[5000];
            sprintf(buffers, "%d lines, %d words, %d characters", info->numLines, info->numWords, info->numChars);

            send(new_fd, buffers, strlen(buffers), 0);
            numBytes = write(new_fd, info, strlen(s) + 1);
            if (numBytes != strlen(s) + 1) {
                perror("write");
                close(new_fd);
                exit(0);
            }
            close(new_fd);
            exit(0);
        }
        close(new_fd);  /* parent doesn't need this */
    }
}

