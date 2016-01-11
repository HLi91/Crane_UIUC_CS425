#ifndef SDFS_H
#define SDFS_H


#include "../heartBeat/heartBeat.h"
#include "../TCPserver/client.h"
#include "../TCPserver/server.h"
#include <map>

#include <string>
#include <time.h>
#include <chrono>
#include <functional>
#include <sys/types.h>
#include <ifaddrs.h>

#include <unistd.h>
#include <stdio.h>



#define MULTICASTPUT 0
#define MULTICASTDELETE 1
#define MULTICASTLS 2

#define SDFSHASHMASK 1024;

#define SDFSDEBUG 0



using namespace std;












/*mylog class
logging the information into file, store the caller information*/
class mylog
{
public:
	string filename;
	mylog()
	{
		remove("SDFS.log");
		filename = "SDFS.h";
		
	}

	void setFile(string _filename)
	{
		remove(_filename.c_str());
		filename = _filename;
	}

	int operator()(string info)
	{
		
		std::ofstream out(filename, std::ofstream::out | std::ofstream::app);
		out << msg << info << endl;
		out.close();
		mut_log.unlock();
	}

	mylog& write2log(std::string Caller, std::string File, int Line)
	{
		mut_log.lock();
		msg = "log from " + Caller + "() in " + File + ":" + to_string(Line) + "\n";
		return *this;
	}

private:
	string location_;
	string msg;
	mutex mut_log;
};

#define write2log write2log(__FUNCTION__, __FILE__, __LINE__)



























typedef struct SDFSmetadata
{
	string _ip;
	string _port;
	int version;
	vector <string> fileList;
	vector <string> fileUniName;
	SDFSmetadata(string ip, string port)
	{
		_ip = ip;
		_port = port;
		version = -1;
	}

	SDFSmetadata()
	{
		_ip = string();
		_port = string();
		version = -1;
	}
}SDFSmetadata_t;

class SDFS
{

public:
	SDFS();
	string lsMember();
	string lsMyself();
	int join(string myPort, string remoteIP, string remotePort);
	int leave();

	int put(string localFilename, string remoteFilename);
	int fetch(string localFilename, string remoteFilename);
	int deleteFile(string remoteFilename);
	string store();
	string lsVM(string remoteFilename);
	string myip();
	string getmyport()
	{
		return myport;
	}
	string storeFolderPath();
	long getCurrentTime();
	heartBeat * hb;

private:
	server * fileSever;
	
	map<int, SDFSmetadata_t> fileList;
	mutex mut_fileList;
	string curIP;
	string myport;
	string cloud = "/cloud/";
	string local = "/local/";

	mylog log;
	
	
	std::vector<std::thread*> threadlist;

private:
	int myHash(string input);
	int callback(hbInfo & info);
	
	string constructMulticastMsg(int type, string localFilename, string remoteFilename);

	int addRecHelper(hbInfo & info);
	int failRecHelper(hbInfo & info);
	int multicastRecHelper(hbInfo & info);

	void mcputHelper(hbInfo  info);
	int mcdelHelper(hbInfo & info);
	int mclsHelper(hbInfo & info);

	int failReplicateHelper(hbInfo  info);
	int fileLocation(string filename);

	void printfileList();

	string cwd();
	
	bool is_file_exist(const char *fileName);
};

#endif