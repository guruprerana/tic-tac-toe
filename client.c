/**
 *
 * Author: Marie Huynh, Guruprerana Shabadi
 * Email : marie.huynh@polytechnique.edu, 
 *         guruprerana.shabadi@polytechnique.edu
 *
 * Date  : 28.05.2021
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

#include "client.h"

#define FYI 0x01
#define MYM 0x02
#define END 0x03
#define TXT 0x04
#define MOV 0x05
#define LFT 0x06

#define MAX_LINE_LEN 1000

int sockfd;
struct sockaddr_in addr;
size_t addr_size = sizeof(struct sockaddr_in);

int handle_MYM(unsigned char* recv_buf, int recv_len);
int handle_END(unsigned char* recv_buf, int recv_len);
int handle_FYI(unsigned char* recv_buf, int recv_len);
int char_to_int(unsigned char c);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("IP_ADDRESS and PORT arguments are required\n");
        return 1;
    }
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) != 1) {
        printf("Could not parse IP Address\n");
    }

    char hello_message[7];
    hello_message[0] = TXT;
    strncpy(hello_message+1, "Hello", 6);
    int n_sent = sendto(sockfd, hello_message, 7, 0, (struct sockaddr *) &addr, addr_size);
    if (n_sent < 7) {
        printf("Could not connect to server\n");
        return -1;
    }

    unsigned char recv_buf[MAX_LINE_LEN];
    struct sockaddr recv_addr;
    unsigned int recv_addr_len = sizeof(struct sockaddr);
    int n_recv;

    while ((n_recv = recvfrom(sockfd, recv_buf, MAX_LINE_LEN, 0, &recv_addr, &recv_addr_len)) > 0) {
        switch (recv_buf[0]) {
            case TXT:
                printf("%s\n", recv_buf+1);
                break;
            case MYM:
                handle_MYM(recv_buf, n_recv);
                break;
            case END:
                handle_END(recv_buf, n_recv);
                return 0;
            case FYI:
                handle_FYI(recv_buf, n_recv);
                break;
            default:
                printf("Invalid message from the server\n");
        }
    }

    close(sockfd);
    return 0;
}

int handle_END(unsigned char* recv_buf, int recv_len) {
    switch (recv_buf[1]) {
        case 0:
            printf("The game resulted in a draw.\n");
            break;
        case 1:
            printf("Player 1 wins!\n");
            break;
        case 2:
            printf("Player 2 wins!\n");
            break;
        case 255:
            printf("There is no room for new participants to join the game.\n");
            break;
        default:
            printf("Invalid END message received\n");
            return -1;
    }
    return 0;
}

int handle_MYM(unsigned char* recv_buf, int recv_len) {
    char col = '3', row = '3';

    printf("It is now time to make your move!\n");

    printf("Enter the column number of your move (0, 1, or 2): ");
    while (col != '0' && col != '1' && col != '2') {
        col = getchar();
    }

    printf("Enter the row number of your move (0, 1, or 2): ");
    while (row != '0' && row != '1' && row != '2') {
        row = getchar();
    }

    char move[3];
    move[0] = MOV;
    move[1] = char_to_int(col);
    move[2] = char_to_int(row);

    int n_sent = sendto(sockfd, move, 3, 0, (struct sockaddr *) &addr, addr_size);
    if (n_sent < 3) {
        printf("Could not connect to server\n");
        return -1;
    }

    return 0;
}

int handle_FYI(unsigned char* recv_buf, int recv_len) {
    char positions[3][3] = {
        {' ', ' ', ' '}, 
        {' ', ' ', ' '}, 
        {' ', ' ', ' '}
    };

    unsigned int n = recv_buf[1];
    printf("%d filled positions.\n", n);

    char grid[] = " | | \n-+-+-\n | | \n-+-+-\n | | \n";

    int i = 2;
    for (unsigned int j = 0; j < n; j++) {
        int row_int = recv_buf[i+2], col_int = recv_buf[i+1];

        if (recv_buf[i] == 1) {
            positions[row_int][col_int] = 'X';
        }
        else {
            positions[row_int][col_int] = 'O';
        }

        i = i + 3;
    }

    grid[0] = positions[0][0];
    grid[2] = positions[0][1];
    grid[4] = positions[0][2];

    grid[12] = positions[1][0];
    grid[14] = positions[1][1];
    grid[16] = positions[1][2];

    grid[24] = positions[2][0];
    grid[26] = positions[2][1];
    grid[28] = positions[2][2];

    printf("%s\n", grid);
    return 0;
}

int char_to_int(unsigned char c) {
    switch (c) {
        case '0':
            return 0;
        case '1':
            return 1;
        case '2':
            return 2;
        default:
            return -1;
    }
}
