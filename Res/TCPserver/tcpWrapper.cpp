#include "tcpWrapper.h"
#include <netdb.h>

namespace myTCP
{
	/*init
	*	call this function before everything.
	*	@return	0 when success
	*/
	int tcpwrapper::init()
	{
		sockfd = -1;
		memset(&their_addr, 0, sizeof their_addr);
		if (servinfo != NULL)freeaddrinfo(servinfo);
		p = NULL;
		memset(&hints, 0, sizeof hints);
		new_fd = -1;
		connected = false;
		callbackMap.clear();
		connected = false;
		return 0;


	}


	tcpwrapper::~tcpwrapper()
	{
		tcpClose();
	
	}
	/*get_in_addr
	*	function copyed from BJ's guide
	*/
	void * tcpwrapper::get_in_addr(struct sockaddr *sa)
	{
		if (sa->sa_family == AF_INET) {
			return &(((struct sockaddr_in*)sa)->sin_addr);
		}

		return &(((struct sockaddr_in6*)sa)->sin6_addr);
	}

	/*tcpEncode
	*	do nothing for cs438
	*	@param	input	char*	the data to be encode
	*	@param	output	char*	the output data after encode. delete after using
	*	@param	inputSize	int	the size of input should be less than 4 byte
	*	@param	outputSize	int	the size of output buffer	todo: it seems I made some mistake here
	*/
	void tcpwrapper::tcpEncode(char* input, char* &output, int inputSize, int & outputSize)
	{
		const char* header = "HTTP/1.1 200 OK\r\nContent-Length: ";
		std::string out = header + std::to_string(inputSize) + "\r\n\r\n";
		out.append(input, input+inputSize);
		outputSize = out.length();
		output = new char[outputSize+1];
		memcpy(output, out.c_str(), outputSize);
		output[outputSize] = 0;



	}
	/*tcpDecode
	*	remove the http header
	*	@param	input	char*	the package received from sender
	*	@param	output	char*	the pointer point to the correct position in input
	*	@return	size	int		the size of the total package
	*/
	uint32_t tcpwrapper::tcpDecode(char* input, char * & output)		//shadow copy, never try to free output  return the size of input
	{
		output = NULL;
		char* contentLength = strstr(input, "Content-Length: ");
		if (contentLength == NULL)
			return -1;
		contentLength += 16;
		char* newLine = strstr(contentLength, "\r\n");
		int size = newLine - contentLength;
		output = strstr(input, "\r\n\r\n");
		output += 4;
		std::string contentSize = std::string(contentLength, newLine);
		int outsize = std::stoi(contentSize);
		return outsize;


		


	}

	/*
	this function use new_fd as default. obsolete	
	*/
	int tcpwrapper::receiveHelper(char * & buffer)
	{
		char buf[MAXDATASIZE];
		int numbytes;
		int totalsize = 0;
		int currentPos = 0;
		char* data = NULL;
		do {
			if ((numbytes = recv(new_fd, buf, MAXDATASIZE - 1, 0)) == -1) {
				perror("recv");
				exit(1);
			}
			if (numbytes == 0)
				return SOCKCLOSE;
			if (totalsize == 0)
			{
				totalsize = tcpDecode(buf, data);
				numbytes -= 4;
				buffer = new char[totalsize];
			}
			else
			{
				data = buf;
			}

			memcpy(buffer + currentPos, data, numbytes);
			if (TCPDEBUG)printf("totalsize = %d\n", totalsize);
			if (TCPDEBUG)printf("numbytes = %d\n", numbytes);
			currentPos += numbytes;
			if (TCPDEBUG)printf("totalsize = %d\n", totalsize);
		} while (totalsize != currentPos);
		return 0;
	}

