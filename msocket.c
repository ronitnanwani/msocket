#include "msocket.h"
#define SEMKEY1 6969
#define SEMKEY2 6970
#define SEMKEYMUTEX 6971

struct sembuf wait_operation = {0,-1,0};
struct sembuf signal_operation = {0,1,0};

int printmsgheader(message_header m){
    printf("Message header\n");
    printf("Sequence number %d\n",m.sequence_number);
    printf("Message type %d\n",m.ty);
    printf("Message last sent time %ld\n",m.lastsenttime);
}

int printmsg(Message m){
    printf("Message\n");
    printf("Is msg %d\n",m.ismsg);
    printmsgheader(m.msg_header);
    printf("%s\n",m.data);
}

int printWindow(Window w){
    printf("Window\n");
    printf("%d\n",w.size);
    printf("Ptr1 = %d\n",w.ptr1);
    printf("Ptr2 = %d\n",w.ptr2);
}

int printmtpsocket(MTPSocket s){
    printf("isfree %d\n",s.is_free);
    printf("process_id %d\n",s.process_id);
    printf("udp_socket_id %d\n",s.udp_socket_id);
    printf("Dest ip %s\n",s.ip_address);
    printf("Dest port %hd\n",s.port);
    printf("Curr = %d\n",s.curr);
    printf("Start receive = %d\n",s.str);
    printf("Current index to write to = %d\n",s.wrs);

    for(int i=0;i<MAX_BUFFER_SIZE_SENDER;i++){
        printf("%dth message\n",i);
        printmsg(s.send_buffer[i]);
    }
    for(int i=0;i<MAX_BUFFER_SIZE_RECEIVER;i++){
        printf("%dth message\n",i);
        printf("Is message %d\n",s.receive_buffer[i].ismsg);
        printf("%s\n",s.receive_buffer[i].data);
    }
    printWindow(s.swnd);
    printWindow(s.rwnd);
}

int printSM(SharedMemory* sm){
    for(int i=0;i<MAX_SOCKETS;i++){
        printf("Mtp Socket %d\n",i);
        printmtpsocket(sm->sockets[i]);
    }
}

int m_socket(int domain, int type, int protocol) {

    int semid1 = semget(SEMKEY1,1,0);
    int semid2 = semget(SEMKEY2,1,0);

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
    
    int semmutex = semget(SEMKEYMUTEX,1,0);

    if (type != SOCK_MTP) {
        m_errno = EINVAL;  // Invalid socket type
        return -1;
    }

    // Attach to shared memory
    key_t shm_key = ftok("file1.txt", 65);
    int shm_id = shmget(shm_key, 0, 0);
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

    semop(semmutex,&wait_operation,1);
    
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
        shmdt(shared_memory);
        shmdt(sockinfo);
        semop(semmutex,&signal_operation,1);
        return -1;
    }




    sockinfo->sockid=0;
    sockinfo->port=0;
    sockinfo->err_no=0;
    strcpy(sockinfo->ip,"\0");

    semop(semid1,&signal_operation,1);
    semop(semid2,&wait_operation,1);

    if(sockinfo->sockid == -1){
        shmdt(shared_memory);
        shmdt(sockinfo);
        semop(semmutex,&signal_operation,1);
        m_errno=errno;
        return -1;
    }

    
    shared_memory->sockets[free_entry_index].is_free = 0;
    shared_memory->sockets[free_entry_index].process_id = getpid();
    shared_memory->sockets[free_entry_index].udp_socket_id = sockinfo->sockid;
    shared_memory->sockets[free_entry_index].curr = 1;
    shared_memory->sockets[free_entry_index].str = 0;
    shared_memory->sockets[free_entry_index].wrs = 0;
    memset(shared_memory->sockets[free_entry_index].send_buffer,0,sizeof(shared_memory->sockets[free_entry_index].send_buffer));
    memset(shared_memory->sockets[free_entry_index].receive_buffer,0,sizeof(shared_memory->sockets[free_entry_index].receive_buffer));
    memset(shared_memory->sockets[free_entry_index].receive_temp_buffer,0,sizeof(shared_memory->sockets[free_entry_index].receive_temp_buffer));
    shared_memory->sockets[free_entry_index].swnd.size=5;
    shared_memory->sockets[free_entry_index].swnd.ptr1=1;
    shared_memory->sockets[free_entry_index].swnd.ptr2=5;
    shared_memory->sockets[free_entry_index].rwnd.size=5;
    shared_memory->sockets[free_entry_index].rwnd.ptr1=1;
    shared_memory->sockets[free_entry_index].rwnd.ptr2=5;
    int retval = free_entry_index;
    // printf("###########################################################\n");
    // printf("After m_socket() call\n");
    // printSM(shared_memory);
    shmdt(shared_memory);
    shmdt(sockinfo);
    semop(semmutex,&signal_operation,1);


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

    key_t shm_key2 = ftok("file2.txt",66);
    int shmid2 = shmget(shm_key2,0,0);

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
    // Find the socket entry corresponding to sockfd
    int semmutex = semget(SEMKEYMUTEX,1,0);

    int entry_index = sockfd;


    semop(semmutex,&wait_operation,1);
    sockinfo->sockid=shared_memory->sockets[entry_index].udp_socket_id;
    sockinfo->port=srcport;
    sockinfo->err_no=0;
    strcpy(sockinfo->ip,srcip);


    int semid1 = semget(SEMKEY1, 1, 0);
    int semid2 = semget(SEMKEY2,1,0);

    semop(semid1,&signal_operation,1);
    semop(semid2,&wait_operation,1);

    if(sockinfo->sockid == -1){
        m_errno=errno;
        shmdt(shared_memory);
        shmdt(sockinfo);
        return -1;
    }

    shared_memory->sockets[entry_index].port = destport;
    strcpy(shared_memory->sockets[entry_index].ip_address,destip);
    // printf("###########################################################\n");
    // printf("After m_bind() call\n");
    // printSM(shared_memory);
    semop(semmutex,&signal_operation,1);
    shmdt(shared_memory);
    shmdt(sockinfo);
    return 0;
}


