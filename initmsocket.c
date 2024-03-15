#include "msocket.h"
#define SEMKEY1 6969
#define SEMKEY2 6970
#define SEMKEYMUTEX 6971

struct sembuf wait_operation = {0,-1,0};
struct sembuf signal_operation = {0,1,0};
int semid1;
int semid2;
int semmutex;
int shmid1;
int shmid2;


void remove_shared_memory() {
    if (shmctl(shmid1, IPC_RMID, NULL) == -1) {
        perror("shmctl: remove_shared_memory");
        exit(EXIT_FAILURE);
    }
    if (shmctl(shmid2, IPC_RMID, NULL) == -1) {
        perror("shmctl: remove_shared_memory");
        exit(EXIT_FAILURE);
    }
    printf("Shared memory removed.\n");
}

// Function to remove semaphore
void remove_semaphore() {
    if (semctl(semid1, 0, IPC_RMID) == -1) {
        perror("semctl: remove_semaphore");
        exit(EXIT_FAILURE);
    }
    if (semctl(semid2, 0, IPC_RMID) == -1) {
        perror("semctl: remove_semaphore");
        exit(EXIT_FAILURE);
    }
    if (semctl(semmutex, 0, IPC_RMID) == -1) {
        perror("semctl: remove_semaphore");
        exit(EXIT_FAILURE);
    }
    printf("Semaphores removed.\n");
}

// Signal handler for Ctrl+C
void signal_handler(int signal) {
    printf("Ctrl+C received. Cleaning up...\n");
    remove_shared_memory();
    remove_semaphore();
    exit(EXIT_SUCCESS);
}


