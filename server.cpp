#include<pthread.h>
#include <thread>
#include<bits/stdc++.h>
#include"connection.cpp"
#include"fileIO.cpp"


using namespace std;

#define MAX 256
#define ENQUIRY "Enquiry"
#define WRITE "Write"
#define DONE "Done"
#define SERVER 1
#define NUMSERVERS 1
#define REQUEST "Request"
#define REPLY "Reply"
#define F1 "TestFile1"
#define F2 "TestFile2"
#define F3 "TestFile3"
#define REPLICATE "Replicate"

//Global variables
static int serverFd[2] = {-1};
static bool crExec[3] = {false};
static map<pair<int,int>, bool> Reply;
static priority_queue<pair<int, pair<int, string>>, vector<pair<int,pair<int, string>>>, greater<pair<int,pair<int, string>>>> deferredQueue;
static char* sNo = new char[MAX];
static int portNo;
static map<int, bool> hasRequested;
static atomic<int> lampClock(0);

void init()
{
    for(int i=0; i<3; i++)
    {
        crExec[i] = false;
        serverFd[i] = -1;
    }
}

void incClock(int compTime)
{
    int temp = lampClock;
    lampClock = max(temp, compTime) + 1;
    cout<<"\nLamports clock value: "<<lampClock<<endl;
}

int getFileNo(char fileName[])
{
    if(!strcmp(fileName, F1))
        return 1;
    else if(!strcmp(fileName, F2))
        return 2;
    return 3;
}

void sendFileList(int clientSocket)
{
    string temp = "";
    for(int i=0; i<=2; i++)
    {
        char *response = new char[MAX];
        temp = "TestFile";
        temp += to_string(i);
        //cout<<temp<<endl;
        memcpy(response, temp.data(), temp.length());
        send(clientSocket, response, MAX, 0);
        temp = "";
        delete response;
    }
    return;
}

void sendDefReplies()
{
    int size = deferredQueue.size();
    for(int i=0; i<size; i++)
    {
        pair<int, pair<int, string>> p =  deferredQueue.top();
        
        cout<<"\nQ front: "<<p.first<<endl;

        int tempFd = p.second.first;
        string temp = p.second.second;

        char* fileName = new char[MAX];

        strcpy(fileName, temp.c_str());

        int fileNo = getFileNo(fileName);
        deferredQueue.pop();
        if(send(tempFd, REPLY, MAX, 0) != -1)
        {
            if(send(tempFd, fileName, MAX, 0) != -1)
            {
                cout<<"\nSent deferred reply to FD: "<<tempFd<<" for file: "<<fileName<<endl;
                Reply[{tempFd, fileNo}] = false;
            }
        }
    }
}

void sendRequest(int serverNo, char fileName[])
{
    int fileNo = getFileNo(fileName);
    cout<<serverFd[serverNo]<<" "<<fileNo<<endl;
    if(send(serverFd[serverNo], REQUEST, MAX, 0) != -1)
    {
        incClock(lampClock);
        int *temp = new int;
        *temp = lampClock;
        send(serverFd[serverNo], temp, sizeof(temp), 0);
        if(send(serverFd[serverNo], fileName, MAX, 0) != -1)
        {
            cout<<"\nRequest sent to server for entering critical section"<<endl;
        }
    }
}

void replicateContent(int tempFd, char timeStamp[], char fileName[], char* clientId)
{
    if(send(tempFd, REPLICATE, MAX, 0) != -1)
    {
        if(send(tempFd, fileName, MAX, 0) != -1)
        {
            if(send(tempFd, timeStamp, MAX, 0) != -1)
            {
                if(send(tempFd, clientId, MAX, 0) != -1)
                    cout<<"\nSent replication request to server."<<endl;
            }
        }
    }
}