	int tcpwrapper::noContentLength(std::string & buffer, int fd, char*alreadyReceived, int alreadyReceivedSize)
	{
		std::string temp;
		temp.append(alreadyReceived, alreadyReceived + alreadyReceivedSize);
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 50000;
		/*
		if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
			perror("Error");
			return TCPERROR;
		}*/
		while (1)
		{
			if ((alreadyReceivedSize = recv(fd, alreadyReceived, MAXDATASIZE - 1, 0)) == -1) {
				//perror("recv");
				if (errno == ECONNRESET)
				{
					if (TCPDEBUG)printf("sock %d close remotely\n", fd);
					fdLock.lock();
					FD_CLR(fd, &master);
					fdLock.unlock();
					callbackMap.erase(fd);
					return SOCKCLOSE;

				}
				else if (errno == EAGAIN)
					break;
				break;
				//exit(1);
			}
			if (alreadyReceivedSize == 0)
			{

				if (TCPDEBUG)printf("sock %d close remotely\n", fd);
				fdLock.lock();
				FD_CLR(fd, &master);
				fdLock.unlock();
				callbackMap.erase(fd);
				
				return SOCKCLOSE;

			}


			temp.append(alreadyReceived, alreadyReceived + alreadyReceivedSize);
		
		} 
		std::string GET = "GET ";
		std::size_t foundget = temp.find(GET);
		if (foundget  == 0)
		{
			buffer = temp;
			return 0;

		}

		std::string rnrn = "\r\n\r\n";

		std::size_t found = temp.find(rnrn);
		if (found == std::string::npos)
		{
			
			return TCPERROR;
		
		}
		buffer = temp.substr(found + 4);
		
		return 0;
	
	}

	/*	receiveHelper
	*	receive data from a specific socket, create a char buffer to store the data. This function will block the thread, return only after the entire package arrived
	*	@pre	must already called accept or connect to set up
	*	@param	buffer - (out) a char ptr use to store the data received from tcp. Need to call delete() after use
	*	@param	fd - (in) socket file descriptor
	*	@return int, SOCKCLOSE when socket was closed by remote, TCPERROR when fail, 0 success
	*/
	int tcpwrapper::receiveHelper(std::string & buffer, int fd)
	{
		char * buf = new char[MAXDATASIZE];
		int numbytes;
		int totalsize = 0;
		int currentPos = 0;
		char* data = NULL;

		


		do {

			if ((numbytes = recv(fd, buf, MAXDATASIZE - 1, 0)) == -1) {
				//perror("recv");
				if (errno == ECONNRESET)
				{
					if (TCPDEBUG)printf("sock %d close remotely\n", fd);
					fdLock.lock();
					FD_CLR(fd, &master);
					fdLock.unlock();
					callbackMap.erase(fd);
					delete buf;
					return SOCKCLOSE;
				
				}
				delete buf;
				return TCPERROR;
				//exit(1);
			}
			if (numbytes == 0)
			{

				if (TCPDEBUG)printf("sock %d close remotely\n", fd);
				fdLock.lock();
				FD_CLR(fd, &master);
				fdLock.unlock();
				callbackMap.erase(fd);
				delete buf;
				return SOCKCLOSE;

			}
			if (totalsize == 0)
			{

				totalsize = tcpDecode(buf, data);
				if (totalsize == -1)
				{
				
					int ret = noContentLength(buffer, fd, buf, numbytes);
					delete buf;
					return ret;
				
				}
				if (totalsize == 0)
				{
					buffer.append(buf, numbytes);
					delete buf;
					return 0;
				}
				numbytes -= data - buf;
				//buffer = new char[totalsize];
				//if (TCPDEBUG)printf("newbuffer contains %s\n", buffer.c_str());

			}
			else
			{
				data = buf;
			}
			//if (TCPDEBUG)printf("data = %s\n", data);
			buffer.append(data, numbytes);
			//memcpy(buffer + currentPos, data, numbytes);
			//if (TCPDEBUG)printf("now buffer is = %s\n", buffer);
			//if (TCPDEBUG)printf("totalsize = %d\n", totalsize);
			//if (TCPDEBUG)printf("numbytes = %d\n", numbytes);
			currentPos += numbytes;

			//if (TCPDEBUG)printf("currentPos = %d\n", currentPos);

		} while (totalsize != currentPos);
		delete buf;
		if (TCPDEBUG)printf("buffer contains %s\n", buffer.c_str());
		return 0;

	}
	/*tcpConnect
	*	take care of all the setup for connection. only one connection per object
	*	@param address	char*	the server address
	*	@param port		char*	the server port number
	*	@return 0-success	TCPERROR-when something wrong
	*/
	int tcpwrapper::tcpConnect(const char* address, char* port)
	{
		if (connected == true)
		{
			printf("multiple connection is not allowed\n");

		}

		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

		if ((rv = getaddrinfo(address, port, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return TCPERROR;
		}

		// loop through all the results and connect to the first we can
		for (p = servinfo; p != NULL; p = p->ai_next) {
			if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
				perror("client: socket");
				continue;
			}

			if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
				close(sockfd);
				perror("client: connect");
				continue;
			}

			break;
		}

		if (p == NULL) {
			fprintf(stderr, "client: failed to connect\n");
			return TCPERROR;
		}

		/*
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
			perror("Error");
			return TCPERROR;
		}*/



		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
		//printf("client: connecting to %s:%s\n", s, port);

		freeaddrinfo(servinfo); // all done with this structure
		connected = true;

		socketType = SENDER;
		fdLock.lock();
		FD_SET(sockfd, &master);
		fdmax = sockfd > fdmax ? sockfd : fdmax;
		fdLock.unlock();
		connected = true;
		return 0;

	}
	/*tcpSend
	*	send msg. only use this after tcpConnect
	*	@param sendingMsg	char*	the data to be send.
	*	@param msgLength	int		size of the data
	*	@return 0 when success. TCPERROR when something wrong
	*/
	int tcpwrapper::tcpSend(char* sendingMsg, int msgLength)
	{
		if (socketType != SENDER)
			return TCPERROR;
		int outMsgLength;
		char* outMsg;
		tcpEncode(sendingMsg, outMsg, msgLength, outMsgLength);
		if (TCPDEBUG)printf("msg: %s\n", sendingMsg);
		if (TCPDEBUG)std::cout << "outMsg: " << outMsg << std::endl;
		try{
			if (send(sockfd, outMsg, outMsgLength, MSG_NOSIGNAL) == -1)
			{
				perror("send");
				delete outMsg;
				return TCPCLOSE;
			}
			else delete outMsg;

			//std::cout << "finish sending" << std::endl;
		}
		catch (std::exception& e) {
			return -1;
		}
		return 0;

	}


