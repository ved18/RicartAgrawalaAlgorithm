#include<bits/stdc++.h>
using namespace std;
#define MAX 256
#define FS1 "./FS1/"
#define FS2 "./FS2/"
#define FS3 "./FS3/"

char* getDir(char* serverNo)
{
    char* dir = new char[MAX];

    if(!strcmp(serverNo, "1"))
        memcpy(dir, FS1, sizeof(FS1));
    if(!strcmp(serverNo, "2"))
        memcpy(dir, FS2, sizeof(FS2));
    if(!strcmp(serverNo, "3"))
        memcpy(dir, FS3, sizeof(FS3));

    return dir;
}

bool appendFile(char fileName[], char timeStamp[], char* clientId, char* serverNo)
{
    

    char* dir = getDir(serverNo);
    strcat(dir, fileName);
    cout<<"\nAppending to file: "<<dir<<endl;

    this_thread::sleep_for(chrono::milliseconds(10000));
    ofstream f(dir, ios::out | ios::app);
    
    if(f.is_open())
    {
        f<<"Client ID: "<<clientId<<"\nWrite Time: "<<timeStamp<<endl;
    }
    else
    {
        cout<<"\nCould not open the file"<<endl;
        return false;
    }

    f.close();
    return true;
}