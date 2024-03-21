#include "msocket.h"

int main(int argv,char* argc[]){
    if(argv!=3){
        printf("Usage: ./user1 <myport> <dest_port>\n");
        exit(1);
    }

    int myport = atoi(argc[1]);
    int dest_port = atoi(argc[2]);

    int sockfd = m_socket(AF_INET,SOCK_MTP, 0);
    m_bind(sockfd,"127.0.0.1",myport,"127.0.0.1",dest_port);
    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(dest_port);
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
        buffer[retval]='\0';
        if(retval>0){
            printf("Received: %s\n",buffer);
            i++;
        }
    }

    fprintf(stderr,"Done\n");
    while(1){

    }

    exit(0);
}