	int tcpwrapper::tcpSend(tcpInfo& info, char* sendingMsg, int msgLength, int errorCode)
	{
		int thisfd = info.fd;
		if (thisfd == -1)
			return TCPERROR;
		
		const std::string error_400 = "HTTP/1.1 400 Bad Request\r\nContent-Length: ";
		const std::string error_404 = "HTTP/1.1 404 Not Found\r\nContent - Length: ";
		std::string out;
		if (errorCode = 400)
		{
			out = error_400 + std::to_string(msgLength) + "\r\n\r\n";

		}
		else if (errorCode = 404)
		{
			out = error_404 + std::to_string(msgLength) + "\r\n\r\n";
		}
		else
			return TCPERROR;

		msgLength = out.length();
		const char * outcstr = out.c_str();

		while (msgLength > 0)
		{
			int retVal;
			if ((retVal = send(thisfd, outcstr, msgLength, MSG_NOSIGNAL)) == -1)
			{
				perror("send");
				return TCPCLOSE;
				
			}
			outcstr += retVal;
			msgLength -= retVal;
			if (TCPDEBUG)printf("remaining msgLength: %d\n", msgLength);


		}
	
	}

	/*tcpSend
	*	send msg. only use this within callback function
	*	@param	info		tcpInfo	contain the fd info
	*	@param sendingMsg	char*	the data to be send.
	*	@param msgLength	int		size of the data
	*	@return 0 when success. TCPERROR when something wrong
	*/
	int tcpwrapper::tcpSend(tcpInfo& info, char* sendingMsg, int msgLength)
	{
		int thisfd = info.fd;
		if (thisfd == -1)
			return TCPERROR;
		int outMsgLength;
		char* outMsg, *outMsgBackup;
		tcpEncode(sendingMsg, outMsg, msgLength, outMsgLength);
		outMsgBackup = outMsg;
		if (TCPDEBUG)printf("msg: %s\n", sendingMsg);
		if (TCPDEBUG)printf("outMsg: %s\n", outMsg);
		//if (send(thisfd, outMsg, outMsgLength, 0) == -1)
		//	perror("send");


		while (outMsgLength > 0)
		{
			int retVal;
			if ((retVal = send(thisfd, outMsg, outMsgLength, MSG_NOSIGNAL)) == -1)
			{
				perror("send");
				return TCPCLOSE;
			}
			outMsg += retVal;
			outMsgLength -= retVal;
			if (TCPDEBUG)printf("remaining msgLength: %d\n", outMsgLength);


		}



		delete outMsgBackup;
		return 0;

	}

