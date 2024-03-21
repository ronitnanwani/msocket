#include "msocket.h"

int main(int argv,char* argc[]){
    if(argv!=3){
        printf("Usage: ./user2 <myport> <dest_port>\n");
        exit(1);
    }
    int myport = atoi(argc[1]);
    int dest_port = atoi(argc[2]);

    char recv_filename[100];
    sprintf(recv_filename,"%dr.txt",dest_port);
    FILE* fpr = fopen(recv_filename,"w");

    char send_filename[100];
    sprintf(send_filename,"%d.txt",myport);
    FILE* fps = fopen(send_filename,"r");

    int sockfd = m_socket(AF_INET,SOCK_MTP, 0);
    m_bind(sockfd,"127.0.0.1",myport,"127.0.0.1",dest_port);
    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(myport);
    inet_aton("127.0.0.1",&serv_addr.sin_addr);
    // int retval = m_sendto(sockfd,"Hello there",11,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    char buffer[1025];
    // memset(buffer,'\0',1025);
    // sleep(10);
    int i=0;

    while(1){
        socklen_t serv_addr_len=sizeof(serv_addr);
        int retval = m_recvfrom(sockfd,buffer,1024,0,(struct sockaddr*)&serv_addr,&serv_addr_len);
        if(retval<0){
            continue;
        }
        buffer[retval]='\0';
        fwrite(buffer,1,strlen(buffer)+1,fpr);
    }

    // printf("\nDone receiving and now sending\n\n");

    // size_t bytes_read;
    // while ((bytes_read = fread(buffer, 1, 1024, fps)) > 0) {
    //     int retval=-1;
    //     buffer[bytes_read]='\0';
    //     char sendm[1025];
    //     strcpy(sendm,buffer);
    //     sendm[bytes_read]='\0';
    //     while(retval<0){
    //         retval = m_sendto(sockfd,sendm,bytes_read+1,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    //     }
    //     printf("%d\n",retval);
    // }
    // fprintf(stderr,"Done\n");
    fclose(fpr);
    fclose(fps);
    while(1){

    }
    return 0;
}