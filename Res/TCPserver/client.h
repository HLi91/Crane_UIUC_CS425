#ifndef CLIENT_H
#define CLIENT_H

#include "tcpWrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string>
#include <iostream>

#include <fstream>
#include <streambuf>
#include <mutex>

#include <functional>

using namespace std;
using namespace myTCP;


#define MUT 1

class client
{
public:
	int start(string remoteLocation, string outputLocation);
private:
	char* getPort(char* input);
	char* getHostAddr(char* input);
	char* constructCMD(char* input);
	int receiveHelper(tcpInfo & info);
	char * findFilePos(char * input);
	string output;




};


#endif