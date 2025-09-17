#include<iostream>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>


#define port 8080

using namespace std;



int main(){

    int server_fd, new_soc;
    struct sockaddr_in addr;

    int addrlen = sizeof addr;


    //creating part
    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(server_fd == 0){
        return 1;
    }

    //binding
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if((bind(server_fd,(struct sockaddr *)&addr,addrlen)) < 0){
        return 1;
    }


    //listening

    if(listen(server_fd,3) < 0){
        return 1;
    }

    cout<<"Server active\n";


    new_soc = accept(server_fd,(struct sockaddr*)&addr,(socklen_t *)&addrlen);
    if(new_soc < 0){
        return 1;
    }


    char buffer[1024] = {};

    read(new_soc,buffer,sizeof buffer);
    cout<<"Message read is "<<buffer<<endl;


    const char* msg = "Message from receiver.cpp";
    send(new_soc, msg, strlen(msg),0);


    close(new_soc);
    close(server_fd);



    return 0;
}