	int tcpwrapper::tcpSendAsyn()
	{

		//todo

	}
	/*tcpClose
	*	only the thread that create the tcpwrapper object should be allowed call this function, not any callback function
	*	take care of closeing the fd and thread before delete the tcpwrapper object
	*	tcpwrapper is not designed to be reuse
	*	return 0 when success
	*/
	int tcpwrapper::tcpClose()
	{
		//std::cout << "try to delete tcpwrapper" << std::endl;
		pFlag.mut.lock();
		pFlag.quit = true;
		pFlag.mut.unlock();
		if (TCPDEBUG)printf("total threadsize is %d\n", (int)threadlist.size());
		for (int i = 0; i < threadlist.size(); i++)
		{
			//threadlist[i]->detach();
			//threadlist[i]->~thread();
			threadlist[i]->join();
			delete(threadlist[i]);
			if (TCPDEBUG)printf("delete thread id %d\n", i);
		}
		fdLock.lock();
		if (sockfd != -1)
		{

			for (int i = 0; i <= fdmax; i++)
			{
				if (FD_ISSET(i, &master))
				{
					close(i);				//todo sync
					if (TCPDEBUG)printf("tcpClose:close fd %d\n", i);
				
				}
			
			}
			FD_ZERO(&master);
			callbackMap.clear();
			connected = false;
			close(sockfd);
		
		}
		fdLock.unlock();
		return 0;

	}




	//modified by Junqing Deng for recv timeout
	/*tcpListen
	*	setup the server
	*	@param	port	char*	the port to be listened
	*	@return	0 when success, TCPERROR whe something wrong
	*/

	int tcpwrapper::tcpListen(char* port)
	{
		if (connected == true)
		{
			printf("multiple connection is not allowed\n");
			
		}
		strcpy(backupPort, port);
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE; // use my IP

		if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return TCPERROR;
		}

		for (p = servinfo; p != NULL; p = p->ai_next) {
			if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
				perror("server: socket");
				continue;
			}

