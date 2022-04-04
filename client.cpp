#include<thread>
#include"connection.cpp"
#include<bits/stdc++.h>

using namespace std;

#define MAX 256
#define ENQUIRY "Enquiry"
#define WRITE "Write"
#define DONE "Done"

char clientId[MAX];
vector<string> ips = {"10.176.69.32", "10.176.69.33", "10.176.69.34"};
void writeHelper(int clientSocket)
{
	char* filename = new char[MAX];
	char* response = new char[MAX];

	cout<<"\nEnter File Name to wrtie:";
	cin>>filename;
	
	time_t now = time(0);
   	// convert now to string form
   	char *timeStamp = new char[MAX];
	timeStamp = ctime(&now);
	if(send(clientSocket, filename, MAX, 0) != -1)
	{
		if(send(clientSocket, timeStamp, MAX, 0) != -1)
		{
			if(send(clientSocket, clientId, MAX, 0) != -1)
			{
				if(read(clientSocket, response, MAX) != -1)
					if(!strcmp(response, DONE))
						cout<<"\nDone writing."<<endl;
			}
		}
	}

	delete filename;
	delete response;
	return;
}

void fileWrite(int clientSocket)
{
	if(send(clientSocket, WRITE, MAX, 0) == -1)
	{
		cout<<"\nError in sending data to server."<<endl;
		close(clientSocket);
		exit(0);
	}
	
	writeHelper(clientSocket);

	return;
}

void getFiles(int clientSocket)
{		
	cout<<"\nFile list:"<<endl;
	char *buffer = new char[MAX];
	if(send(clientSocket, ENQUIRY, MAX, 0) == -1)
	{
		cout<<"\nError reading data from server."<<endl;
		close(clientSocket);
		exit(0);
	}
	while(read(clientSocket, buffer, MAX) != -1)
	{
		if(!strcmp(buffer, DONE))
			break;
		cout<<buffer<<endl;
	}
	delete buffer;
}

void mainFunc(int clientSocket)
{
	int choice;
	while(true)
	{
		cout<<"\nChoose\n1 Get files\n2 Write to a file\n3 Exit"<<endl;
		cin>>choice;
		switch(choice)
		{
			case 1:
			getFiles(clientSocket);
			break;

			case 2:
			fileWrite(clientSocket);
			break;

			case 3:
			send(clientSocket, DONE, MAX, 0);
			break;
		}
		if(choice == 3)
			return;
	}
}

//clientid port ip
int main(int argc, char** argv)
{
	int clientSocket; //file descriptor
	
	memcpy(clientId, argv[1], sizeof(argv));
	
	int temp = rand()%3;
	char* ip = new char[MAX];
	memcpy(ip, ips[temp].data(), ips[temp].length());

	connServer(clientSocket, argv[3], stoi(argv[2]));
	delete ip;

	thread clientThread(mainFunc, clientSocket);
	clientThread.join();
	
	//closing the connection to server
	close(clientSocket);

	return  0;
}
