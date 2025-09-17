#include<iostream>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>


#define port 8080

using namespace std;

int main(){

    int sock = 0;
    struct sockaddr_in serv_addr;

    sock = socket(PF_INET,SOCK_STREAM,0);

    if(sock < 0){
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    
    if(inet_pton(AF_INET,"127.0.0.1",&serv_addr.sin_addr) <= 0){
        return 1;
    }

    if((connect(sock,(struct sockaddr *)& serv_addr,sizeof serv_addr)) < 0){
        return 1;
    }

    const char* msg = "Message from the client\n";
    send(sock,msg,strlen(msg),0);

    char buffer[1024] = {0};

    read(sock,buffer, sizeof buffer);
    cout<<"Message received by client is "<<buffer<<endl;

    close(sock);

    return 0;
}