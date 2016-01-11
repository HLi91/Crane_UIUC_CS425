#include "server.h"

void server::error_404(tcpInfo &info)
{
	info.parent->tcpSend(info, 0, 0, 404);

}


char * server::getPos(const char * input)
{
	char * inputcp = new char[strlen(input) + 1];
	strcpy(inputcp, input);

	char * end = strstr(inputcp, " HTTP");
	char * start = strstr(inputcp, "GET");
	if (end == NULL || start == NULL || start > end)
		return NULL;
	start += 5;
	int size = end - start;
	char * ret = new char[size + 1];
	memcpy(ret, start, size);
	ret[size] = 0;
	return ret;

}

int server::callback(tcpInfo &info)
{

	if (info.type != REC)
		return 0;
	mutex * wait = (mutex *)info.res;

	//cout << "server receive msg" << endl;
	//cout << info.msg << endl;
	char * sendback;
	char * filePath = getPos(info.msg.c_str());
	//cout << filePath << endl;
	int sendbacksize;


	if (filePath == NULL)
	{
		error_404(info);

		return 0;
	}
	else
	{
		streampos size;
		char * memblock;

		ifstream file(filePath, ios::in | ios::binary | ios::ate);
		if (file.is_open())
		{
			size = file.tellg();
			memblock = new char[size];
			file.seekg(0, ios::beg);
			file.read(memblock, size);
			file.close();

			//cout << "the entire file content is in memory";
			sendback = memblock;
			sendbacksize = size;
		}
		else
		{
			cout << "file not available" << endl;
			error_404(info);

			return 0;
		}

	}

	info.parent->tcpSend(info, sendback, sendbacksize);


}


int server::start(string port)
{
	

	listener = new tcpwrapper();
	char *a = new char[port.size() + 1];
	a[port.size()] = 0;
	memcpy(a, port.c_str(), port.size());
	listener->tcpListen(a);
	mutex wait;
	//mut_quit.lock();
	using namespace std::placeholders;

	listener->tcpAcceptRecAsync(bind(&server::callback, this, _1), 0, (void*)&wait);


	//mut_quit.lock();
	//mut_quit.unlock();
	return 0;


}




void server::quit()
{
	//mut_quit.unlock();
	listener->tcpClose();
	
}