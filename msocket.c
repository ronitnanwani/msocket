#include "msocket.h"
#define SEMKEY1 6969
#define SEMKEY2 6970

struct sembuf wait_operation = {0,-1,0};
struct sembuf signal_operation = {0,1,0};

int printmsgheader(message_header m){
    printf("Message header\n");
    printf("Sequence number %d\n",m.sequence_number);
    printf("Message type %d\n",m.ty);
}

int printmsg(Message m){
    printf("Message\n");
    printmsgheader(m.msg_header);
    printf("%s\n",m.data);
}

int printWindow(Window w){
    printf("Window\n");
    printf("%d\n",w.size);
    printf("Sequence numbers\n");
    for(int i=0;i<MAX_SIZE;i++){
        printf("%d ",w.sequence_numbers[i]);
    }
    printf("\n");
}

int printmtpsocket(MTPSocket s){
    printf("isfree %d\n",s.is_free);
    printf("process_id %d\n",s.process_id);
    printf("udp_socket_id %d\n",s.udp_socket_id);
    printf("Dest ip %s\n",s.ip_address);
    printf("Dest port %hd\n",s.port);

    for(int i=0;i<MAX_BUFFER_SIZE_SENDER;i++){
        printmsg(s.send_buffer[i]);
    }
    for(int i=0;i<MAX_BUFFER_SIZE_RECEIVER;i++){
        printmsg(s.receive_buffer[i]);
    }
    printWindow(s.swnd);
    printWindow(s.rwnd);
}

int printSM(SharedMemory* sm){
    for(int i=0;i<MAX_SOCKETS;i++){
        printmtpsocket(sm->sockets[i]);
    }
}

int m_socket(int domain, int type, int protocol) {

    if (type != SOCK_MTP) {
        m_errno = EINVAL;  // Invalid socket type
        return -1;
    }

    // Attach to shared memory
    key_t shm_key = ftok("file1.txt", 65);
    int shm_id = shmget(shm_key, 0, 0666);
    if (shm_id == -1) {
        m_errno=errno;
        perror("shmget");
        return -1;
    }

    SharedMemory* shared_memory = (SharedMemory*)shmat(shm_id, NULL, 0);
    if (shared_memory == (void*)-1) {
        m_errno=errno;
        perror("shmat");
        return -1;
    }

    // Find a free entry in the shared memory
    int free_entry_index = -1;
    for (int i = 0; i < MAX_SOCKETS; ++i) {
        if (shared_memory->sockets[i].is_free) {
            free_entry_index = i;
            break;
        }
    }


    // If no free entry is available, return error
    if (free_entry_index == -1) {
        m_errno = ENOBUFS;
        return -1;
    }


    key_t shm_key2 = ftok("file2.txt",66);
    int shmid2 = shmget(shm_key2,0,0666);

    if(shmid2 == -1){
        m_errno=errno;
        perror("shmget");
        return -1;
    }

    SockInfo* sockinfo = (SockInfo*)shmat(shmid2,NULL,0);

    if(sockinfo == (void*)(-1)){
        m_errno=errno;
        perror("shmat");
        return -1;
    }


    sockinfo->sockid=0;
    sockinfo->port=0;
    sockinfo->err_no=0;
    strcpy(sockinfo->ip,"\0");


    int semid1 = semget(SEMKEY1,1,0);
    int semid2 = semget(SEMKEY2,1,0);

    semop(semid1,&signal_operation,1);
    semop(semid2,&wait_operation,1);


    if(sockinfo->sockid == -1){
        m_errno=errno;
        return -1;
    }

    
    shared_memory->sockets[free_entry_index].is_free = 0;
    shared_memory->sockets[free_entry_index].process_id = getpid();
    shared_memory->sockets[free_entry_index].udp_socket_id = sockinfo->sockid;
    memset(shared_memory->sockets[free_entry_index].send_buffer,0,sizeof(shared_memory->sockets[free_entry_index].send_buffer));
    memset(shared_memory->sockets[free_entry_index].receive_buffer,0,sizeof(shared_memory->sockets[free_entry_index].receive_buffer));
    memset(&shared_memory->sockets[free_entry_index].swnd,0,sizeof(shared_memory->sockets[free_entry_index].swnd));
    memset(&shared_memory->sockets[free_entry_index].rwnd,0,sizeof(shared_memory->sockets[free_entry_index].rwnd));

    int retval = shared_memory->sockets[free_entry_index].udp_socket_id;

    printf("After m_socket() call\n");
    printSM(shared_memory);
    shmdt(shared_memory);
    shmdt(sockinfo);


    return retval;
}