// Thread R function
void* thread_R(void* arg) {
    SharedMemory* shared_memory = (SharedMemory*)arg;
    int fnospace[MAX_SOCKETS];
    memset(fnospace,0,sizeof(fnospace));
    while (1) {

        fd_set readfds;
        FD_ZERO(&readfds);
        int maxfd = 0;
        semop(semmutex,&wait_operation,1);
        for(int i=0;i<MAX_SOCKETS;i++){
            if(shared_memory->sockets[i].is_free == 0){
                // printf("Socket %d is not free\n",i);
                FD_SET(shared_memory->sockets[i].udp_socket_id,&readfds);
                if(shared_memory->sockets[i].udp_socket_id>maxfd){
                    maxfd = shared_memory->sockets[i].udp_socket_id;
                }
            }
        }
        semop(semmutex,&signal_operation,1);

        struct timeval tv;
        tv.tv_sec = T;
        tv.tv_usec = 0;

        int activity = select(maxfd+1,&readfds,NULL,NULL,&tv);

        if(activity<0){
            perror("select");
            pthread_exit(NULL);
        }

        if(activity==0){
            semop(semmutex,&wait_operation,1);
            for(int i=0;i<MAX_SOCKETS;i++){
                if(fnospace[i]){
                    if(shared_memory->sockets[i].is_free == 0){
                        if(shared_memory->sockets[i].receive_buffer[shared_memory->sockets[i].wrr].ismsg == 0){
                            fnospace[i] = 0;
                            Message ackmsg;
                            ackmsg.msg_header.sequence_number = (shared_memory->sockets[i].rwnd.ptr1+15)%16;
                            ackmsg.msg_header.ty = 2;
                            memset(ackmsg.data,'\0',sizeof(ackmsg.data));
                            
                            // Calculate the size of the receiver window
                            int cnt = 0;
                            while(cnt<5 && shared_memory->sockets[i].receive_buffer[(shared_memory->sockets[i].wrr+cnt)%5].ismsg == 0){
                                cnt++;
                            }

                            shared_memory->sockets[i].rwnd.ptr2 = (shared_memory->sockets[i].rwnd.ptr1+cnt-1)%16;

                            ackmsg.msg_header.lastsenttime = time(NULL);
                            sprintf(ackmsg.data,"%d",cnt);

                            printf("ACK seg no = %d and size = %d\n",ackmsg.msg_header.sequence_number,cnt);
                            struct sockaddr_in servaddr;
                            servaddr.sin_family = AF_INET;
                            servaddr.sin_port = htons(shared_memory->sockets[i].port);
                            inet_aton(shared_memory->sockets[i].ip_address,&servaddr.sin_addr);
                            sendto(shared_memory->sockets[i].udp_socket_id,(void *)(&ackmsg),sizeof(ackmsg),0,(struct sockaddr*)&servaddr,sizeof(servaddr));
                        }
                    }
                }
            }
            semop(semmutex,&signal_operation,1);
            printf("Timeout\n");
            continue;
        }


        semop(semmutex,&wait_operation,1);
        for (int i = 0; i < MAX_SOCKETS; i++)
        {      
            if(shared_memory->sockets[i].is_free == 0){
                if(FD_ISSET(shared_memory->sockets[i].udp_socket_id,&readfds)){
                    struct sockaddr_in cliaddr;
                    int len = sizeof(cliaddr);
                    Message msg;
                    int n = recvfrom(shared_memory->sockets[i].udp_socket_id,(void *)(&msg),sizeof(msg),0,(struct sockaddr*)&cliaddr,&len);

                    if(n<0){
                        semop(semmutex,&signal_operation,1);
                        perror("recvfrom");
                        continue;
                    }

                    // If client not same as the one who sent the message, ignore
                    if(strcmp(shared_memory->sockets[i].ip_address,inet_ntoa(cliaddr.sin_addr))!=0 || shared_memory->sockets[i].port!=ntohs(cliaddr.sin_port)){
                        semop(semmutex,&signal_operation,1);
                        continue;
                    }

                    if(n==0){
                        semop(semmutex,&signal_operation,1);
                        continue;
                    }
                    
                    Window senderwindow = shared_memory->sockets[i].swnd;
                    Window receiverwindow = shared_memory->sockets[i].rwnd;
                    
                    if(msg.msg_header.ty == 2){
                        // ACK message
                        int ack = msg.msg_header.sequence_number; //received till this index number

                        int temp1 = senderwindow.ptr1;

                        printf("Received ack = %d\n",ack);
                        
                        while(temp1 != (ack+1)%16){
                            
                            for(int j=0;j<MAX_BUFFER_SIZE_SENDER;j++){
                                if(shared_memory->sockets[i].send_buffer[j].ismsg && shared_memory->sockets[i].send_buffer[j].msg_header.sequence_number == temp1){
                                    shared_memory->sockets[i].send_buffer[j].ismsg = 0;
                                    shared_memory->sockets[i].send_buffer[j].msg_header.lastsenttime = -1;
                                }
                            }
                            temp1 = (temp1+1)%16;
                        }

                        // for(int j=0;j<MAX_BUFFER_SIZE_SENDER;j++){
                        //     if(shared_memory->sockets[i].send_buffer[j].ismsg && shared_memory->sockets[i].send_buffer[j].msg_header.sequence_number <= ack){
                        //         shared_memory->sockets[i].send_buffer[j].ismsg = 0;
                        //         shared_memory->sockets[i].send_buffer[j].msg_header.lastsenttime = -1;
                        //     }
                        // }

                        senderwindow.ptr1 = (ack+1)%16;

                        // Convert the message data to int
                        int recvsizeleft = atoi(msg.data);
                        
                        senderwindow.ptr2 = (senderwindow.ptr1+recvsizeleft-1+16)%16;
                        senderwindow.size = recvsizeleft;

                        printf("Sender window ptr1 = %d, ptr2 = %d, size = %d\n",senderwindow.ptr1,senderwindow.ptr2,senderwindow.size);

                        shared_memory->sockets[i].swnd = senderwindow;

                    }
                    else if(msg.msg_header.ty == 1){
                        printf("In actual message\n");
                        // Actual message
                        int seq = msg.msg_header.sequence_number;
                        int ptr1 = receiverwindow.ptr1;
                        int ptr2 = receiverwindow.ptr2;

                        int temp1=ptr1;
                        int temp2=ptr2;
                        int flag=0;

                        while(temp1 != ((temp2+1)%16)){
                            // printf("temp1 = %d,seq = %d\n",temp1,seq);
                            if(seq == temp1){
                                flag=1;
                                break;
                            }
                            temp1 = (temp1+1)%16;
                        }

                        printf("Sequence number = %d\n",seq);
                        printf("Flag = %d\n",flag);
                        printf("14 idx is msg = %d\n",shared_memory->sockets[i].receive_temp_buffer[14].ismsg);
                        if(flag && shared_memory->sockets[i].receive_temp_buffer[seq].ismsg == 0){
                            strcpy(shared_memory->sockets[i].receive_temp_buffer[seq].data,msg.data);
                            shared_memory->sockets[i].receive_temp_buffer[seq].ismsg = 1;
                        }

                        int wrr = shared_memory->sockets[i].wrr;
                        flag = 0;
                        printf("ptr1 = %d, ptr2 = %d, size = %d\n",receiverwindow.ptr1,receiverwindow.ptr2,receiverwindow.size);
                        for(int k=0;k<receiverwindow.size;k++){
                            int j = (ptr1+k)%16;
                            printf("j = %d, ismsg = %d\n",j,shared_memory->sockets[i].receive_temp_buffer[j].ismsg);
                            if(shared_memory->sockets[i].receive_temp_buffer[j].ismsg == 1){
                                printf("Message %s\n",shared_memory->sockets[i].receive_temp_buffer[j].data);
                                printf("Writing at index %d\n",wrr);
                                strcpy(shared_memory->sockets[i].receive_buffer[wrr].data,shared_memory->sockets[i].receive_temp_buffer[j].data);
                                shared_memory->sockets[i].receive_buffer[wrr].ismsg = 1;
                                wrr = (wrr+1)%5;
                                shared_memory->sockets[i].wrr = wrr;
                                shared_memory->sockets[i].receive_temp_buffer[j].ismsg = 0;
                                receiverwindow.ptr1 = (receiverwindow.ptr1+1)%16;
                                flag = 1;
                            }
                            else{
                                break;
                            }
                        }


                        int cnt = 0;
                        printf("wrr = %d\n",wrr);
                        printf("(wrr+cnt)mod5 = %d\n",(wrr+cnt)%5);
                        printf("ismsg = %d\n",shared_memory->sockets[i].receive_buffer[(wrr+cnt)%5].ismsg);
                        while(cnt<5 && shared_memory->sockets[i].receive_buffer[(wrr+cnt)%5].ismsg == 0){
                            cnt++;
                        }

                        printf("cnt = %d\n",cnt);
                        if(cnt ==0) receiverwindow.ptr2 = receiverwindow.ptr1;
                        else receiverwindow.ptr2 = (receiverwindow.ptr1+cnt-1)%16;

                        receiverwindow.size = cnt;

                        shared_memory->sockets[i].rwnd = receiverwindow;

                        if(flag){
                            Message ackmsg;
                            ackmsg.msg_header.sequence_number = (receiverwindow.ptr1+15)%16;
                            ackmsg.msg_header.ty = 2;
                            memset(ackmsg.data,'\0',sizeof(ackmsg.data));
                            sprintf(ackmsg.data,"%d",receiverwindow.size);
                            ackmsg.msg_header.lastsenttime = time(NULL);

                            struct sockaddr_in servaddr;
                            memset(&servaddr,0,sizeof(servaddr));
                            servaddr.sin_family = AF_INET;
                            servaddr.sin_port = htons(shared_memory->sockets[i].port);
                            inet_aton(shared_memory->sockets[i].ip_address,&servaddr.sin_addr);
                            sendto(shared_memory->sockets[i].udp_socket_id,(void *)(&ackmsg),sizeof(ackmsg),0,(struct sockaddr*)&servaddr,sizeof(servaddr));
                            printf("ACK seg no = %d and size = %d\n",ackmsg.msg_header.sequence_number,receiverwindow.size);
                        }

                        if(cnt == 0){
                            fnospace[i] = 1;
                        }
                        
                    }

                }
            }
        
        }
        semop(semmutex,&signal_operation,1);
    }  
} 

