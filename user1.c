#include "msocket.h"

int main(){
    int sockfd = m_socket(AF_INET,SOCK_MTP, 0);
    m_bind(sockfd,"127.0.0.1",6000,"127.0.0.1",6001);
    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(6001);
    inet_aton("127.0.0.1",&serv_addr.sin_addr);
    int len = sizeof(serv_addr);
    int i=0;
    while(i<25){
        int retval;
        char sendm[1024];
        sprintf(sendm,"Hello there %d",i);
        retval = m_sendto(sockfd,sendm,1024,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
        if(retval<0){
            continue;
        }
        printf("Sent: %s\n",sendm);
        i++;
    }

    printf("\nDone sending and now receving\n\n");

    char buffer[1024];
    i=0;
    while(i<25){
        socklen_t serv_addr_len=sizeof(serv_addr);
        int retval = m_recvfrom(sockfd,buffer,1024,0,(struct sockaddr*)&serv_addr,&serv_addr_len);
        if(retval>0){
            printf("Received: %s\n",buffer);
            i++;
        }
    }

    while(1){

    }

    return 0;
}