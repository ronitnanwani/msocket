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
    while (1) {
        // Receive messages from UDP socket and handle them
        // Update shared memory (e.g., message_buffer, rwnd) accordingly
        // This thread should handle incoming messages from UDP socket
        // Sleep for demonstration purpose (replace with actual receive code)
        sleep(1);
    }
    return NULL;
}

// Thread S function
void* thread_S(void* arg) {
    SharedMemory* shared_memory = (SharedMemory*)arg;
    while (1) {
        // Send messages and handle timeouts and retransmissions
        // Update shared memory (e.g., message_buffer, swnd) accordingly
        // This thread should handle sending messages, timeouts, and retransmissions
        // Sleep for demonstration purpose (replace with actual send code)
        sleep(1);
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



    // Create threads
    pthread_t thread_R_id, thread_S_id, garbage_collector_id;
    pthread_create(&thread_R_id, NULL, thread_R, (void*)shared_memory);
    pthread_create(&thread_S_id, NULL, thread_S, (void*)shared_memory);
    pthread_create(&garbage_collector_id, NULL, garbage_collector, (void*)shared_memory);

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


    while(1){
        semop(semid1,&wait_operation,1);
            if(sockinfo->sockid == 0){      //m_socket call
                int udp_socket_id = socket(AF_INET, SOCK_DGRAM, 0);
                if(udp_socket_id == -1){
                    sockinfo->err_no=errno;
                }
                sockinfo->sockid = udp_socket_id;
            }
            else{                           //m_bind call
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
        semop(semid2,&signal_operation,1);  
    }
    // // Wait for threads to finish (not really applicable in this case)
    // pthread_join(thread_R_id, NULL);
    // pthread_join(thread_S_id, NULL);
    // pthread_join(garbage_collector_id, NULL);
    // Detach shared memory
    shmdt(shared_memory);

    // Remove shared memory segment if needed
    // shmctl(shm_id, IPC_RMID, NULL);

    return 0;
}