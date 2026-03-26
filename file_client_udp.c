/* file_client_udp.c */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

struct pdu {
    char type;
    char data[100];
};

int main(int argc, char **argv)
{
    char *host = "localhost";
    int port = 3000;
    struct hostent *phe;
    struct sockaddr_in sin;
    int s;
    struct pdu spdu, rpdu;
    int n, fd;
    char choice[10];
    char filename[100];

    switch (argc) {
        case 1:
            break;
        case 2:
            host = argv[1];
            break;
        case 3:
            host = argv[1];
            port = atoi(argv[2]);
            break;
        default:
            fprintf(stderr, "usage: %s [host [port]]\n", argv[0]);
            exit(1);
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    if ((phe = gethostbyname(host)) != NULL) {
        memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
    } else if ((sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE) {
        fprintf(stderr, "Can't get host entry\n");
        exit(1);
    }

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("socket");
        exit(1);
    }

    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("connect");
        close(s);
        exit(1);
    }

    while (1) {
        printf("\n1. Download file\n2. Quit\nEnter choice: ");
        scanf("%9s", choice);

        if (strcmp(choice, "2") == 0) {
            break;
        }

        if (strcmp(choice, "1") != 0) {
            printf("Invalid choice\n");
            continue;
        }

        printf("Enter filename: ");
        scanf("%99s", filename);

        memset(&spdu, 0, sizeof(spdu));
        spdu.type = 'C';
        strcpy(spdu.data, filename);

        if (write(s, &spdu, strlen(spdu.data) + 2) < 0) {
            perror("write");
            continue;
        }

        n = read(s, &rpdu, sizeof(rpdu));
        if (n < 0) {
            perror("read");
            continue;
        }

        if (rpdu.type == 'E') {
            printf("Server error: %s\n", rpdu.data);
            continue;
        }

        fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            perror("open local file");
            continue;
        }

        while (1) {
            if (rpdu.type == 'D' || rpdu.type == 'F') {
                if (write(fd, rpdu.data, n - 1) < 0) {
                    perror("write file");
                    break;
                }
            }

            if (rpdu.type == 'F') {
                printf("Download complete: %s\n", filename);
                break;
            }

            n = read(s, &rpdu, sizeof(rpdu));
            if (n < 0) {
                perror("read");
                break;
            }
        }

        close(fd);
    }

    close(s);
    return 0;
}