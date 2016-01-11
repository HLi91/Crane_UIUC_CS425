#include "UdpWrapper.h"

/*code from BJ's guide*/
void * udpWrapper::get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


/*udpSend
send msg to the destination. the msg size should be limited to 1 udp packet
msg is encode with http-like header
@param	msg		input	string	data to be sent
@param	addr	input	string	destination ip address
@param	port	input	string	destination port num*/
int udpWrapper::udpSend(string msg, string addr, string port)
{

	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
		
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(addr.c_str(), port.c_str(), &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		cout << addr << " " << endl;
		return 1;
	}

	// loop through all the results and make a socket
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}
	string encodeMsg = udpEncode(msg);


		

	if ((numbytes = sendto(sockfd, encodeMsg.c_str(), encodeMsg.length(), 0,
		p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		cout << "fd is " << sockfd << endl;
		cout << addr << ":" << port << endl;

		//exit(1);
	}

	freeaddrinfo(servinfo);
	listenfd = sockfd;
		
	close(sockfd);
	return 0;
}


/*udpEncode
encode the msg with http-like header. including content-length
@param	msg		input	string	data to be encode
@return	string			encoded msg
*/

string udpWrapper::udpEncode(string msg)
{
	const char* header = "Content-Length: ";
	std::string out = header + std::to_string(msg.length()) + "\r\n\r\n"+msg;
	return out;
	
}


/*udpDecode
decode the msg, rip the header
@param	raw		input	received raw data
@return	string			content msg
*/
string udpWrapper::udpDecode(string raw)
{
	size_t pos = raw.find("Content-Length: ");
	pos += 16;
	size_t end = raw.find("\r\n", pos);
	string contentLength = raw.substr(pos, end);
	string content = raw.substr(raw.find("\r\n\r\n")+4);
	return content;

}



/*udpRecSetup
set up listener
@param	port	input	string	local port no.
@return	int		nagetive: fail	positive:socket id
*/

int udpWrapper::udpRecSetup(const char* port)
{
	
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	
		
		
		

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

		// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return -2;
	}

	freeaddrinfo(servinfo);

	printf("listener: waiting to recvfrom...\n");
	listenfd = sockfd;
	return sockfd;
	
}

/*udpRecAsyn
handle the incomming udp packet
call the callback function when received a complete packet
@param	callback	callback function
@param	typeInfo	int		user defined res type used in the callback function
@param	res			void*	user defined res used in the callback function
*/
int udpWrapper::udpRecAsyn(function<int(udpInfo&)> callback, int typeInfo, void * res)
{
	udpInfo callbackInfo;
	callbackInfo.callback = callback;
	callbackInfo.fd = listenfd;
	callbackInfo.parent = this;
	callbackInfo.res = res;
	callbackInfo.varTypeInfo = typeInfo;
	worker = new std::thread(&udpWrapper::beginRec, this, callbackInfo);
	return 0;


}

/*beginRec
helper function for udpRecAsyn
*/
int udpWrapper::beginRec(udpInfo info)
{
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	int numbytes;
	struct sockaddr_storage their_addr;
	char s[INET6_ADDRSTRLEN];

	addr_len = sizeof their_addr;
	while (1)
	{
		if ((numbytes = recvfrom(listenfd, buf, MAXBUFLEN - 1, 0,
			(struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			return -1;
		}

		if(DEBUG) printf("listener: got packet from %s\n", inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),s, sizeof s));

		string raw;
		raw.append(buf, numbytes);
		
		info.msg.clear();
		info.msg = udpDecode(raw);
		//cout << "raw msg size " << raw.size() << endl;
		//cout << raw << endl;
		//cout << "decoded msg size " << info.msg.size() << endl;
		//cout << info.msg << endl;
		info.callback(info);

		quitFlag.mut.lock();
		if (quitFlag.quit)
		{
			quitFlag.mut.unlock();
			if (DEBUG)printf("thread quit\n");
			return 0;
		
		}
		quitFlag.mut.unlock();

	}
}

int udpWrapper::quit()
{
	quitFlag.mut.lock();
	quitFlag.quit = true;
	quitFlag.mut.unlock();
	close(listenfd);

}