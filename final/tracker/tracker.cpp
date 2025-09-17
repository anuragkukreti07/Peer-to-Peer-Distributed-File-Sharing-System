#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
using namespace std;

unordered_map<string,string> users;
unordered_set<string> logged_curr;
unordered_map<string,string> groupOwner;
unordered_map<string,unordered_set<string>> groupMembers;
unordered_map<string,unordered_set<string>> joinRequests;
pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;


string my_ip;
int my_port, tracker_no;
string peer_ip;
int peer_port;

vector<string> split(const string &s){
    stringstream ss(s);
    string word;
    vector<string> tokens;
    while (ss >> word){
        tokens.push_back(word);
    }
    return tokens;
}


void syncWithPeer(const string &msg){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        return;
    }

    struct sockaddr_in peer_addr;
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(peer_port);
    inet_pton(AF_INET,peer_ip.c_str(),&peer_addr.sin_addr);

    if(connect(sockfd, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) >= 0){
        send(sockfd,msg.c_str(),msg.size(), 0);
    }

    close(sockfd);
}

string handleCommand(vector<string>& cmd, string& currentUser, bool isSync=false) {
    if(cmd.empty()){
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

        if(!isSync){
            syncWithPeer("SYNC create_user "+uname+" "+pass);
        }
        return "User "+uname+" created successfully\n";
    }else if(cmd[0]=="login" && cmd.size()==3){
        pthread_mutex_lock(&lock1);
        string uname=cmd[1],pass=cmd[2];
        if(users.count(uname)==0){ 
            pthread_mutex_unlock(&lock1); return "Error: user does not exist\n"; 
        }
        if(users[uname]!=pass){ 
            pthread_mutex_unlock(&lock1); return "Error: incorrect password\n"; 
        }
        if(logged_curr.count(uname)){ 
            pthread_mutex_unlock(&lock1); return "Error: already logged in\n"; 
        }
        logged_curr.insert(uname);
        currentUser=uname;
        pthread_mutex_unlock(&lock1);
        return "User "+uname+" logged in successfully\n";
    }else if(cmd[0]=="create_group" && cmd.size()==2){
        if(currentUser==""){
            return "Error: please login first\n";
        }
        pthread_mutex_lock(&lock1);
        string gid=cmd[1];
        if(groupOwner.count(gid)){ 
            pthread_mutex_unlock(&lock1); 
            return "Error: group already exists\n"; 
        }
        groupOwner[gid]=currentUser;
        groupMembers[gid].insert(currentUser);
        pthread_mutex_unlock(&lock1);

        if(!isSync){
            syncWithPeer("SYNC create_group "+gid+" "+currentUser);
        }
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

        if(!isSync){
            syncWithPeer("SYNC join_group "+gid+" "+currentUser);
        }
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

        if(!isSync){
            syncWithPeer("SYNC leave_group "+gid+" "+currentUser);
        }
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
        if(currentUser==""){
            return "Error: please login first\n";
        }
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
        if(currentUser==""){
            return "Error: please login first\n";
        }
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

        if(!isSync){
            syncWithPeer("SYNC accept_request "+gid+" "+uname);
        }
        return "User "+uname+" added to group "+gid+"\n";
    }else if(cmd[0]=="logout" && cmd.size()==1){
        if(currentUser==""){
            return "Error: no user logged in\n";
        }
        pthread_mutex_lock(&lock1);
        logged_curr.erase(currentUser);
        string uname=currentUser;
        currentUser="";
        pthread_mutex_unlock(&lock1);
        return "User "+uname+" logged out successfully\n";
    }else if(cmd[0]=="SYNC"){
        cmd.erase(cmd.begin());
        string dummy="";
        return handleCommand(cmd, dummy, true);
    }

    return "Unknown or invalid command\n";
}



void *clientHandler(void *arg){
    int new_sock = *((int*)arg);
    delete (int*)arg;
    string currentUser="";
    char buffer[1024];
    while(true){
        memset(buffer,0,sizeof(buffer));
        int val=read(new_sock,buffer,sizeof(buffer));
        if(val<=0) break;
        string input(buffer);
        auto tokens=split(input);
        string resp=handleCommand(tokens,currentUser);
        send(new_sock,resp.c_str(),resp.size(),0);
    }
    close(new_sock);
    pthread_exit(nullptr);
}


void *trackerListener(void *arg){
    int server_fd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr; addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_port=htons(my_port);
    bind(server_fd,(struct sockaddr*)&addr,sizeof(addr));
    listen(server_fd,5);

    cout<<"[Tracker "<<tracker_no<<"] listening on "<<my_port<<"\n";

    while(true){
        int *new_sock=new int;
        *new_sock=accept(server_fd,nullptr,nullptr);
        pthread_t tid; pthread_create(&tid,nullptr,clientHandler,new_sock);
        pthread_detach(tid);
    }
    close(server_fd);
    return nullptr;
}






int main(int argc,char*argv[]){
    if(argc!=3){ 
        cerr<<"Usage: ./tracker tracker_info.txt tracker_no\n"; 
        return 1; 
    }
    tracker_no=stoi(argv[2]);

    ifstream fin(argv[1]);
    vector<pair<string,int>> trackers;
    string ip; int port;
    while(fin>>ip>>port) trackers.push_back({ip,port});
    fin.close();

    my_ip=trackers[tracker_no-1].first;
    my_port=trackers[tracker_no-1].second;
    int peer_no=(tracker_no==1?2:1);
    peer_ip=trackers[peer_no-1].first;
    peer_port=trackers[peer_no-1].second;

    pthread_t tid; pthread_create(&tid,nullptr,trackerListener,nullptr);
    pthread_join(tid,nullptr);
    return 0;
}
