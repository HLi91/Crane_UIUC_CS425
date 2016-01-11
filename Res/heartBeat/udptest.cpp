#include "UdpWrapper.h"
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

int callback(udpInfo &info)
{
	cout << "Received: " << info.msg << endl;

}

int main(int argc, char *argv[])
{
	udpWrapper* listener = new udpWrapper();
	listener->udpRecSetup(argv[1]);
	using namespace std::placeholders;
	listener->udpRecAsyn(callback, 0, NULL);
	udpWrapper* sender = new udpWrapper();

	while (1)
	{
		cout << ">>";
		string ip, port, msg;
		cin >> ip;
		cin >> port;
		cin >> msg;
		sender->udpSend(msg, ip, port);

	
	}


}