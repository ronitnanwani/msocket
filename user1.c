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
    // while(i<8){
    //     int retval;
    //     if(i<5){
    //         retval = m_sendto(sockfd,"Hello there",11,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    //         printf("Sent: %s\n","Hello there");
    //     }
    //     else{
    //         retval = m_sendto(sockfd,"Hello there2",11,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    //         printf("Sent: %s\n","Hello there2");
    //     }
    //     i++;
    // }

    int retval = m_sendto(sockfd,"Hello there",11,0,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    return 0;
}