ssize_t m_sendto(int sockfd, const void* buf, size_t len, int flags, struct sockaddr* dest_addr, socklen_t dest_len) {
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

    int semmutex = semget(SEMKEYMUTEX,1,0);

    int entry_index = sockfd;

    //do bad file descriptor check

    // Check if destination IP and port match the bound IP and port
    struct sockaddr_in* dest_add = (struct sockaddr_in*)(dest_addr);
    char dest_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET,&(dest_add->sin_addr.s_addr),dest_ip,INET_ADDRSTRLEN);
    short dest_port = ntohs(dest_add->sin_port);
    
    semop(semmutex,&wait_operation,1);
    if((strcmp(dest_ip,shared_memory->sockets[entry_index].ip_address)!=0) || (dest_port!=shared_memory->sockets[entry_index].port)){
        m_errno = ENOTBOUND; // Not bound to destination IP/port
        shmdt(shared_memory);
        semop(semmutex,&signal_operation,1);
        return -1;
    }

    int flag=0;

    if(shared_memory->sockets[entry_index].send_buffer[shared_memory->sockets[entry_index].wrs].ismsg==1){
        m_errno=ENOBUFS;
        shmdt(shared_memory);
        semop(semmutex,&signal_operation,1);
        return -1;
    }

    //can write to send buff;
    Message msgtowrite;
    msgtowrite.ismsg=1;
    msgtowrite.msg_header.sequence_number=shared_memory->sockets[entry_index].curr;
    shared_memory->sockets[entry_index].curr=(shared_memory->sockets[entry_index].curr+1)%16;
    msgtowrite.msg_header.ty=1;
    msgtowrite.msg_header.lastsenttime=-1;
    strcpy(msgtowrite.data,buf);
    shared_memory->sockets[entry_index].send_buffer[shared_memory->sockets[entry_index].wrs]=msgtowrite;
    shared_memory->sockets[entry_index].wrs = (shared_memory->sockets[entry_index].wrs+1)%MAX_BUFFER_SIZE_SENDER;
    // printf("###########################################################\n");
    // printf("After m_sendto call\n");
    // printSM(shared_memory);
    semop(semmutex,&signal_operation,1);
    shmdt(shared_memory);

    return strlen(buf);
}


// Function to receive a message on an MTP socket
ssize_t m_recvfrom(int sockfd, void *buf, size_t len,int flags,struct sockaddr* sender_addr,socklen_t sender_addr_len) {
    // Attach to shared memory
    key_t shm_key = ftok("file1.txt", 65);
    int shm_id = shmget(shm_key,0, 0666);

    if (shm_id == -1) {
        m_errno=errno;
        perror("shmget");
        return -1;
    }
    int semmutex = semget(SEMKEYMUTEX,1,0);

    SharedMemory* shared_memory = (SharedMemory*)shmat(shm_id, NULL, 0);
    if (shared_memory == (void*)-1) {
        m_errno=errno;
        perror("shmat");
        return -1;
    }

    semop(semmutex,&wait_operation,1); 
    // Find the socket entry corresponding to sockfd
    int entry_index = sockfd;

    //do badfd check
    if(shared_memory->sockets[entry_index].receive_buffer[shared_memory->sockets[entry_index].str].ismsg == 0){
        m_errno = ENOMSG; // No message available
        shmdt(shared_memory);
        semop(semmutex,&signal_operation,1);
        return -1;
    }


    strcpy(buf,shared_memory->sockets[entry_index].receive_buffer[shared_memory->sockets[entry_index].str].data);
    shared_memory->sockets[entry_index].receive_buffer[shared_memory->sockets[entry_index].str].ismsg=0;
    printf("str = %d\n",shared_memory->sockets[entry_index].str);
    shared_memory->sockets[entry_index].str = (shared_memory->sockets[entry_index].str+1)%MAX_BUFFER_SIZE_RECEIVER;

    shmdt(shared_memory);
    semop(semmutex,&signal_operation,1);


    // printf("###########################################################\n");
    // printf("After m_recvfrom() call\n");

    return strlen(buf); // Return number of bytes received
}




int m_close(int sockfd) {

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

    int semid1 = semget(SEMKEY1, 1, 0);
    int semid2 = semget(SEMKEY2,1,0);


    // Find the socket entry corresponding to sockfd
    int entry_index = sockfd;
    //handle badfd error


    int semmutex = semget(SEMKEYMUTEX,1,0);

    semop(semmutex,&wait_operation,1);
    sockinfo->sockid=shared_memory->sockets[entry_index].udp_socket_id;
    sockinfo->port=-1;
    semop(semid1,&signal_operation,1);
    semop(semid2,&wait_operation,1);

    int close_result;
    if(sockinfo->sockid==0){        //successfully removed
        shared_memory->sockets[entry_index].is_free = 1;
        close_result=0;
    }
    else{                           //error caused in closing the socket
        m_errno = sockinfo->err_no;
        close_result=-1;
    }


    // printf("###########################################################\n");
    // printf("After m_close() call\n");
    // printSM(shared_memory);
    semop(semmutex,&signal_operation,1);
    // Detach shared memory
    shmdt(shared_memory);

    return close_result;
}