void writeFile(int clientSocket, char fileName[MAX], char timeStamp[MAX], char clientId[MAX])
{

    int fileNo = getFileNo(fileName);

    //send request to servers.
    if(!Reply[{serverFd[0],fileNo}])
    {
        hasRequested[fileNo] = true;
        thread sendRequest1(sendRequest, 0, fileName);
        sendRequest1.join();
    }
    if(!Reply[{serverFd[1],fileNo}])
    {
        hasRequested[fileNo] = true;
        thread sendRequest2(sendRequest, 1, fileName);
        sendRequest2.join();
    }
    //check for read replies.
    

    //did we receive both the replies??
    cout<<"Debug Clientid: "<<clientId<<endl;
    while(true)
    {
        if(Reply[{serverFd[0],fileNo}] and Reply[{serverFd[1],fileNo}])
        {
            hasRequested[fileNo] = false;
            //lets enter critical section
            cout<<"\nEntering critical section."<<endl;
            cout<<"Client: "<<clientId<<endl;
            crExec[fileNo] = true;
            
            appendFile(fileName, timeStamp, clientId, sNo);
            cout<<"\nCool received replies from both the servers.\nWrote to the required file."<<endl;
            thread replicateThread1(replicateContent, serverFd[0], timeStamp, fileName, clientId);
            thread replicateThread2(replicateContent, serverFd[1], timeStamp, fileName, clientId);

            replicateThread1.join();
            replicateThread2.join();

            crExec[fileNo] = false;
            //send replies to requests queue

            sendDefReplies();
            break;
        }
    }

}

void sockListen(int serverSocket)
{
    int clientSocket = -1;
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);

    //listening on the socket
    if (listen(serverSocket, 5) != 0)
    {
        cout<<"\nListen failed";
        exit(0);
    }
       
    
    //accepting requests from another client
    clientSocket = accept(serverSocket, (SA *)&cliaddr, &len);
    if (clientSocket < 0)
    {
        cout<<"\nFailed to connect to client."<<endl;
        exit(0);
    }
    else
    {
        cout<<"\nServer connected to client."<<endl;    
    }

    char *response = new char[MAX];

    while(true)
    {
        char *request = new char[MAX];
        if(read(clientSocket, request, MAX) == -1)
        {
            cout<<"\nError in reading from client"<<endl;
            close(clientSocket);
            exit(0);
        }
        else if(!strcmp(request, DONE))
        {
            close(clientSocket);
            exit(0);
        }
        else if(!strcmp(request, ENQUIRY))      //Enquiry about files
        {
            cout<<"\nSending file list to server."<<endl;
            sendFileList(clientSocket);
            send(clientSocket, DONE, MAX, 0);
        }
        else if(!strcmp(request, WRITE))        //write to file
        {
            char fileName[MAX];
            char timeStamp[MAX];
            char clientId[MAX];
            
            incClock(lampClock);
            
            if(read(clientSocket, fileName, MAX) != -1)
            {
                if(read(clientSocket, timeStamp, MAX) != -1)
                {
                    if(read(clientSocket, clientId, MAX) != -1)
                    {
                        cout<<"\nHere to write on server file: "<<fileName<<endl;
                        thread writeThread(writeFile, clientSocket, fileName, timeStamp, clientId);
                        writeThread.join();
                        if(send(clientSocket, DONE, MAX, 0) == -1)
                            cout<<"\nError in writing to file."<<endl;
                    }
                }
            }    
        }
        else
        {
            exit(0);
        }
        delete request;
    }
}

//listening server sockets for write requests and replies
void sockListenServer(int tempFd)
{
    while(true)
    {
        char* request = new char[MAX];
        char fileName[MAX];
        char* timeStamp = new char[MAX];
        int fileNo;
        char* clientId;
        

        if(read(tempFd, request, MAX) != -1)
        {   
            char compTime[MAX];
            // if(read(tempFd, compTime, MAX) != -1)
            //     incClock(stoi(compTime));

            cout<<"\nRequest type: "<<request<<endl;
            if(!strcmp(request, REPLICATE))
            {
                // replicate to file.
                if(read(tempFd, fileName, MAX) != -1)
                {
                    if(read(tempFd, timeStamp, MAX) != -1)
                    {
                        if(read(tempFd, clientId, MAX) != -1)
                        {
                            cout<<fileName<<" "<<timeStamp<<" "<<clientId<<endl;
                            appendFile(fileName, timeStamp, clientId, sNo);

                        };
                    }
                }    
            }
            else if(!strcmp(request, REQUEST))
            {
                int* temp = new int;
                read(tempFd, temp, sizeof(temp));
                incClock(*temp);
                //write request from another server. Need to reply it back.
                if(read(tempFd, fileName, MAX) != -1)
                    cout<<"\nNeed to write to following file: "<<fileName<<endl;
                else
                    cout<<"\nCould not get filename from requesting server."<<endl;
                fileNo = getFileNo(fileName);
                if(crExec[fileNo] == true)
                {
                    string temp = string(fileName);
                    cout<<"\nWrite request received from another server. Server already in a critical section for given file. Need to wait."<<endl;
                    deferredQueue.push({(int)lampClock, {tempFd, temp}});
                }
                else if(hasRequested[fileNo] == true)
                {
                    string temp = string(fileName);
                    cout<<"\nWrite request received from another server. Current server also needs to enter critical section. Adding to queue based on timestamp."<<endl;
                    deferredQueue.push({(int)lampClock, {tempFd, temp}});
                }
                else
                {
                    
                    if(send(tempFd, REPLY, MAX, 0) != -1)
                    {
                        if(send(tempFd, fileName, MAX, 0) != -1)
                        {
                            cout<<"\nSent reply to server for writing."<<endl;
                            Reply[{tempFd, fileNo}] = false;
                        }
                    }
                    else
                        cout<<"Error sending reply to server."<<endl;
                } 
            }
            else if(!strcmp(request, REPLY))
            {
                if(read(tempFd, fileName, MAX) != -1)
                    cout<<"\nReply received for following file: "<<fileName<<endl;
                else
                    cout<<"\nCould not get reply correctly."<<endl;
                fileNo = getFileNo(fileName);
                // reply from server to write
                cout<<"\nReceived reply from server for: "<<tempFd<<" "<<fileNo<<endl;
                Reply[{tempFd, fileNo}] = true;
            }            
            else
            {
                terminate();
                cout<<"\nConnection falied."<<endl;
                exit(0);
            }
        }
        delete request;
        delete timeStamp;
    }
}