			if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
				perror("setsockopt");
				return TCPERROR;
			}

			if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
				close(sockfd);
				perror("server: bind");
				continue;
			}

			break;
		}

		if (p == NULL)  {
			fprintf(stderr, "server: failed to bind\n");
			return TCPERROR;
		}

		freeaddrinfo(servinfo); // all done with this structure

		//setting timeout for the socket recv function by Junqing Deng
		//======================================================================
		/*
		struct timeval tv;
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
			perror("Error");
			return TCPERROR;
		}
		*/
		//======================================================================
		//end of setting timeout

		fdLock.lock();
		FD_ZERO(&master);    // clear the master and temp sets
		FD_ZERO(&read_fds);

		FD_SET(sockfd, &master);
		fdmax = sockfd;
		fdLock.unlock();
		socketType = LISTENER;


		if (listen(sockfd, BACKLOG) == -1) {
			perror("listen");
			return TCPERROR;
		}

		connected = true;

		return 0;

	}

	/*tcpAccept
	*	will accept the connect and receive the data for once	obsolete
	*	@param	recVal	out	std::string		the string of data to received
	*	@return	0 when success
	*	this function is obsoleted
	*/
	int tcpwrapper::tcpAccept(std::string & recVal)
	{

		sin_size = sizeof their_addr;
		memset(&their_addr, 0, sizeof their_addr);

		printf("server:waiting for connection");
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			return -1;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);


		char * buffer;
		receiveHelper(buffer);

		recVal.append(buffer);

		delete buffer;

		printf("client: received '%s'\n", recVal.c_str());

		close(new_fd);
		new_fd = -1;
		return 0;
	}


	/*this function is obsoleted
	*	tcpBeginAcceptHelper
	*	endlessly run the accept function, fork a new process to handle the receive, callback to user when data is ready
	*/
	void tcpwrapper::tcpBeginAcceptHelper(tcpInfo callbackInfo)
	{
		if (TCPDEBUG)printf("enter\n");
		while (1) {  // main accept() loop

			if (TCPDEBUG)printf("tcpBeginAcceptHelper\n");
			sin_size = sizeof their_addr;

			new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
			if (new_fd == -1) {
				perror("accept");
				continue;
			}

			inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *)&their_addr),
				s, sizeof s);
			printf("server: got connection from %s\n", s);



			if (!fork())
			{ // this is the child process
				close(sockfd); // child doesn't need the listener


				char* buffer;
				receiveHelper(buffer);
				callbackInfo.msg.append(buffer);
				delete buffer;
				callbackInfo.fd = new_fd;
				callbackInfo.callback(callbackInfo);

				close(new_fd);
				new_fd = -1;

				exit(0);
			}
			close(new_fd);  // parent doesn't need this

			new_fd = -1;
			callbackInfo.mflag->mut.lock();
			if (callbackInfo.mflag->quit)
			{
				callbackInfo.mflag->mut.unlock();
				if (TCPDEBUG)printf("thread quit\n");
				return;
			}
			callbackInfo.mflag->mut.unlock();
		}


	}
	/*tcpBeginAccept	obsolete
	*	asynchronically accept and receive data. call the callback function when data is ready
	*/
	int tcpwrapper::tcpBeginAccept(AcceptCallback callback, int typeInfo, void * res)
	{

		tcpInfo callbackInfo;
		callbackInfo.mflag = &pFlag;
		callbackInfo.parent = this;
		callbackInfo.callback = callback;
		callbackInfo.res = res;
		callbackInfo.varTypeInfo = typeInfo;
		callbackInfo.type = REC;
		if (TCPDEBUG)printf("before thread\n");
		std::thread* worker = new std::thread(&tcpwrapper::tcpBeginAcceptHelper, this, callbackInfo);
		threadlist.push_back(worker);




	}
	/*tcpLiveRec	obsolete
	*	should only be called by sender type (tcpConnect())
	*	@param	recVal	out	data received
	*	@return	0 when success, TCPERROR when something wrong
	*/
	int tcpwrapper::tcpLiveRec(std::string & recVal)
	{
		
		if (socketType != SENDER)
			return TCPERROR;
		std::string buffer;
		receiveHelper(buffer, sockfd);

		recVal.append(buffer);

		//delete buffer;

		printf("client: received '%s'\n", recVal.c_str());
		return 0;

	}
	/*obsolete*/
	int tcpwrapper::tcpLiveSend(char* sendingMsg, int msgLength)
	{
		



		int outMsgLength;
		char* outMsg;
		tcpEncode(sendingMsg, outMsg, msgLength, outMsgLength);
		if (TCPDEBUG)printf("msg: %s\n", sendingMsg);
		if (TCPDEBUG)printf("outMsg: %s\n", outMsg);
		if (send(new_fd, outMsg, outMsgLength, 0) == -1)
			perror("send");
		delete outMsg;

	}

	/*tcpAcceptRecAsync
	*	async auto handle all the incomming request include ACCEPT and REC for both listener or sender type
	*	call the callback function when the income request has been porcessed.
	*	each object should only call this function once
	*	@param	callback	the callback function ptr. the callback function must check the incomming request type by checking tcpInfo.type
	*	@param	typeInfo	int	the type info for void* res defined by the user for passing information to callback function
	*	@param	res			void*	the user defined information passes to the callback function
	*	@return 0
	*/
	int tcpwrapper::tcpAcceptRecAsync(std::function<int(tcpInfo&)> callback, int typeInfo, void * res)
	{

		tcpInfo callbackInfo;
		callbackInfo.mflag = &pFlag;
		callbackInfo.parent = this;
		callbackInfo.callback2 = callback;
		callbackInfo.res = res;
		callbackInfo.varTypeInfo = typeInfo;
		callbackInfo.type = REC;
		
		if (TCPDEBUG)printf("before thread\n");
		std::thread* worker = new std::thread(&tcpwrapper::tcpBeginAcceptRecHelper, this, callbackInfo);
		threadlist.push_back(worker);
		callbackInfo.isThread = true;
		
		return 0;
	}



	int tcpwrapper::tcpAcceptRecAsyncSingleThread(std::function<int(tcpInfo&)> callback, int typeInfo, void * res)
	{
		tcpInfo callbackInfo;
		callbackInfo.mflag = &pFlag;
		callbackInfo.parent = this;
		callbackInfo.callback2 = callback;
		callbackInfo.res = res;
		callbackInfo.varTypeInfo = typeInfo;
		callbackInfo.type = REC;
		callbackInfo.isThread = false;

		if (TCPDEBUG)printf("before thread\n");
		std::thread* worker = new std::thread(&tcpwrapper::tcpBeginAcceptRecHelper, this, callbackInfo);
		threadlist.push_back(worker);

		return 0;
	}

	/*only for debug purpose*/
	void tcpwrapper::fordebug()
	{
		return;
		for (auto it = callbackMap.begin(); it != callbackMap.end(); ++it)
		{
		
			
			{
			
				printf("msg contains: %s\n", it->second.msg.c_str());
			}
		
		}
	
	}


	void tcpwrapper::callbackHelper(tcpInfo info)
	{
		
		

		
		
		if (info.callback2!=NULL)
			info.callback2(info);
		
		
	
	}
	

	/*tcpBeginAcceptRecHelper
	*	the helper function for tcpAcceptRecAsync
	*/
	void tcpwrapper::tcpBeginAcceptRecHelper(tcpInfo callbackInfo)
	{
		

		
		
		if (TCPDEBUG)printf("tcpBeginAcceptRecHelper:enter\n");
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 200000;

		while (1)
		{
			callbackInfo.mflag->mut.lock();
			//if (TCPDEBUG)printf("check quit flag\n");
			if (callbackInfo.mflag->quit)
			{
				callbackInfo.mflag->mut.unlock();
				if (TCPDEBUG)printf("thread quit\n");
				//FD_ZERO(&master);			//tcpClose() will call this func
										//callback to notify user program the function has terminate
				return;
			}
			callbackInfo.mflag->mut.unlock();
			fdLock.lock();
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			read_fds = master; // copy it
			errorfd = master;
			if (select(fdmax + 1, &read_fds, NULL, &errorfd, NULL) == -1) {
				perror("select");
				std::cout << "fdmax is" << fdmax << std::endl;
				for (int i = 0; i <= fdmax; i++)
				{
					if (FD_ISSET(i, &read_fds))
					{
						std::cout << i << std::endl;
						close(i);
						FD_CLR(i, &master);
						if (i == sockfd)
						{
							std::cout << "listening port is corrupted" << std::endl;
							fdLock.unlock();
							tcpListen(backupPort);
							std::cout << "tcpListener restarted" << std::endl;
							fdLock.lock();

						}
					}

				}
				fdLock.unlock();
				continue;

				
			}
			fdLock.unlock();
			for (int i = 0; i <= fdmax; i++)
			{
				if (FD_ISSET(i, &read_fds))
				{
					if (i == sockfd && socketType == LISTENER)
					{
					
						if (TCPDEBUG)printf("tcpBeginAcceptRecHelper:accept type\n");
						sin_size = sizeof their_addr;

						new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
						if (new_fd == -1) {
							perror("accept");
							continue;
						}
						fdLock.lock();
						FD_SET(new_fd, &master);
						fdLock.unlock();
						callbackMap[new_fd] = callbackInfo;
						
						if(TCPDEBUG)std::cout << "map size " << callbackMap.size() << std::endl;
						fordebug();


						if (new_fd > fdmax) {    // keep track of the max
							fdmax = new_fd;
						}

						//printf("add new fd %d to set\n", new_fd);

						inet_ntop(their_addr.ss_family,
							get_in_addr((struct sockaddr *)&their_addr),
							s, sizeof s);
						//printf("server: got connection from %s\n", s);

						
						
						//printf("info lock\n");

						callbackMap[new_fd].type = ACCEPT;
						callbackMap[new_fd].fd = new_fd;
						//callbackMap[new_fd].callback(callbackMap[new_fd]);
						
						if (callbackMap[i].isThread)
						{
							std::thread* worker = new std::thread(&tcpwrapper::callbackHelper, this, callbackMap[i]);
							threadlist.push_back(worker);
						}
						else
						{
							callbackHelper(callbackMap[i]);
						}

						/*
						std::future<int> fut = std::async(std::launch::async, callbackMap[new_fd].callback, std::ref(callbackMap[new_fd]));
						//if (callbackMap[new_fd].spanms == 0)
						//{
						//	futureList.push_back(std::move(fut));
						//	continue;
						//}
						std::chrono::milliseconds span(callbackMap[new_fd].spanms);
						if (fut.wait_for(span) == std::future_status::timeout)
						{
						
							futureList.push_back(std::move(fut));
							printf("user defined function timeout\n");
						
						}*/
						
					
					}
					else
					{
						

						//callbackInfo.rec_fd = i;
						//std::cout << "tcpfd " << i << std::endl;
						if (TCPDEBUG) printf("receive from socket %d\n", i);
						std::string buffer;
						int recVal;
						callbackMap[i].state = TCPSUCCESS;
						if ((recVal = receiveHelper(buffer, i)) == SOCKCLOSE)
						{
							
							if (TCPDEBUG)printf("connection close\n");
							//callbackMap[i].msg.clear();	
							fdLock.lock();
							close(i);
							FD_CLR(i, &master);
							fdLock.unlock();
							callbackMap[i].state = SOCKCLOSE;
						
						}
						if (recVal == TCPERROR && buffer.length() != 0)
						{
							callbackMap[i].state = TCPERROR;
						
						
						}

						//printf("tcpBeginAcceptRecHelper:data received\n");

						

						if (socketType != LISTENER)
						{
							callbackMap[i] = callbackInfo;
							
						
						}
						
						callbackMap[i].msg = buffer;
						
						callbackMap[i].type = REC;
						callbackMap[i].fd = i;
						//callbackMap[i].callback(callbackMap[i]);
						if (callbackMap[i].isThread)
						{
							std::thread* worker = new std::thread(&tcpwrapper::callbackHelper, this, callbackMap[i]);
							threadlist.push_back(worker);
						}
						else
						{
							callbackHelper(callbackMap[i]);
						}
						//callbackMap[i].callback(callbackMap[i]);
						//std::future<int> fut;
						//futureList.push_back(std::move(fut));
						//futureList[futureList.size()-1] = std::async(std::launch::async, callbackMap[i].callback, std::ref(callbackMap[i]));
						//printf("i am here\n");
						//if (callbackMap[new_fd].spanms == 0)
						//{
						//	futureList.push_back(std::move(fut));
						//	continue;
						//}
						

						/*
						std::chrono::milliseconds span(callbackMap[new_fd].spanms);
						if (fut.wait_for(span) == std::future_status::timeout)
						{

							futureList.push_back(std::move(fut));
							printf("user defined function timeout\n");
						}
						else
						{
							printf("user defined function return\n");
						
						}*/
							

							

					}
				
				
				
				}
			
			
			}
		
		}
	
	
	}

	

}