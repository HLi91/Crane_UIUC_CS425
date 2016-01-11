#include "client.h"

char* client::getHostAddr(char* input)
{
	char* startPos;
	char* endPos;
	if ((startPos = strstr(input, "http://")) != NULL)
	{
		startPos += 7;

	}
	else
		return NULL;

	if ((endPos = strstr(startPos, ":")) == NULL)
	{
		if ((endPos = strstr(startPos, "/")) == NULL)
			return NULL;
	}
	endPos--;
	int size = (int)(endPos - startPos) + 1;
	char * retVal = new char[size + 1];
	memcpy(retVal, startPos, size);
	*(retVal + size) = '\0';

	return retVal;

}

char * client::findFilePos(char * input)
{
	char* temp;
	char* start;
	if ((temp = strstr(input, "http://")) == NULL)
		return NULL;
	temp += 7;
	if ((start = strstr(temp, "/")) == NULL)
		return NULL;
	return start;
}

char* client::constructCMD(char* input)
{


	string cmd = "GET ";
	cmd.append(findFilePos(input));
	cmd.append(" HTTP/1.1\r\nUser-Agent: Wget/1.12 (linux-gnu)\r\nHost: ");
	cmd.append(getHostAddr(input));
	cmd += ":";
	cmd += getPort(input);
	cmd += "\r\nConnection: Keep-Alive\r\n\r\n";

	char * retVal = new char[cmd.length() + 1];
	strcpy(retVal, cmd.c_str());

	//cout << retVal << endl;
	return retVal;

}
char* client::getPort(char* input)
{
	char* startPos;
	char* endPos;
	char *out = new char[5];
	memcpy(out, "80", 2);
	out[2] = 0;

	if ((startPos = strstr(input, ":")) != NULL)
	{
		startPos += 1;
		if ((startPos = strstr(startPos, ":")) != NULL)
		{
			startPos += 1;

		}
		else
			return out;

	}
	else
		return out;

	//char * retVal = new char[5];
	memcpy(out, startPos, 4);
	*(out + 4) = '\0';

	return out;



}

int client::receiveHelper(tcpInfo & info)
{
	if (info.type != REC)
		return 0;

	if (info.state == TCPCLOSE || info.state == TCPERROR)
	{
		mutex * wait = (mutex *)info.res;
		wait->unlock();
		return 0;
	}

	if (info.state = SOCKCLOSE && info.msg.size() == 0)
	{
		mutex * wait = (mutex *)info.res;
		wait->unlock();
		return 0;	
	}

	//cout << "client receive: ";
	//cout << info.msg.c_str() << endl;

	std::ofstream out(output);
	out << info.msg;
	out.close();
	mutex * wait = (mutex *)info.res;
	wait->unlock();
}

int client::start(string remoteLocation, string outputLocation)
{
	
	mutex wait;
	char *a = new char[remoteLocation.size() + 1];
	a[remoteLocation.size()] = 0;
	memcpy(a, remoteLocation.c_str(), remoteLocation.size());
	output = outputLocation;
	//cout << "output location is:" << output << endl;
	char* cmd = constructCMD(a);
	char* hostAddr = getHostAddr(a);
	char* port = getPort(a);


	//cout << "cmd: " << cmd << endl;
	//cout << "hostAddr: " << hostAddr << endl;
	//cout << "port: " << port << endl;


	tcpwrapper* socket = new tcpwrapper();
	if (socket->tcpConnect(hostAddr, port) != 0)
		return -1;
	socket->tcpSend(cmd, strlen(cmd));
	wait.lock();
	//cout << "start rec" << endl;
	using namespace std::placeholders;
	socket->tcpAcceptRecAsync(bind(&client::receiveHelper, this, _1), MUT, (void*)&wait);

	wait.lock();

	return 0;

}
