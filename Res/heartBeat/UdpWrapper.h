#ifndef UDPWRAPPER_H
#define UDPWRAPPER_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <map>
#include <future>
#include <chrono>

#define DEBUG 0
#define MAXBUFLEN 64000

using namespace std;

class udpWrapper;
class udpInfo;
typedef int(*AcceptCallback)(udpInfo&);

class mutFlagUDP
{
public:
	std::mutex mut;
	volatile bool quit;
	mutFlagUDP()
	{

		quit = false;

	}
};


class udpInfo
{
public:
	
	udpWrapper * parent;
	std::string msg;
	function<int(udpInfo&)> callback;
	int fd = -1;					//use for send msg back
	int varTypeInfo;				//use to define the type of void* res
	void * res;						//define by caller
	size_t pos;						//help for reading

};

class udpWrapper
{
private:
	int listenfd;
	std::thread* worker;
	mutFlagUDP quitFlag;

public:
	int udpSend(string msg, string addr, string port);
	int udpRecSetup(const char* port);
	int udpRecAsyn(function<int(udpInfo&)> callback, int typeInfo, void * res);
	int quit();
	~udpWrapper()
	{
		quit();	
	}

	
private:
	string udpEncode(string msg);
	string udpDecode(string raw);
	int beginRec(udpInfo info);
	void * get_in_addr(struct sockaddr *sa);

};






#endif