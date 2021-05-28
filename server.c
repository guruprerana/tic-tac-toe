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
#include <pthread.h>
#include <stdbool.h>

#define FYI 0x01
#define MYM 0x02
#define END 0x03
#define TXT 0x04
#define MOV 0x05
#define LFT 0x06

#define MAX_LINE_LEN 1000
#define MAX_THREADS 2

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t state_change = PTHREAD_COND_INITIALIZER;

struct sockaddr *ex_addr = NULL, *oh_addr = NULL;
unsigned int ex_addr_len = sizeof(struct sockaddr), oh_addr_len = sizeof(struct sockaddr);
pthread_mutex_t addr_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t player_change = PTHREAD_COND_INITIALIZER;

int active_requests = 0;
pthread_mutex_t active_requests_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t active_requests_change = PTHREAD_COND_INITIALIZER;

pthread_t previous_game_thread = 0;

char positions[3][3] = {
    {' ', ' ', ' '},
    {' ', ' ', ' '},
    {' ', ' ', ' '}
};
int next_player = 0;

int sockfd;

struct process_request_args {
    char *recv_buf;
    struct sockaddr *recv_addr; 
    unsigned int recv_addr_len; 
    int buf_len;
    pthread_t previous_game_thread;
};

void *process_player_request(void *args_void);
void *process_game_request(void *args_void);
int check_winner();
int is_valid(int col, int row);
int update_move(int player, int row, int col);
void send_welcome(int current_player);
void send_mym();
void send_end();
void send_fyi();

int main(int argc, char *argv[]) {
    
    if (argc < 2) {
        printf("PORT argument is required\n");
    }
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) != 1) {
        printf("Could not parse IP Address\n");
    }
    printf("Socket created.\n");

    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in)) == -1) {
        printf("Could not bind to specified port\n");
        close(sockfd);
        return 1;
    }
    printf("Bind to port %s.\n", argv[1]);

    while (1) {
        while (active_requests >= MAX_THREADS) {
        // while we are at max capacity, we need to wait
            pthread_cond_wait(&active_requests_change, &active_requests_mutex);
        }

        struct process_request_args *args = malloc(sizeof(struct process_request_args));

        args->recv_buf = malloc(MAX_LINE_LEN * sizeof(char));
        args->recv_addr = malloc(sizeof(struct sockaddr));
        args->recv_addr_len = sizeof(struct sockaddr);
        args->buf_len = recvfrom(sockfd, args->recv_buf, MAX_LINE_LEN, 0, args->recv_addr, &args->recv_addr_len);

        if (args->buf_len <= 0) {
            printf("Error occured while receiving data.\n");
            continue;
        }

        pthread_t thread;

        switch (args->recv_buf[0]) {
            case TXT:
                if (pthread_create(&thread, NULL, process_player_request, (void *) args)) {
                    printf("Error creating thread\n");
                    continue;
                }
                break;
            case MOV:
                args->previous_game_thread = previous_game_thread;
                if (pthread_create(&thread, NULL, process_game_request, (void *) args)) {
                    printf("Error creating thread\n");
                    continue;
                }
                previous_game_thread = thread;
                break;
            default:
                printf("Invalid request received\n");
                continue;
        }

        pthread_mutex_lock(&active_requests_mutex);
        active_requests++;
        pthread_mutex_unlock(&active_requests_mutex);
    }

    close(sockfd);
    return 0;
}

void *process_player_request(void *args_void) {
    struct process_request_args *args = (struct process_request_args *) args_void;
    if (strncmp(args->recv_buf+1, "Hello", 5)) {
        printf("Invalid client request received\n");
    } else {
        if (ex_addr == NULL) {
            ex_addr = malloc(sizeof(struct sockaddr));
            memcpy(ex_addr, args->recv_addr, args->recv_addr_len);
            ex_addr_len = args->recv_addr_len;

            // send welcome message you are player 1
            send_welcome(1);
        } else if (oh_addr == NULL) {
            oh_addr = malloc(sizeof(struct sockaddr));
            memcpy(oh_addr, args->recv_addr, args->recv_addr_len);
            oh_addr_len = args->recv_addr_len;

            // send welcome message you are player 2
            send_welcome(2);

            next_player = 1;

            send_fyi();
            send_mym();
            // start the game 
        } else {
            // reject a player

            unsigned char message[2];
            message[0] = END;
            message[1] = 0xFF;

            sendto(sockfd, message, 2, 0, args->recv_addr, args->recv_addr_len);
        }
    }

    free(args->recv_addr);
    free(args->recv_buf);
    free(args);

    pthread_mutex_lock(&active_requests_mutex);
    active_requests--;
    pthread_cond_signal(&active_requests_change);
    pthread_mutex_unlock(&active_requests_mutex);

    return NULL;
}

