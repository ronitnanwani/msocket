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
#include <time.h>  

// Global error variable
int m_errno;

#define SOCK_MTP 12345
#define MAX_SOCKETS 25 // Maximum number of MTP sockets
#define MAX_SIZE 5
#define MAX_BUFFER_SIZE_SENDER 10
#define MAX_BUFFER_SIZE_RECEIVER 5
#define ENOTBOUND 1002
#define ENOSPACE 1003
#define T 10
#define p 0.05

typedef struct {
    int sequence_number;
    int ty;          //1 for message and 2 for ack
    time_t lastsenttime;
} message_header;

// Structure to represent a message in the buffer
typedef struct {
    int ismsg;      //0 if not msg 1 if msg
    message_header msg_header;
    char data[1024]; // Assuming message size is 1KB
} Message;

// Structure to represent a message in the buffer
typedef struct {
    int ismsg;       // 0 if not msg 1 if msg
    char data[1024]; // Assuming message size is 1KB
} MessageRecv;

// Structure to represent sender/receiver window
typedef struct {
    int size;
    int ptr1;
    int ptr2;
}Window;

// Structure to represent an MTP socket
typedef struct {
    int is_free;
    int process_id;
    int udp_socket_id;
    char ip_address[INET_ADDRSTRLEN]; // Assuming IPv4 address (e.g., "xxx.xxx.xxx.xxx\0")
    unsigned short port;
    // Other necessary fields
    int curr;       //next sequence number to assign to a message coming inside the send buffer;
    int wrs;        //current index to write to in the sender buffer;
    int str;         //which index to start reading from in the receiving buffer
    int wrr;        //current index to write to in the receiver buffer;
    Message send_buffer[MAX_BUFFER_SIZE_SENDER];
    MessageRecv receive_buffer[MAX_BUFFER_SIZE_RECEIVER];
    MessageRecv receive_temp_buffer[16];
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
ssize_t m_sendto(int , const void* , size_t , int , struct sockaddr* , socklen_t);
ssize_t m_recvfrom(int , void *, size_t ,int ,struct sockaddr* ,socklen_t);
int m_close(int);