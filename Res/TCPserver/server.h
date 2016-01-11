#ifndef SERVER_H
#define SERVER_H

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

using namespace std;
using namespace myTCP;

class server
{
public:
	int start(string port);
	int startThread(string port);
	void quit();
private:
	void error_404(tcpInfo &info);
	char * getPos(const char * input);
	int callback(tcpInfo &info);

	mutex mut_quit;


	tcpwrapper* listener;




};


#endif