void *process_game_request(void *args_void) {
    struct process_request_args *args = (struct process_request_args *) args_void;

    if (args->previous_game_thread != 0) {
        pthread_join(args->previous_game_thread, NULL);
    }

    if (args->buf_len < 3) {
        printf("Invalid MOV request\n");
    } else if (next_player == 1) {
        if (memcmp(ex_addr, args->recv_addr, ex_addr_len)) {
            printf("Wrong player or invalid client message received\n");
        } else {
            if (is_valid(args->recv_buf[1], args->recv_buf[2]) == 1) {
                update_move(next_player, args->recv_buf[1], args->recv_buf[2]);
                next_player = 2;
            }
            if (check_winner() == -1) {
                send_fyi();
                send_mym();
            } else {
                send_end();
            }
        }
    } else if (next_player == 2) {
        if (memcmp(oh_addr, args->recv_addr, oh_addr_len)) {
            printf("Wrong player or invalid client message received\n");
        } else {
            if (is_valid(args->recv_buf[1], args->recv_buf[2]) == 1) {
                update_move(next_player, args->recv_buf[1], args->recv_buf[2]);
                next_player = 1;
            }
            if (check_winner() == -1) {
                send_fyi();
                send_mym();
            } else {
                send_end();
            }
        }
    }

    free(args->recv_addr);
    free(args->recv_buf);
    free(args);

    pthread_mutex_lock(&active_requests_mutex);
    active_requests--;
    pthread_cond_signal(&active_requests_change);
    pthread_mutex_unlock(&active_requests_mutex);

    return NULL;
}

int check_winner(){
    //rows and columns
    for (int k = 0; k < 3; k++){
        char current = positions[k][0];
        if ((current == positions[k][1]) && (current == positions[k][2])){
            if (current == 'X'){
                return 1;
            }
            else if (current == 'O'){
                return 2;
            }
        }
        current = positions[0][k];
            if ((current == positions[1][k]) && (current == positions[2][k])){
                if (current == 'X'){
                    printf("The game is finished : player 1 won.\n");
                    return 1;
                }
                else if (current == 'O'){
                    printf("The game is finished : player 2 won.\n");
                    return 2;
                }
            }
        }
    
    int current = positions[1][1];
    if ((current == positions[0][0]) && (current == positions[2][2])){
        if (current == 'X'){
            printf("The game is finished : player 1 won.\n");
            return 1;
        }
        else {
            printf("The game is finished : player 2 won.\n");
            return 2;
        }
    }
    if ((current == positions[2][0]) && (current == positions[0][2])){
        if (current == 'X'){
            printf("The game is finished : player 1 won.");
            return 1;
        }
        else if (current == 'O'){
            printf("The game is finished : player 2 won.\n");
            return 2;
        }
    }
    
    
    for (int i=0; i < 3; i++){
        for (int j=0;j < 3; j++){
            if (positions[i][j] == ' '){\
                return -1;
            }
        }
    }
    
    printf("The game is finished : draw.\n");
    return 0;
    
}

int is_valid(int col, int row) {
    if ((col < 0)||(col > 2)||(row < 0)|| (row >2)){
        printf("Position not in grid. Please retry");
        return 0;
    }
    if (positions[row][col] != ' '){
        printf("Position taken. Please retry");
        return 0;
    }
    return 1;
}

int update_move(int player, int col, int row) {
    if (player == 1){
            positions[row][col] = 'X';
        }
    else{
            positions[row][col] = 'O';
        }
    return 1;
}

void send_welcome(int current_player) {
    char message[45];
    message[0] = TXT;
    int sent_len;
    if (current_player == 1){
        strncpy(message+1, "Welcome! You are player 1. You play with X.", 44);
        sent_len = sendto(sockfd, message, 45, 0, ex_addr, ex_addr_len);
    }
    else{
        strncpy(message+1, "Welcome! You are player 2. You play with O.", 44);
        sent_len = sendto(sockfd, message, 45, 0, oh_addr, oh_addr_len);
    }
    if (sent_len != 45) {
      printf("Could not send welcome.\n");
    }
}

void send_mym() {
    char message[1];
    message[0] = MYM;
    int sent_len;
    if (next_player == 2){
        sent_len = sendto(sockfd, message, 1, 0, oh_addr, oh_addr_len);
    }
    else {
        sent_len = sendto(sockfd, message, 1, 0, ex_addr, ex_addr_len);
    }
    if (sent_len != 1) {
      printf("Could not send MYM.\n");
    }
}

void send_end() {
    char message[2];
    message[0] = END;
    message[1] = check_winner();
    int sent_len1;
    int sent_len2;
    sent_len1 = sendto(sockfd, message, 2, 0, oh_addr, oh_addr_len);
    sent_len2 = sendto(sockfd, message, 2, 0, ex_addr, ex_addr_len);
    if ((sent_len1 != 2)|| (sent_len2 != 2)) {
      printf("Could not send END.\n");
    }

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            positions[i][j] = ' ';
        }
    }

    previous_game_thread = 0;
    free(ex_addr);
    free(oh_addr);
    ex_addr = NULL;
    oh_addr = NULL;
}

void send_fyi(){
    char moves[30];
    moves[0] = FYI;
    int count = 0;
    for (int i = 0; i < 3; i++){
        for (int j = 0; j < 3; j++){
            if (positions[i][j] == 'X'){
                count++;
                moves[3*count-1] = 1;
                moves[3*count] = j;
                moves[3*count+1] = i;
            }
            else if (positions[i][j] == 'O'){
                count++;
                moves[3*count-1] = 2;
                moves[3*count] = j;
                moves[3*count+1] = i;
            }
        }
    }
    moves[1] = count;
    moves[3*count+2] = '\0';
    int message_len = 3*count+2;
    int sent_len;
    if (next_player == 2){
        sent_len = sendto(sockfd, moves, message_len, 0, oh_addr, oh_addr_len);
    }
    else {
        sent_len = sendto(sockfd, moves, message_len, 0, ex_addr, ex_addr_len);
    }
    if (sent_len != message_len) {
      printf("Could not send END.\n");
    }
}