int m_bind(int sockfd, char* srcip,short srcport,char* destip,short destport){
    // Attach to shared memory
    key_t shm_key = ftok("file1.txt", 65);
    int shm_id = shmget(shm_key,0, 0666);
    if (shm_id == -1) {
        m_errno=errno;
        perror("shmget");
        return -1;
    }

    SharedMemory* shared_memory = (SharedMemory*)shmat(shm_id, NULL, 0);
    if (shared_memory == (void*)-1) {
        m_errno=errno;
        perror("shmat");
        return -1;
    }

    // Find the socket entry corresponding to sockfd
    int entry_index = -1;
    for (int i = 0; i < MAX_SOCKETS; ++i) {
        if (shared_memory->sockets[i].udp_socket_id == sockfd) {
            entry_index = i;
            break;
        }
    }

    // If sockfd is not found, return error
    if (entry_index == -1) {
        m_errno = EBADF; // Bad file descriptor
        return -1;
    }


    key_t shm_key2 = ftok("file2.txt",66);
    int shmid2 = shmget(shm_key2,0,0666);

    if(shmid2 == -1){
        m_errno=errno;
        perror("shmget");
        return -1;
    }

    SockInfo* sockinfo = (SockInfo*)shmat(shmid2,NULL,0);

    if(sockinfo == (void*)(-1)){
        m_errno=errno;
        perror("shmat");
        return -1;
    }

    sockinfo->sockid=sockfd;
    sockinfo->port=srcport;
    sockinfo->err_no=0;
    strcpy(sockinfo->ip,srcip);

    int semid1 = semget(SEMKEY1, 1, 0);
    int semid2 = semget(SEMKEY2,1,0);

    semop(semid1,&signal_operation,1);
    semop(semid2,&wait_operation,1);

    if(sockinfo->sockid == -1){
        m_errno=errno;
        return -1;
    }

    shared_memory->sockets[entry_index].port = destport;
    strcpy(shared_memory->sockets[entry_index].ip_address,destip);
    printf("After m_bind() call\n");
    printSM(shared_memory);
    shmdt(shared_memory);
    shmdt(sockinfo);
    return 0;
}








int m_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr_in *dest_addr, socklen_t addrlen) {
    // Attach to shared memory
    key_t shm_key = ftok("file1.txt", 65);
    int shm_id = shmget(shm_key, 0, 0666);
    if (shm_id == -1) {
        m_errno=errno;
        perror("shmget");
        return -1;
    }
    SharedMemory* shared_memory = (SharedMemory*)shmat(shm_id, NULL, 0);
    if (shared_memory == (void*)-1) {
        m_errno=errno;
        perror("shmat");
        return -1;
    }

    // Find the socket entry corresponding to sockfd
    int entry_index = -1;
    for (int i = 0; i < MAX_SOCKETS; ++i) {
        if (shared_memory->sockets[i].udp_socket_id == sockfd) {
            entry_index = i;
            break;
        }
    }

    // If sockfd is not found, return error
    if (entry_index == -1) {
        m_errno = EBADF; // Bad file descriptor
        return -1;
    }

    // Check if destination IP and port match the bound IP and port
    char dest_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(dest_addr->sin_addr), dest_ip, INET_ADDRSTRLEN);
    if(strcmp(dest_ip,shared_memory->sockets[entry_index].ip_address) != 0){
        m_errno = ENOTBOUND; // Not bound to destination IP/port
        return -1;
    }

    int flag=0;
    for(int i=0;i<MAX_BUFFER_SIZE_SENDER;i++){
        if(strcmp(shared_memory->sockets[entry_index].send_buffer[i].data,"") == 0){
            strcpy(shared_memory->sockets[entry_index].send_buffer[i].data,buf);
            flag=1;
            break;    
        }
    }


    // Detach shared memory
    shmdt(shared_memory);

    if(flag){
        return strlen(buf);
    }   
    else{
        return -1;
        m_errno=ENOBUFS;
    }
}







