#include<iostream>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
using namespace std;

#define port 8081

int main(){

    int sock = 0;
    struct sockaddr_in addr;

    sock = socket(PF_INET, SOCK_STREAM, 0);

    if(sock < 0){
        return 1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if((inet_pton(AF_INET, "127.0.0.1",&addr.sin_addr)) <= 0){
        return 1;
    }

    if(connect(sock, (struct sockaddr *)&addr, sizeof addr) < 0){
        return 1;
    }

    cout<<"Connected to server...Type commands (e.g., 'register anurag 1234')\n";

    while(true){
        string input;
        cout<<"> ";
        getline(cin,input);

        if(input == "exit"){
            break;
        }

        send(sock,input.c_str(),sizeof input, 0);
        char buffer[1024] = {0};
        int val = read(sock,buffer,sizeof(buffer));
        cout<<"Message from server is "<<buffer<<endl;
    }

    return 0;
}