// Thread S function
void* thread_S(void* arg) {
    SharedMemory* shared_memory = (SharedMemory*)arg;

    while (1) {
        usleep((T*1000000)/4);
        // printf("Here in thread s\n");
        semop(semmutex,&wait_operation,1);
        // printf("Here in thread s after aquiring semaphore\n");
        for(int i=0;i<MAX_SOCKETS;i++){
            if(shared_memory->sockets[i].is_free == 0){         //If it is a valid entry in the shared memory
                Window senderwindow = shared_memory->sockets[i].swnd;
                if(senderwindow.size!=0){
                    int temp1 = senderwindow.ptr1;
                    int temp2 = senderwindow.ptr2;

                    int flag=0;

                    while(temp1!=(temp2+1)%16){         //handling messages within the window
                        for(int j=0;j<MAX_BUFFER_SIZE_SENDER;j++){

                            // if it is a message within the sender window then do as needed
                            if((shared_memory->sockets[i].send_buffer[j].ismsg) && (shared_memory->sockets[i].send_buffer[j].msg_header.sequence_number == temp1) && (shared_memory->sockets[i].send_buffer[j].msg_header.lastsenttime!=-1)){
                                time_t currtime;
                                time(&currtime);

                                // printf("Difference between %ld and %ld is %ld\n",currtime,shared_memory->sockets[i].send_buffer[j].msg_header.lastsenttime,currtime-shared_memory->sockets[i].send_buffer[j].msg_header.lastsenttime);

                                //if it has been timed out then set flag
                                if((currtime - shared_memory->sockets[i].send_buffer[j].msg_header.lastsenttime)>=T){
                                    flag=1;
                                    // printf("Flag is set to 1\n");
                                }
                                break;
                            }
                        }
                        temp1 = (temp1+1)%16;
                    }


                    //message in the sender window has timed out then resend all the messages in the sender window
                    if(flag){
                        temp1 = senderwindow.ptr1;
                        temp2 = senderwindow.ptr2;
                        while(temp1!=(temp2+1)%16){         //handling messages within the window
                            for(int j=0;j<MAX_BUFFER_SIZE_SENDER;j++){

                                // if it is a message within the sender window then do as needed
                                if((shared_memory->sockets[i].send_buffer[j].ismsg) && (shared_memory->sockets[i].send_buffer[j].msg_header.sequence_number == temp1) && (shared_memory->sockets[i].send_buffer[j].msg_header.lastsenttime!=-1)){
                                    time_t currtime;
                                    time(&currtime);

                                    // shared_memory->sockets[i].send_buffer[j].msg_header.lastsenttime=currtime;

                                    struct sockaddr_in servaddr;
                                    memset(&servaddr,0,sizeof(servaddr));
                                    char ip[INET_ADDRSTRLEN];
                                    strcpy(ip,shared_memory->sockets[i].ip_address);
                                    inet_aton(ip,&servaddr.sin_addr);
                                    servaddr.sin_family = AF_INET;
                                    servaddr.sin_port=htons(shared_memory->sockets[i].port);
                                    shared_memory->sockets[i].send_buffer[j].msg_header.lastsenttime=currtime;
                                    Message msgtosend = shared_memory->sockets[i].send_buffer[j];
                                    sendto(shared_memory->sockets[i].udp_socket_id,(void *)(&msgtosend),sizeof(msgtosend),0,(struct sockaddr*)&servaddr,sizeof(servaddr));
                                    break;
                                }
                            }
                            temp1 = (temp1+1)%16;
                        }    
                    }
                }

                //handling first send for messages newly come into the buffer
                for(int j=0;j<MAX_BUFFER_SIZE_SENDER;j++){
                    if((shared_memory->sockets[i].send_buffer[j].ismsg) & (shared_memory->sockets[i].send_buffer[j].msg_header.lastsenttime == -1)){     //sending this msg for the first time
                        time_t currtime;
                        time(&currtime);
                        struct sockaddr_in servaddr;
                        memset(&servaddr,0,sizeof(servaddr));
                        char ip[INET_ADDRSTRLEN];
                        strcpy(ip,shared_memory->sockets[i].ip_address);
                        inet_aton(ip,&servaddr.sin_addr);
                        servaddr.sin_family = AF_INET;
                        servaddr.sin_port=htons(shared_memory->sockets[i].port);
                        shared_memory->sockets[i].send_buffer[j].msg_header.lastsenttime=currtime;
                        Message msgtosend = shared_memory->sockets[i].send_buffer[j];
                        sendto(shared_memory->sockets[i].udp_socket_id,(void *)(&msgtosend),sizeof(msgtosend),0,(struct sockaddr*)&servaddr,sizeof(servaddr));
                        printf("Sending this message %s\n",msgtosend.data);
                    }   
                }
            }
        }
        semop(semmutex,&signal_operation,1);
        
    }
    return NULL;
}