// Function to receive a message on an MTP socket
int m_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr_in *src_addr, socklen_t *addrlen) {
    // Attach to shared memory
    key_t shm_key = ftok("file1.txt", 65);
    int shm_id = shmget(shm_key, 0, 0666);

    if (shm_id == -1) {
        m_errno=errno;
        perror("shmget");
        return -1;
    }

    SharedMemory* shared_memory = (SharedMemory*)shmat(shm_id, NULL, 0);
    if (shared_memory == (void*)-1) {
        m_errno=errno;
        perror("shmat");
        return -1;
    }

    // Find the socket entry corresponding to sockfd
    int entry_index = -1;
    for (int i = 0; i < MAX_SOCKETS; ++i) {
        if (shared_memory->sockets[i].udp_socket_id == sockfd) {
            entry_index = i;
            break;
        }
    }

    // If sockfd is not found, return error
    if (entry_index == -1) {
        m_errno = EBADF; // Bad file descriptor
        return -1;
    }

    // Check if there is a message available in the receive buffer
    if (strcmp(shared_memory->sockets[entry_index].receive_buffer[0].data,"") == 0) {
        m_errno = ENOMSG; // No message available
        return -1;
    }

    strcpy(buf,shared_memory->sockets[entry_index].receive_buffer[0].data);


    for (int i = 0; i < MAX_BUFFER_SIZE_RECEIVER - 1; ++i) {
        shared_memory->sockets[entry_index].receive_buffer[i]=shared_memory->sockets[entry_index].receive_buffer[i + 1];
    }

    // Clear the last entry in the receive buffer
    memset(shared_memory->sockets[entry_index].receive_buffer[MAX_BUFFER_SIZE_RECEIVER - 1].data,'\0',sizeof(shared_memory->sockets[entry_index].receive_buffer[MAX_BUFFER_SIZE_RECEIVER - 1].data));
    shared_memory->sockets[entry_index].receive_buffer[MAX_BUFFER_SIZE_RECEIVER - 1].msg_header.sequence_number=-1;
    shared_memory->sockets[entry_index].receive_buffer[MAX_BUFFER_SIZE_RECEIVER - 1].msg_header.ty=-1;
    // Detach shared memory
    shmdt(shared_memory);

    return strlen(buf); // Return number of bytes received
}






int m_close(int sockfd) {
    // Attach to shared memory
    key_t shm_key = ftok("file1.txt", 65);
    int shm_id = shmget(shm_key, 0, 0666);
    if (shm_id == -1) {
        m_errno=errno;
        perror("shmget");
        return -1;
    }
    SharedMemory* shared_memory = (SharedMemory*)shmat(shm_id, NULL, 0);
    if (shared_memory == (void*)-1) {
        m_errno=errno;
        perror("shmat");
        return -1;
    }

    // Find the socket entry corresponding to sockfd
    int entry_index = -1;
    for (int i = 0; i < MAX_SOCKETS; ++i) {
        if (shared_memory->sockets[i].udp_socket_id == sockfd) {
            entry_index = i;
            break;
        }
    }

    // If sockfd is not found, return error
    if (entry_index == -1) {
        m_errno = EBADF; // Bad file descriptor
        return -1;
    }

    // Mark the socket entry as free
    shared_memory->sockets[entry_index].is_free = 1;

    // Detach shared memory
    shmdt(shared_memory);

    // Close the UDP socket
    int close_result = close(sockfd);
    if (close_result == -1) {
        m_errno=errno;
        perror("close");
        return -1;
    }

    return 0; // Return success
}