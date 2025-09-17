#include<iostream>
#include<unistd.h>
#include<string.h>
#include<sstream>
#include<bits/stdc++.h>
#include<arpa/inet.h>
#include<sys/wait.h>
#include<pthread.h>
using namespace std;

#define port 8081

unordered_map<string,string> users;
unordered_set<string> logged_curr;
pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;



vector<string> split(string& s){
    stringstream ss(s);
    string words;
    vector<string> tokens;

    while(ss>>words){
        tokens.push_back(words);
    }

    return tokens;
}


string handleCommand(vector<string>& cmd){
    if(cmd.size() == 0){
        return "Invalid\n";
    }


    if(cmd[0] == "register"){

        if(cmd.size() != 3){
            return "Usage: register <username> <password>";
        }

        string uname = cmd[1],pass = cmd[2];
        if(users.count(uname) != 0){
            return "Error as user already exists\n";
        }
        users[uname] = pass;
        return "Registered user " + uname + " successfully";


    }else if (cmd[0] == "login"){


        if(cmd.size() != 3){
            return "Usage: login <username> <password>";
        }
        string uname = cmd[1],pass= cmd[2];
        if(users.count(uname) == 0){
            return "User doesnt exist\n";
        }else if(users[uname] != pass){
            return "Incorrect pass\n";
        }else if(logged_curr.count(uname)){
            return "User is already logged in\n";
        }else{
            logged_curr.insert(uname);
        }
        return "User " + uname + " logged in successfully\n";


    }else if(cmd[0] == "logout"){
        if(cmd.size() != 2){
            return "Usage: logout <username>";
        }
        string uname = cmd[1];
        if(logged_curr.count(uname) == 0){
            return "Error: user not logged in";
        }
        logged_curr.erase(uname);
        return "User " + uname + " logged out successfully";


    }else if(cmd[0] == "create_group"){
        
        if(cmd.size() != 2){
            return "Usage: create_group <group_id>";
        }
        return "Group " + cmd[1] + " created";

    }else{
        return "Unknown command: " + cmd[0];
    }
}



void *clientHandler(void *arg){
    int new_sock = *((int*)arg);
    delete (int*)arg;

    char buffer[1024] = {0};
    while (true){
        memset(buffer, 0, sizeof(buffer));
        int val = read(new_sock, buffer, sizeof(buffer));
        if(val <= 0){
            break;
        }

        string input(buffer);
        auto tokens = split(input);
        string resp = handleCommand(tokens);

        send(new_sock, resp.c_str(), resp.size(), 0);
    }

    close(new_sock);
    cout<<"CLient handled successfully..\n";
    pthread_exit(nullptr);
}


int main(){

    int server_fd = 0, new_soc = 0;
    struct sockaddr_in addr;
    int addrlen = sizeof addr;

    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        perror("socket failed check\n");
        return 1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if((bind(server_fd,(struct sockaddr*)& addr, addrlen)) < 0){
        perror("bind failed check\n");
        return 1;
    }

    if(listen(server_fd,5) < 0){
        perror("listen failed check\n");
        return 1;
    }

    cout<<"Server is listening \n";

    while(true){
        int *new_sock = new int;
        *new_sock = accept(server_fd, (struct sockaddr *)&addr,(socklen_t*) &addrlen);
        if (*new_sock < 0) {
            perror("accept failed");
            delete new_sock;
            continue;
        }

        cout << "New client connected...\n";
        pthread_t tid;
        pthread_create(&tid, nullptr, clientHandler, new_sock);
        pthread_detach(tid);
    }


    close(server_fd);
    return 0;

}