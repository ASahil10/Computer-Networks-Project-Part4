/* file_server_udp.c */

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

int main(int argc, char *argv[])
{
    struct sockaddr_in sin, fsin;
    socklen_t alen;
    struct pdu rpdu, spdu;
    int s;
    int port = 3000;
    int fd, n;
    struct stat st;
    int remaining;

    switch (argc) {
        case 1:
            break;
        case 2:
            port = atoi(argv[1]);
            break;
        default:
            fprintf(stderr, "Usage: %s [port]\n", argv[0]);
            exit(1);
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("socket");
        exit(1);
    }

    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("bind");
        close(s);
        exit(1);
    }

    printf("UDP file server running on port %d\n", port);

    while (1) {
        alen = sizeof(fsin);

        n = recvfrom(s, &rpdu, sizeof(rpdu), 0, (struct sockaddr *)&fsin, &alen);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        if (rpdu.type != 'C') {
            continue;
        }

        printf("Client requested file: %s\n", rpdu.data);

        fd = open(rpdu.data, O_RDONLY);
        if (fd < 0) {
            spdu.type = 'E';
            strcpy(spdu.data, "File not found or cannot be opened");
            sendto(s, &spdu, strlen(spdu.data) + 2, 0,
                   (struct sockaddr *)&fsin, alen);
            continue;
        }

        if (stat(rpdu.data, &st) < 0) {
            spdu.type = 'E';
            strcpy(spdu.data, "Cannot determine file size");
            sendto(s, &spdu, strlen(spdu.data) + 2, 0,
                   (struct sockaddr *)&fsin, alen);
            close(fd);
            continue;
        }

        /* optional safety check based on assignment wording */
        if (st.st_size <= 100) {
            spdu.type = 'E';
            strcpy(spdu.data, "File size must be greater than 100 bytes");
            sendto(s, &spdu, strlen(spdu.data) + 2, 0,
                   (struct sockaddr *)&fsin, alen);
            close(fd);
            continue;
        }

        remaining = st.st_size;

        while ((n = read(fd, spdu.data, 100)) > 0) {
            remaining -= n;

            if (remaining == 0)
                spdu.type = 'F';
            else
                spdu.type = 'D';

            if (sendto(s, &spdu, n + 1, 0, (struct sockaddr *)&fsin, alen) < 0) {
                perror("sendto");
                break;
            }
        }

        close(fd);
        printf("Finished sending file: %s\n", rpdu.data);
    }

    close(s);
    return 0;
}