// Garbage collector function
void* garbage_collector(void* arg) {
    SharedMemory* shared_memory = (SharedMemory*)arg;
    
    while (1) {
        // Perform cleanup
        for (int i = 0; i < MAX_SOCKETS; ++i) {
            // if (!shared_memory->sockets[i].is_free && !kill(shared_memory->sockets[i].process_id, 0)) {
            //     // Process is not alive, clean up socket entry
            //     shared_memory->sockets[i].is_free = 1;
            //     // Reset other fields as needed
            // }
        }
        
        sleep(1);
    }
    
    return NULL;
}

int main() {

    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        return EXIT_FAILURE;
    }

    key_t shm_key = ftok("file1.txt", 65);
    shmid1 = shmget(shm_key, sizeof(SharedMemory), IPC_CREAT | 0666);

    // printf("shmid1 = %d\n",shmid1);

    if (shmid1 == -1) {
        perror("shmget");
        exit(1);
    }

    SharedMemory* shared_memory = (SharedMemory*)shmat(shmid1, NULL, 0);

    if (shared_memory == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    // Initialize shared memory
    for (int i = 0; i < MAX_SOCKETS; i++) {
        shared_memory->sockets[i].is_free = 1;
        // Initialize other fields as needed
    }

    key_t shm_key2 = ftok("file2.txt",66);
    shmid2 = shmget(shm_key2, sizeof(SockInfo), IPC_CREAT | 0666);
    if (shmid2 == -1) {
        perror("shmget");
        exit(1);
    }

    SockInfo* sockinfo = (SockInfo*)shmat(shmid2, NULL, 0);

    if (sockinfo == (void*)-1) {
        perror("shmat");
        exit(1);
    }

    sockinfo->err_no=0;
    strcpy(sockinfo->ip,"\0");
    sockinfo->port=0;
    sockinfo->sockid=0;

    semid1 = semget(SEMKEY1,1,IPC_CREAT | IPC_EXCL | 0666);

    if(semid1 == -1){
        perror("semget");
        exit(EXIT_FAILURE);
    }

    semid2 = semget(SEMKEY2,1,IPC_CREAT | IPC_EXCL | 0666);

    if(semid2 == -1){
        perror("semget");
        exit(EXIT_FAILURE);
    }

    semmutex = semget(SEMKEYMUTEX,1,IPC_CREAT | IPC_EXCL | 0666);

    if(semmutex == -1){
        perror("semget");
        exit(EXIT_FAILURE);
    }

    semctl(semid1, 0, SETVAL, 0);
    semctl(semid2, 0, SETVAL, 0);
    semctl(semmutex,0,SETVAL,1);


    // Create threads
    pthread_t thread_R_id, thread_S_id, garbage_collector_id;
    pthread_attr_t attr;
    // Make attr detached
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread_R_id,&attr, thread_R, (void*)shared_memory);
    pthread_create(&thread_S_id,&attr, thread_S, (void*)shared_memory);
    pthread_create(&garbage_collector_id,&attr, garbage_collector, (void*)shared_memory);



    while(1){
        semop(semid1,&wait_operation,1);
            if((sockinfo->sockid == 0) && (sockinfo->port==0)){      //m_socket call
                int udp_socket_id = socket(AF_INET, SOCK_DGRAM, 0);
                if(udp_socket_id == -1){
                    sockinfo->err_no=errno;
                }
                sockinfo->sockid = udp_socket_id;
            }
            else if((sockinfo->sockid != 0) && (sockinfo->port != -1)){                           //m_bind call
                int sockfd = sockinfo->sockid;
                struct sockaddr_in servaddr;
                memset(&servaddr, 0, sizeof(servaddr)); 
                char ip[INET_ADDRSTRLEN];
                strcpy(ip,sockinfo->ip);
                short port = sockinfo->port;

                servaddr.sin_family    = AF_INET; 
                inet_aton(ip, &servaddr.sin_addr); 
                servaddr.sin_port = htons(port); 
                
                if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
                        sizeof(servaddr)) < 0 ) 
                { 
                    perror("bind failed"); 
                    sockinfo->err_no=errno;
                    sockinfo->sockid=-1;
                }

            }
            else if((sockinfo->sockid != 0) && (sockinfo->port == -1)){     //m_close() call
                int sockfd = sockinfo->sockid;
                if(close(sockfd) == -1){
                    perror("close");
                    sockinfo->err_no=errno;
                }
                else{
                    sockinfo->sockid=0;     //successfully closed
                }
            }
        semop(semid2,&signal_operation,1);  
    }
    shmdt(shared_memory);
    return 0;
}