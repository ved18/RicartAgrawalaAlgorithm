#include<unistd.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<netdb.h>
#include<bits/stdc++.h>
using namespace std;
#define PORT 8080
#define SA struct sockaddr

void connServer(int& serverFd, const char* ip, int port)
{
	struct sockaddr_in servaddr;
	cout<<ip<<" "<<port<<endl;
	//creating socket
	serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if(serverFd == -1)
	{
		cout<<"\nfailed to connect to socket";
		exit(0);
	}

	bzero(&servaddr, sizeof(servaddr));
	
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = inet_addr(ip);
    //inet_pton(AF_INET, ip, &(servaddr.sin_addr));
    //establishing connection with the server
	if(connect(serverFd, (const SA*)&servaddr, sizeof(servaddr)) != 0)
	{
		serverFd = -1;
		cout<<"\nCould not connect to server: "<<ip <<"\nCheck if server is running."<<endl;
	}
	else
	{
		cout<<"\nConnected to the server successfull."<<endl;
		
	}
    return;
}