//IP1 IP2 serverNo portno
int main(int argc, char** argv)
{

    init();
    sNo = argv[3];
    portNo = stoi(argv[4]);
    int serverSocket; //file descriptors
    cout<<argv[1]<<endl;
    struct sockaddr_in servaddr;
    struct sockaddr_in servaddr1, servaddr2;
    //creating a socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    //clientThreads
    thread clientThread[5];

    if (serverSocket == -1)
    {
        cout<<"\nCouldnt create socket";
        exit(0);
    }
    else
    {
        cout<<"\nSocket creation successful";
    }

    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(portNo);

    //binding server to socket
    if ((bind(serverSocket, (SA *)&servaddr, sizeof(servaddr)) != 0))
    {
        cout<<"\nSocket bind failed";
        exit(0);
    }
    else
    {
        cout<<"\nSocket binded"<<endl;
    }
    connServer(serverFd[0], argv[1], portNo);
    connServer(serverFd[1], argv[2], portNo);
    cout<<"DEBUG out of connection"<<endl;
    
    //listening on the socket
    int ret = listen(serverSocket, 5);
    if (ret)
    {
        cout<<"\nListen failed";
        exit(0);
    }
    else
    {
        cout<<"\nListening on socket for server 1...."<<endl;
    }

    socklen_t len = sizeof(servaddr);

    //connect to other servers
    
    //sleep for 10 s
    //connect()

    if(serverFd[0] < 0)
    {
        serverFd[0] = accept(serverSocket, (SA *)&servaddr1, &len);
        if (serverFd[0] < 0)
        {
            cout<<"\nFailed to connect to server 1.";
            exit(0);
        }
        else
        {
            cout<<"\nServer connected to server 1"<<endl;
            cout<<"\nDebug serverFd= "<<serverFd[0]<<endl;
        }
    }
    else
    {
        cout<<"\nConnected to server 1"<<endl;
        cout<<"\nDebug serverFd= "<<serverFd[0]<<endl;

    }

    // //listening on the socket
    ret = listen(serverSocket, 5);
    if(ret)
    {
        cout<<"\nListen failed";
        exit(0);
    }
    else
        cout<<"\nListening on socket for server 2...."<<endl;

    if(serverFd[1] < 0)
    {
        serverFd[1] = accept(serverSocket, (SA *)&servaddr2, &len);
        if (serverFd[1] < 0)
        {
            cout<<"\nFailed to connect to server 2.";
            exit(0);
        }
        else
        {
            cout<<"\nServer connected to server 2"<<endl;
        }
    }
    else
    {
        cout<<"\nServer connected to server 2"<<endl;
    }

    
    thread serverThread1(sockListenServer, serverFd[0]);
    thread serverThread2(sockListenServer, serverFd[1]);
    cout<<"\nListening on socket for client...."<<endl;
    for(int i=0; i<5; i++)
        clientThread[i] = thread(sockListen, serverSocket);
    
    serverThread1.join();
    exit(0);
    serverThread2.join();
    exit(0);

    for(int i=0; i<5; i++)
        clientThread[i].join();

    exit(0);
    //closing the socket connection
}