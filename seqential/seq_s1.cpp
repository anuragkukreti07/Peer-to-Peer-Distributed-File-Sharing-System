#include<iostream>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>


#define port 8080

using namespace std;


int main(){
    struct sockaddr_in addr;
    int sock = 0;
    sock = socket(PF_INET, SOCK_STREAM, 0);

    if(sock < 0){
        return 1;
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    //addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(inet_pton(AF_INET, "127.0.0.1",&addr.sin_addr) <= 0){
        return 1;
    }


    if(connect(sock, (struct sockaddr *)&addr ,sizeof(addr)) < 0){
        return 1;
    }

    int nums[2];
    cout<<"Enter two numbers to add\n";
    cin>>nums[0]>>nums[1];

    send(sock, nums, sizeof nums, 0);

    int res = 0;

    read(sock, &res, sizeof res);

    cout<<"Sum received is "<<res<<endl;

    const char* ack = "Sum is received\n";
    send(sock, ack, strlen(ack),0);

    close(sock);

    return 0;
}