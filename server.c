/**
 *
 * Author: Marie Huynh, Guruprerana Shabadi
 * Email : marie.huynh@polytechnique.edu, 
 *         guruprerana.shabadi@polytechnique.edu
 *
 * Date  : 17.05.2021
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

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int sockfd;

#define MAX_LINE_LEN 1000

void *receive_thread(void *args);

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("IP_ADDRESS and PORT arguments are required\n");
    return 1;
  }
  
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  pthread_t thread;
  if (pthread_create(&thread, NULL, receive_thread, NULL)) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(argv[2]));
  if (inet_pton(AF_INET, argv[1], &addr.sin_addr) != 1) {
    printf("Could not parse IP Address\n");
  }

  char in_buf[MAX_LINE_LEN];

  while (fgets(in_buf, MAX_LINE_LEN, stdin) != NULL) {
    int n_sent = sendto(sockfd, in_buf, strlen(in_buf), 0,
	   (struct sockaddr *) &addr, sizeof(struct sockaddr_in));

    if (n_sent < strlen(in_buf)) {
      printf("Could not send line\n");
      continue;
    }
  }

  close(sockfd);
  return 0;
}

void *receive_thread(void *args) {
  char recv_buf[MAX_LINE_LEN];

  while (1) {
    struct sockaddr recv_addr;
    unsigned int recv_addr_len = sizeof(struct sockaddr);

    int n_recv = recvfrom(sockfd, recv_buf, MAX_LINE_LEN, 0, 
      &recv_addr, &recv_addr_len);

    if (n_recv == -1) {
      printf("Error receiving from server\n");
    }

    recv_buf[n_recv] = '\0';
    printf("%s", recv_buf);
    memset(recv_buf, 0, MAX_LINE_LEN);
  }
}