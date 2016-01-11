#ifndef TCPWRAPPER_H
#define TCPWRAPPER_H

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
#include <time.h>
#include <chrono>
#include <thread> 



#define BACKLOG 10		// how many pending connections queue will hold
#define MAXDATASIZE 1024*1024*10
#define TCPDEBUG 0
#define ACCEPT 1
#define REC	2
#define SENDER 1
#define LISTENER 2
#define SOCKCLOSE -2
#define TCPERROR -3
#define TCPCLOSE -1
#define TCPSUCCESS 0



namespace myTCP
{
	

	class mutFlag
	{
	public:
		std::mutex mut;
		volatile bool quit;
		mutFlag()
		{
		
			quit = false;
		
		}
	};
	class tcpwrapper;
	class tcpInfo;
	typedef int(*AcceptCallback)(tcpInfo&);
	/*************************
	stored data for callback fucntion
	**************************/
	class tcpInfo
	{
	public:
		std::mutex* lock = NULL;
		tcpwrapper * parent;
		std::string msg;
		AcceptCallback callback;
		std::function<int(tcpInfo&)> callback2;
		mutFlag* mflag;					//use to terminate all thread;
		int type;						//ACCEPT or REC
		int fd = -1;					//use for send msg back
		int varTypeInfo;				//use to define the type of void* res
		void * res;						//define by caller
		int rec_fd;						//use only for void tcpRecHelper func
		int spanms;						//callback function timeout factor
		int state;						//the receive state;
		bool isThread;

		~tcpInfo()
		{
			if (lock != NULL)
			{
			
				delete(lock);
			
			}
		
		}

		
	};

	






	 class tcpwrapper
	{

	private:
		int sockfd;									//the main socket for listen
		char backupPort[6];
		struct addrinfo hints, *servinfo, *p;
		struct sockaddr_storage their_addr;			// connector's address information
		socklen_t sin_size;
		struct sigaction sa;
		int yes = 1;
		char s[INET6_ADDRSTRLEN];
		int rv;
		int new_fd;
		bool connected;
		mutFlag pFlag;
		std::vector<std::thread*> threadlist;
		std::vector<std::future <int>> futureList;	//a list use to host useless future


		fd_set master;								// master file descriptor list
		fd_set read_fds;							// temp file descriptor list for select()
		fd_set errorfd;
		int fdmax;									// maximum file descriptor number
		
		int socketType;								//SENDER, LISTENER
		
		std::map < int, tcpInfo > callbackMap;
		char buf[MAXDATASIZE];

		std::mutex fdLock;

		
		


	public:
		int init();
		~tcpwrapper();

	public:
		int tcpConnect(const char* address, char* port);
		int tcpSend(char* sendingMsg, int msgLength);
		int tcpSend(tcpInfo& info, char* sendingMsg, int msgLength);
		int tcpSend(tcpInfo& info, char* sendingMsg, int msgLength, int errorCode);
		int tcpSendAsyn();		//todo:async callback function		

	

	public:
		int tcpListen(char* port);
		int tcpAccept(std::string & recVal);	//the func close the connection when return
		int tcpBeginAccept(AcceptCallback callback, int typeInfo, void * res);			
										//asyc callback fucntion use for large file transport
										//the function only close the connection after the callback function return
		int tcpAcceptRecAsync(std::function<int(tcpInfo&)> callback, int typeInfo, void * res);		//you must check the typeInfo.type to make sure if its a receiveCB or acceptCB
										//automatically decide to accept or receive, easy to exit
									
		int tcpAcceptRecAsyncSingleThread(std::function<int(tcpInfo&)> callback, int typeInfo, void * res);

		
	public:				//use only after the connection established (by the callback function)
		int tcpLiveRec(std::string & recVal);
		int tcpLiveSend(char* sendingMsg, int msgLength);
		int tcpClose();
	
	
	
	private:
		void tcpBeginAcceptHelper(tcpInfo callbackInfo);
		void *get_in_addr(struct sockaddr *sa);
		void tcpEncode(char* input, char* &output, int inputSize, int & outputSize);
		uint32_t tcpDecode(char* input, char * & output);		//shadow copy, never try to free output
		int receiveHelper(char * & buffer);						//remember to free buffer
		int receiveHelper(std::string & buffer, int fd);
		void tcpRecHelper(tcpInfo callbackInfo);
		void tcpBeginAcceptRecHelper(tcpInfo callbackInfo);
		void fordebug();
		void callbackHelper(tcpInfo info);
		int noContentLength(std::string & buffer, int fd, char*alreadyReceived, int alreadyReceivedSize);
	};






}

#endif // !TCPWRAPPER_H