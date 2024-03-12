#include "msocket.h"

int main(){
    key_t shm_key = ftok("file1.txt", 65);
    int shm_id = shmget(shm_key,0, 0666);
    if (shm_id == -1) {
        m_errno=errno;
        perror("shmget");
        return -1;
    }
}