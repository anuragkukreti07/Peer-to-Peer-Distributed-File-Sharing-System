
#include<iostream>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/wait.h>

#define port 8080

using namespace std;

int main(){

    int server_fd,new_soc;
    struct sockaddr_in addr;
    int addrlen = sizeof addr;

    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        return 1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if((bind(server_fd,(struct sockaddr *)&addr,addrlen)) < 0){
        return 1;
    }


    if(listen(server_fd,5) < 0){
        return 1;
    }

    cout<<"Server is listening...\n";

    while(true){

        new_soc = accept(server_fd, (struct sockaddr*)& addr,(socklen_t*)& addrlen);
        if(new_soc < 0){
            return 1;
        }

        pid_t pid = fork();

        if(pid == 0){

            close(server_fd);
            int nums[2];
            
            read(new_soc, &nums, sizeof nums);
            cout<<"Received "<<nums[0] <<" "<<nums[1]<<endl;

            int sum = nums[0] + nums[1];

            send(new_soc, &sum, sizeof sum, 0);
            

            char ack[1024] = {0};
            read(new_soc,ack,sizeof ack);

            cout<<"Acknowldegement received is "<<ack<<endl;
            close(new_soc);
            return 0;
        }else if(pid > 0){
            close(new_soc);
            waitpid(-1, NULL, WNOHANG);
        }
    }


    close(server_fd);

    return 0;
}