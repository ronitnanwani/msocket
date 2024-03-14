#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>      
#include <sys/socket.h>     
#include <netinet/in.h>     
#include <arpa/inet.h>      
#include <string.h>   
#include <sys/sem.h> 
#include <signal.h>   
#include <sys/select.h>
  

// Global error variable
int m_errno;

#define SOCK_MTP 12345
#define MAX_SOCKETS 25 // Maximum number of MTP sockets
#define MAX_SIZE 5
#define MAX_BUFFER_SIZE_SENDER 10
#define MAX_BUFFER_SIZE_RECEIVER 5
#define ENOTBOUND 1002
#define T 5
#define p 0.05

typedef struct {
    int sequence_number;
    int ty;          //0 for message and 1 for ack
} message_header;
// Structure to represent a message in the buffer
typedef struct {
    message_header msg_header;
    char data[1024]; // Assuming message size is 1KB
} Message;

// Structure to represent sender/receiver window
typedef struct {
    // Define sender/receiver window structure here
    // For example:
    int size;
    int sequence_numbers[MAX_SIZE]; // Array of sequence numbers
}Window;

// Structure to represent an MTP socket
typedef struct {
    int is_free;
    int process_id;
    int udp_socket_id;
    char ip_address[INET_ADDRSTRLEN]; // Assuming IPv4 address (e.g., "xxx.xxx.xxx.xxx\0")
    unsigned short port;
    // Other necessary fields
    Message send_buffer[MAX_BUFFER_SIZE_SENDER];
    Message receive_buffer[MAX_BUFFER_SIZE_RECEIVER];
    Window swnd;
    Window rwnd;
} MTPSocket;

// Structure to represent shared memory containing information about N MTP sockets
typedef struct {
    MTPSocket sockets[MAX_SOCKETS];
} SharedMemory;

typedef struct{
    int sockid;
    char ip[INET_ADDRSTRLEN];
    short port;
    int err_no;
} SockInfo;

int m_socket(int, int, int);
int m_bind(int, char*,short,char*,short);
int m_sendto(int, const void *, size_t, int, const struct sockaddr_in *, socklen_t);
int m_recvfrom(int, void *, size_t, int, struct sockaddr_in *, socklen_t*);
int m_close(int);