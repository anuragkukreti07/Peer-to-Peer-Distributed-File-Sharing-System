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
unordered_map<string, string> groupOwner;
unordered_map<string, unordered_set<string>> groupMembers;
unordered_map<string, unordered_set<string>> joinRequests;
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


string handleCommand(vector<string>& cmd,string& currentUser){
    if(cmd.size()==0){
        return "Invalid\n";
    }else if(cmd[0]=="create_user" && cmd.size()==3){
        pthread_mutex_lock(&lock1);
        string uname=cmd[1],pass=cmd[2];
        if(users.count(uname)){
            pthread_mutex_unlock(&lock1);
            return "Error: user already exists\n";
        }
        users[uname]=pass;
        pthread_mutex_unlock(&lock1);
        return "User "+uname+" created successfully\n";
    }else if(cmd[0]=="login" && cmd.size()==3){
        pthread_mutex_lock(&lock1);
        string uname=cmd[1],pass=cmd[2];
        if(users.count(uname)==0){
            pthread_mutex_unlock(&lock1);
            return "Error: user does not exist\n";
        }
        if(users[uname]!=pass){
            pthread_mutex_unlock(&lock1);
            return "Error: incorrect password\n";
        }
        if(logged_curr.count(uname)){
            pthread_mutex_unlock(&lock1);
            return "Error: user already logged in\n";
        }
        logged_curr.insert(uname);
        currentUser=uname;
        pthread_mutex_unlock(&lock1);
        return "User "+uname+" logged in successfully\n";
    } else if(cmd[0]=="create_group" && cmd.size()==2){
        if(currentUser=="") return "Error: please login first\n";
        pthread_mutex_lock(&lock1);
        string gid=cmd[1];
        if(groupOwner.count(gid)){
            pthread_mutex_unlock(&lock1);
            return "Error: group already exists\n";
        }
        groupOwner[gid]=currentUser;
        groupMembers[gid].insert(currentUser);
        pthread_mutex_unlock(&lock1);
        return "Group "+gid+" created successfully\n";
    }else if(cmd[0]=="join_group" && cmd.size()==2){
        if(currentUser=="") return "Error: please login first\n";
        pthread_mutex_lock(&lock1);
        string gid=cmd[1];
        if(groupOwner.count(gid)==0){
            pthread_mutex_unlock(&lock1);
            return "Error: group does not exist\n";
        }
        if(groupMembers[gid].count(currentUser)){
            pthread_mutex_unlock(&lock1);
            return "Error: already a member\n";
        }
        joinRequests[gid].insert(currentUser);
        pthread_mutex_unlock(&lock1);
        return "Join request sent for group "+gid+"\n";
    }else if(cmd[0]=="leave_group" && cmd.size()==2){
        if(currentUser=="") return "Error: please login first\n";
        pthread_mutex_lock(&lock1);
        string gid=cmd[1];
        if(groupOwner.count(gid)==0){
            pthread_mutex_unlock(&lock1);
            return "Error: group does not exist\n";
        }
        if(groupMembers[gid].count(currentUser)==0){
            pthread_mutex_unlock(&lock1);
            return "Error: not a member\n";
        }
        if(groupOwner[gid]==currentUser){
            pthread_mutex_unlock(&lock1);
            return "Error: owner cannot leave the group\n";
        }
        groupMembers[gid].erase(currentUser);
        pthread_mutex_unlock(&lock1);
        return "User "+currentUser+" left group "+gid+"\n";
    }else if(cmd[0]=="list_groups" && cmd.size()==1){
        pthread_mutex_lock(&lock1);
        if(groupOwner.empty()){
            pthread_mutex_unlock(&lock1);
            return "No groups available\n";
        }
        string res="Groups:\n";
        for(auto &g:groupOwner){
            res+=g.first+"\n";
        }
        pthread_mutex_unlock(&lock1);
        return res;
    }else if(cmd[0]=="list_requests" && cmd.size()==2){
        if(currentUser=="") return "Error: please login first\n";
        pthread_mutex_lock(&lock1);
        string gid=cmd[1];
        if(groupOwner.count(gid)==0){
            pthread_mutex_unlock(&lock1);
            return "Error: group does not exist\n";
        }
        if(groupOwner[gid]!=currentUser){
            pthread_mutex_unlock(&lock1);
            return "Error: only owner can view requests\n";
        }
        if(joinRequests[gid].empty()){
            pthread_mutex_unlock(&lock1);
            return "No pending requests\n";
        }
        string res="Pending requests for group "+gid+":\n";
        for(auto &u:joinRequests[gid]){
            res+=u+"\n";
        }
        pthread_mutex_unlock(&lock1);
        return res;
    }else if(cmd[0]=="accept_request" && cmd.size()==3){
        if(currentUser=="") return "Error: please login first\n";
        pthread_mutex_lock(&lock1);
        string gid=cmd[1],uname=cmd[2];
        if(groupOwner.count(gid)==0){
            pthread_mutex_unlock(&lock1);
            return "Error: group does not exist\n";
        }
        if(groupOwner[gid]!=currentUser){
            pthread_mutex_unlock(&lock1);
            return "Error: only owner can accept requests\n";
        }
        if(joinRequests[gid].count(uname)==0){
            pthread_mutex_unlock(&lock1);
            return "Error: no such request\n";
        }
        joinRequests[gid].erase(uname);
        groupMembers[gid].insert(uname);
        pthread_mutex_unlock(&lock1);
        return "User "+uname+" added to group "+gid+"\n";
    }else if(cmd[0]=="logout" && cmd.size()==1){
        if(currentUser=="") return "Error: no user logged in\n";
        pthread_mutex_lock(&lock1);
        logged_curr.erase(currentUser);
        string uname=currentUser;
        currentUser="";
        pthread_mutex_unlock(&lock1);
        return "User "+uname+" logged out successfully\n";
    }else{
        return "Unknown or invalid command\n";
    }
}

void *clientHandler(void *arg){

    int new_sock = *((int*)arg);
    delete (int*)arg;
    string currentUser = "";
    char buffer[1024] = {0};
    while (true){
        memset(buffer, 0, sizeof(buffer));
        int val = read(new_sock, buffer, sizeof(buffer));
        if(val <= 0){
            break;
        }

        string input(buffer);
        auto tokens = split(input);
        string resp = handleCommand(tokens,currentUser);

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