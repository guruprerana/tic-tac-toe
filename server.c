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

#define MAX_LINE_LEN 1000
#define MAX_THREADS 2

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t state_change = PTHREAD_COND_INITIALIZER;

int active_requests;

int sockfd;

struct process_request_args {
  char *recv_buf;
  struct sockaddr *recv_addr; 
  unsigned int recv_addr_len; 
  int buf_len;
};

void *process_request(void *args_void);

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

  if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in)) == -1) {
    printf("Could not bind to specified port\n");
    close(sockfd);
    return 1;
  }

  active_requests = 0;

  while (1) {
    while (active_requests >= MAX_THREADS) {
      // while we are at max capacity, we need to wait
      pthread_cond_wait(&state_change, &mutex);
    }

    char *recv_buf = malloc(MAX_LINE_LEN * sizeof(char));
    struct sockaddr *recv_addr = malloc(sizeof(struct sockaddr));

    unsigned int recv_addr_len = sizeof(struct sockaddr);
    int buf_len;
    buf_len = recvfrom(sockfd, recv_buf, MAX_LINE_LEN, 0, recv_addr, &recv_addr_len);

    if (buf_len == -1) {
      printf("Error occured while receiving data.\n");
      continue;
    }

    // add end of string character at the end of buffer
    recv_buf[buf_len] = '\0';

    struct process_request_args *args = malloc(sizeof(struct process_request_args));
    args->recv_buf = recv_buf;
    args->recv_addr = recv_addr;
    args->recv_addr_len = recv_addr_len;
    args->buf_len = buf_len;

    pthread_t thread;
    if (pthread_create(&thread, NULL, process_request, (void *) args)) {
      fprintf(stderr, "Error creating thread\n");
      continue;
    }
    pthread_mutex_lock(&mutex);
    active_requests++;
    pthread_mutex_unlock(&mutex);
  }

  close(sockfd);
  return 0;
}

void *process_request(void *args_void) {
  struct process_request_args *args = (struct process_request_args *) args_void;
  printf("%s", args->recv_buf);
  sleep(5);
  int sent_len = sendto(sockfd, args->recv_buf, args->buf_len, 0, args->recv_addr, args->recv_addr_len);
  if (sent_len != args->buf_len) {
    printf("Could not send data back\n");
  }

  free(args->recv_addr);
  free(args->recv_buf);
  free(args);

  pthread_mutex_lock(&mutex);
  active_requests--;
  pthread_cond_signal(&state_change);
  pthread_mutex_unlock(&mutex);

  return NULL;
}
