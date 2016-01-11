#include "SDFS.h"


SDFS::SDFS()
{
	hb = new heartBeat();
	fileSever = new server();

}
/*lsMember
list all processes in the SDFS
@return	string	member list
*/
string SDFS::lsMember()
{
	string ret;
	for (auto & it : fileList)
	{
		ret += it.second._ip + ":" + it.second._port + "\n";
	
	}
	return ret;
}
/*lsMyself
list my ip and port info
@return	string	myip and port
*/
string SDFS::lsMyself()
{
	
	return myip() + ":" + myport;
}
/*join
join into the SDFS system by starting the heartbeat and tcpserver
@param	myPort		string	the local port number, both for udpheartbeat and tcpserver
@param	remoteIP	string	introducer's ip
@param	remotePort	string	introducer's port number
*/
int SDFS::join(string myPort, string remoteIP, string remotePort)
{
	if (SDFSDEBUG)printf("enter SDFS::join\n");
	log.write2log(remoteIP + ":" + remotePort + " joining SDFS");
	if (fileList.empty())
	{
		fileList.insert(pair<int, SDFSmetadata_t>(myHash(myip() + myPort), SDFSmetadata_t(myip(), myPort)));
		myport = myPort;
	}
	
	fileSever->start(myPort);
	using namespace std::placeholders;
	
	
	hb->join(myPort, remoteIP, remotePort, bind(&SDFS::callback, this, _1));
	
	if (SDFSDEBUG)printf("leave SDFS::join\n");
	return 0;
}
/*leave
stop the heartbeat and tcpserver
*/
int SDFS::leave()
{
	log.write2log(myip() + ":" + myport + " leave SDFS");
	hb->leave();
	fileSever->quit();

	return 0;
}

string SDFS::storeFolderPath()
{
	return cwd() + "/";
}

/*put
put the local file into SDFS by broadcasting the local file info
@param	localFilename	string	the file name, currently the file should be stored at ./local/ folder
@param	remoteFilename	string	the file name used in SDFS, special char '/' is allowed
@return	0 when success -1 when something wrong
*/
int SDFS::put(string localFilename, string remoteFilename)
{
	if (SDFSDEBUG)printf("enter SDFS::put\n");
	if (!is_file_exist((cwd() + local + localFilename).c_str()))
	{
		cout << "file not exist on local machine" << endl;
		log.write2log("error: file not exist on local machine");
		return -1;
	
	}
	if (lsVM(remoteFilename).size() != 0)
	{
		cout << remoteFilename << " already exist in SDFS" << endl;
		log.write2log("error: " + remoteFilename + " already exist in SDFS");
		return -1;
	
	}

	log.write2log("put " + localFilename + " as " + remoteFilename + " into SDFS" );
	string msg = constructMulticastMsg(MULTICASTPUT, cwd() + local + localFilename, remoteFilename);
	hb->broadcast(msg);
	//cout << msg << endl;

	
	hbInfo fake;
	fake.msgType = CBMULTICAST;
	fake.msg = msg;
	callback(fake);
	


	
	if (SDFSDEBUG)printf("leave SDFS::put\n");
	return 0;
}
/*fetch
get the file from SDFS to local file system
@param	localFilename	string	the local file name include the location, current folder if not specified
@param	remoteFilename	string	the file name used in SDFS
@return	0 when success	-1 when file not find
*/
int SDFS::fetch(string localFilename, string remoteFilename)
{
	for (auto & replica : fileList)
	{
	
		for (int i = 0; i < replica.second.fileList.size(); i++)
		{
			if (replica.second.fileList[i].compare(remoteFilename) == 0)
			{
				client * download = new client();
				int ret = download->start("http://" + replica.second._ip + ":" + replica.second._port + cloud + replica.second.fileUniName[i], localFilename);
				if (ret == 0)
				{
					cout << "success" << endl;
					log.write2log("downloading success: http://" + replica.second._ip + ":" + replica.second._port + "/" + remoteFilename + " as " + localFilename);
					return 0;
				}			
			
			}
		
		
		}
		
		
	
	}
	return -1;
}
/*deleteFile
Broadcast a msg to delete the file, currently the file is not remoted from physical disk but only from the SDFS's list
@param	remoteFilename	string	the file name used in SDFS
*/
int SDFS::deleteFile(string remoteFilename)
{
	string msg = constructMulticastMsg(MULTICASTDELETE, string(), remoteFilename);
	hb->broadcast(msg);
	log.write2log("delete file CMD: " + remoteFilename);
	hbInfo fake;
	fake.msgType = CBMULTICAST;
	fake.msg = msg;
	callback(fake);
	return 0;
}
/*store
show the list of file that stored at the local machine
@return	string	the list of file
*/
string SDFS::store()
{
	string ret;
	//ret += myip() + ":" + myport + "\n";
	for (auto & it : fileList[myHash(myip() + myport)].fileList)
	{
		ret += it + "\n";
	
	}

	return ret;
}
/*lsVM
list the replica info that a specific file is stored at
@param	remoteFilename	string	the file name used in SDFS
@return	string	the list of processes identified by ip and port
*/
string SDFS::lsVM(string remoteFilename)
{
	string msg;
	for (auto & it : fileList)
	{
		for (auto & file : it.second.fileList)
		{
		
			if (file.compare(remoteFilename) == 0)
			{
				msg += "file stored at:" + it.second._ip + ":" + it.second._port + "\n";
				break;
			}
		
		}
	
	
	}
	printfileList();
	return msg;
}




/*callback
the callback function pass to the heartbeat object, use to distribute the messages comming for lower layer
@param	info	hbInfo&		containing info collect from heartbeat layer, include the multicast msg
*/
int SDFS::callback(hbInfo & info)
{
	//if (SDFSDEBUG)printf("enter SDFS::callback\n");


	switch (info.msgType)
	{
	case CBADD:
		if (SDFSDEBUG) cout << info.msg;

		addRecHelper(info);
		break;
	case CBFAIL:
		if (SDFSDEBUG) cout << info.msg;
		failRecHelper(info);
		break;
	case CBMULTICAST:
		if (SDFSDEBUG) cout << info.msg;
		multicastRecHelper(info);
		break;
	default:
		//if (SDFSDEBUG) cout << "ignored" << endl;
		break;
	}
	//if (SDFSDEBUG)printf("leave SDFS::callback\n");
	return 0;

}







/*myHas
hash a string, output an int as hash value
@param	input	string	input to be hashed
@return	int		the hashed output
*/
int SDFS::myHash(string input)
{
	int ret = 7;
	for (int i = 0; i < input.size(); i++)
	{
		 ret = ret*31 + (int)input[i];	
		 //cout << input[i] << endl;
	}
	ret = ret % SDFSHASHMASK;
	return ret;
}

/*myip
Get my ip address
@return	addr	string	my ip address
*/
string SDFS::myip()
{
	
	if (curIP.length() != 0)
		return curIP;
	struct ifaddrs *ifap, *ifa;
	struct sockaddr_in *sa;
	char *addr;

	getifaddrs(&ifap);
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_INET) {
			sa = (struct sockaddr_in *) ifa->ifa_addr;
			addr = inet_ntoa(sa->sin_addr);
			//printf("Interface: %s\tAddress: %s\n", ifa->ifa_name, addr);
		}
	}

	freeifaddrs(ifap);

	curIP = addr;
	
	return addr;

}


/*addRecHelper
called by callback() when a new process joins the system and is detected by heartbeat layer
@param	info	hbInfo&		containing info collect from heartbeat layer, include the multicast msg
*/
int SDFS::addRecHelper(hbInfo & info)
{
	if (SDFSDEBUG)printf("enter SDFS::addRecHelper\n");
	log.write2log("receive callback");
	string msg = constructMulticastMsg(MULTICASTLS, "", "");
	hb->broadcast(msg);
	if (SDFSDEBUG)printf("leave SDFS::addRecHelper\n");
	return 0;
}

/*failRecHelper
called by callback() when a process failsand is detected by heartbeat layer
@param	info	hbInfo&		containing info collect from heartbeat layer, include the multicast msg
*/
int SDFS::failRecHelper(hbInfo & info)
{
	log.write2log("receive callback");
	if (SDFSDEBUG)printf("enter SDFS::failRecHelper\n");
	if (info.ip == myip() && info.port == myport)
		return 0;
	
	
	hbInfo copyinfo(info);
	std::thread* worker2 = new std::thread(&SDFS::failReplicateHelper, this, copyinfo);



	//failReplicateHelper(info);



	
	if (SDFSDEBUG)printf("leave SDFS::failRecHelper\n");
	return 0;
}
/*multicastRecHelper
called by callback() when a multicast msg is detected by heartbeat layer, distributes the multicast msg to proper func
@param	info	hbInfo&		containing info collect from heartbeat layer, include the multicast msg
*/
int SDFS::multicastRecHelper(hbInfo & info)
{
	if (SDFSDEBUG)printf("enter SDFS::multicastRecHelper\n");
	
	size_t startptr = info.msg.find("<SDFSTYPE>", 0);
	
	if (startptr == std::string::npos)
	{
		if (SDFSDEBUG)printf("leave SDFS::multicastRecHelper\n");
		return 0;	
	}
	startptr += sizeof("<SDFSTYPE>") - 1;
	size_t endptr = info.msg.find("</SDFSTYPE>", startptr);
	int type = stoi(info.msg.substr(startptr, endptr - startptr));
	switch (type)
	{
	case MULTICASTDELETE:
		mcdelHelper(info);
		break;
	case MULTICASTLS:
		mclsHelper(info);
		break;

	case MULTICASTPUT:
	{
		hbInfo copyinfo(info);
		std::thread* worker2 = new std::thread(&SDFS::mcputHelper, this, copyinfo);
		threadlist.push_back(worker2);
	}
		//mcputHelper(info);
		break;
	default:
		break;
	}
	if (SDFSDEBUG)printf("leave SDFS::multicastRecHelper\n");
	return 0;
}

/*constructMulticastMsg
construct the multicast msg base on the multicast type, including list of local file, put msg, delete msg
@param	type			int		msg type
@param	localFilename	string	optional, only needed when type is PUT
@param	remoteFilename	string	optional, only needed when type is PUT and DELETE
@return	string	the multicast msg in string type
*/
string SDFS::constructMulticastMsg(int type, string localFilename, string remoteFilename)
{
	if (SDFSDEBUG)printfileList();
	if (SDFSDEBUG)printf("enter SDFS::constructMulticastMsg\n");
	mut_fileList.lock();
	string msg;
	msg += "<SDFSTYPE>" + to_string(type) + "</SDFSTYPE>\r\n";
	msg += "<SDFSIP>" + myip() + "</SDFSIP>\r\n";
	msg += "<SDFSPORT>" + myport + "</SDFSPORT>\r\n";
	msg += "<SDFSVER>" + to_string(++fileList[myHash(myip() + myport)].version) + "</SDFSVER>\r\n";
	if (type == MULTICASTLS)
	{		
		//cout << fileList[myHash(myip() + myport)]._ip << endl;
		//if (fileList[myHash(myip() + myport)].fileList.size() != 0)
			//cout << fileList[myHash(myip() + myport)].fileList[0] << endl;
		for (int i = 0; i < fileList[myHash(myip() + myport)].fileList.size(); i ++)
		{
			msg += "<SDFSNAME>" + fileList[myHash(myip() + myport)].fileList[i] + "</SDFSNAME>\r\n";
			msg += "<SDFSUNINAME>" + fileList[myHash(myip() + myport)].fileUniName[i] + "</SDFSUNINAME>\r\n";

		}
		//cout << msg << endl;
	}
	if (type == MULTICASTPUT)
	{
		msg += "<FILENAME>" + remoteFilename + "</FILENAME>\r\n";
		msg += "<FILELOC>" + localFilename + "</FILELOC>\r\n";
		msg += "<FILEUNINAME>" + myip() + to_string(getCurrentTime())  + "</FILEUNINAME>\r\n";
	}

	if (type == MULTICASTDELETE)
	{
		msg += "<FILENAME>" + remoteFilename + "</FILENAME>\r\n";
		//msg += "<FILELOC>" + localFilename + "</FILELOC>\r\n";

	}

	mut_fileList.unlock();
	if (SDFSDEBUG)printf("leave SDFS::constructMulticastMsg\n");
	return msg;
}

/*failReplicateHelper
thread function to handle the failure and potential data copy by invode fetch function
@param	info	hbInfo&		containing info collect from heartbeat layer, include the multicast msg
*/
int SDFS::failReplicateHelper(hbInfo  info)
{
	int failedID = myHash(info.ip + info.port);
	
	mut_fileList.lock();
	SDFSmetadata_t failedEntry = fileList[failedID];

	
	bool flag = false;
	for (int i = 0; i < failedEntry.fileList.size(); i++)
	{
		int key = fileLocation(failedEntry.fileList[i]);
		//cout << endl << "current machine key is " << key << endl << endl;
		//cout << endl << "i is " << i << endl << endl;
		if (key == 3)
		{
			
			mut_fileList.unlock();
			fetch(cwd() + cloud + failedEntry.fileUniName[i], failedEntry.fileList[i]);
			mut_fileList.lock();
			fileList[myHash(myip() + myport)].fileList.push_back(failedEntry.fileList[i]);
			fileList[myHash(myip() + myport)].fileUniName.push_back(failedEntry.fileUniName[i]);
			if (SDFSDEBUG)printfileList();
			log.write2log("replicate " + failedEntry.fileList[i] + "to current machine");
			flag = true;
		}
		
	}

	/*
	for (auto & it : failedEntry.fileList)
	{
		int key = fileLocation(it);
		cout << endl << "current machine key is" << key << endl << endl;
		if (key == 3)
		{
			fetch(cwd() + cloud + it, it);
			fileList[myHash(myip() + myport)].fileList.push_back(it);
			flag = true;
		}

	}*/

	mut_fileList.unlock();
	if (flag == true)
	{
		string msg = constructMulticastMsg(MULTICASTLS, "", "");
		hb->broadcast(msg);
	
	
	}
	int id = myHash(info.ip + info.port);
	fileList.erase(id);
	printfileList();
	return 0;
}

/*fileLocation
core function to determine whether a file should be have a replica in the current process
@param	filename	string	the file name used in SDFS
@return	int		0-2 indicate the first, second and third replica, 3 indicate 4th replica(when some processes fail),  -1 indicate none of my(current process) bussiness*/
int SDFS::fileLocation(string filename)
{
	int key = myHash(filename);
	for (auto  it = fileList.begin();; it++)
	{
		//cout << endl << endl << endl;
		//cout << "filename is" << filename << endl;
		//cout << "key is " << key << endl;
		//cout << "id is " << it->first << endl;
		if (key <= it->first || it == fileList.end())
		{
			if (it == fileList.end() || key < it->first)
			{
				if (it == fileList.begin())
				{
					it = fileList.end();
				}
				it--;			
			}
			
			int myid = myHash(myip() + myport);
			for (int i = 0; i <= 3; i++, it--)
			{
				
				
				if (it->first == myid)
				{
					return i;
				}

				if (it == fileList.begin())
				{

					it = fileList.end();
					//it--;

				}
			}
			return -1;
		
		}


		if (key > it->first)
		{
			continue;
		}

	
	}

}
/*cwd
@return the current working directory
*/
string SDFS::cwd()
{
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) != NULL)
		//fprintf(stdout, "Current working dir: %s\n", cwd);
		return string(cwd);
	else
		//perror("getcwd() error");
		return string();
	


}

/*mcputHelper
analyse the PUT message, download the file to local disk when necessary
@param	info	hbInfo&		containing info collect from heartbeat layer, include the multicast msg
*/
void SDFS::mcputHelper(hbInfo  info)
{
	if (SDFSDEBUG)printf("enter SDFS::mcputHelper\n");
	//cout << "from " << info.ip << endl;
	size_t startptr = info.msg.find("<SDFSIP>", 0) + sizeof("<SDFSIP>")-1;
	size_t endptr = info.msg.find("</SDFSIP>", startptr);
	string ip = info.msg.substr(startptr, endptr - startptr);


	startptr = info.msg.find("<SDFSPORT>", 0) + sizeof("<SDFSPORT>")-1;
	endptr = info.msg.find("</SDFSPORT>", startptr);
	string port = info.msg.substr(startptr, endptr - startptr);

	startptr = info.msg.find("<SDFSVER>", 0) + sizeof("<SDFSVER>")-1;
	endptr = info.msg.find("</SDFSVER>", startptr);
	int ver = stoi(info.msg.substr(startptr, endptr - startptr));
	
	
	startptr = info.msg.find("<FILENAME>", 0) + sizeof("<FILENAME>")-1;
	endptr = info.msg.find("</FILENAME>", startptr);
	string filename = info.msg.substr(startptr, endptr - startptr);

	startptr = info.msg.find("<FILELOC>", 0) + sizeof("<FILELOC>")-1;
	endptr = info.msg.find("</FILELOC>", startptr);
	string filenameLoc = info.msg.substr(startptr, endptr - startptr);

	startptr = info.msg.find("<FILEUNINAME>", 0) + sizeof("<FILEUNINAME>") - 1;
	endptr = info.msg.find("</FILEUNINAME>", startptr);
	string fileUniName = info.msg.substr(startptr, endptr - startptr);
	//cout << "unique name: " << fileUniName << endl;
	bool flag = false;
	
	
	mut_fileList.lock();
	//cout << "enter lock" << endl;
	if (ver > fileList[myHash(ip + port)].version || 
		(ip == myip() && port == myport && ver == fileList[myHash(ip + port)].version))
	{

		fileList[myHash(ip + port)].version = ver;
		int check = fileLocation(filename);
		if (SDFSDEBUG) cout << "current machine is no." << check << endl;
		if (check >= 0 && check < 3)
		{
			mut_fileList.unlock();
			client* download = new client();
			download->start("http://" + ip + ":" + port + "/" + filenameLoc, cwd() + cloud + fileUniName);
			delete download;
			mut_fileList.lock();
			int mykey = myHash(myip() + myport);
			fileList[myHash(myip() + myport)].fileList.push_back(filename);
			fileList[myHash(myip() + myport)].fileUniName.push_back(fileUniName);
			//printfileList();
			log.write2log("replicate " + filename + "to current machine");
			flag = true;
		}
	}
	mut_fileList.unlock();
	
	
	if (flag)
	{
		string msg = constructMulticastMsg(MULTICASTLS, "", "");
		hb->broadcast(msg);
	
	
	}
	if (SDFSDEBUG)printf("leave SDFS::mcputHelper\n");
	return;
}

/*mcputHelper
analyse the DELETE message, remove the file from the local list but not from disk. multicast the result
@param	info	hbInfo&		containing info collect from heartbeat layer, include the multicast msg
*/
int SDFS::mcdelHelper(hbInfo & info)
{
	if (SDFSDEBUG)printf("enter SDFS::mcdelHelper\n");
	size_t startptr = info.msg.find("<FILENAME>", 0) + sizeof("<FILENAME>")-1;
	size_t endptr = info.msg.find("</FILENAME>", startptr);
	string filename = info.msg.substr(startptr, endptr - startptr);
	bool flag = false;
	mut_fileList.lock();
	for (int i = 0; i < fileList[myHash(myip() + myport)].fileList.size(); i++)
	{
		if (filename.compare(fileList[myHash(myip() + myport)].fileList[i]) == 0)
		{
			fileList[myHash(myip() + myport)].fileList.erase(fileList[myHash(myip() + myport)].fileList.begin()+i);
			fileList[myHash(myip() + myport)].fileUniName.erase(fileList[myHash(myip() + myport)].fileUniName.begin() + i);
			log.write2log("remove " + filename + "from current machine");
			flag = true;
			break;
		
		}
	}
	mut_fileList.unlock();

	if (flag)
	{
		string msg = constructMulticastMsg(MULTICASTLS, "", "");
		hb->broadcast(msg);

	
	}
	if (SDFSDEBUG)printf("leave SDFS::mcdelHelper\n");
	return 0;
}
/*mcputHelper
analyse the LIST message, update the local list for specific processes to the newest version
@param	info	hbInfo&		containing info collect from heartbeat layer, include the multicast msg
*/
int SDFS::mclsHelper(hbInfo & info)
{
	if (SDFSDEBUG)printf("enter SDFS::mclsHelper\n");
	size_t startptr = info.msg.find("<SDFSIP>", 0) + sizeof("<SDFSIP>")-1;
	size_t endptr = info.msg.find("</SDFSIP>", startptr);
	string ip = info.msg.substr(startptr, endptr - startptr);


	startptr = info.msg.find("<SDFSPORT>", 0) + sizeof("<SDFSPORT>")-1;
	endptr = info.msg.find("</SDFSPORT>", startptr);
	string port = info.msg.substr(startptr, endptr - startptr);

	startptr = info.msg.find("<SDFSVER>", 0) + sizeof("<SDFSVER>")-1;
	endptr = info.msg.find("</SDFSVER>", startptr);
	int ver = stoi(info.msg.substr(startptr, endptr - startptr));

	mut_fileList.lock();
	if (fileList.find(myHash(ip + port)) == fileList.end())
	{
		fileList[myHash(ip + port)] = SDFSmetadata_t(ip, port);	
	}
	if (ver > fileList[myHash(ip + port)].version)
	{
		fileList[myHash(ip + port)].version = ver;
		fileList[myHash(ip + port)].fileList.clear();
		fileList[myHash(ip + port)].fileUniName.clear();
		startptr = 0;
		while (true)
		{
			
			startptr = info.msg.find("<SDFSNAME>", startptr);
			
			

			if (startptr != std::string::npos)
			{
				startptr += sizeof("<SDFSNAME>") - 1;
				endptr = info.msg.find("</SDFSNAME>", startptr);
				string file = info.msg.substr(startptr, endptr - startptr);
				fileList[myHash(ip + port)].fileList.push_back(file);

				startptr = info.msg.find("<SDFSUNINAME>", startptr);
				startptr += sizeof("<SDFSUNINAME>") - 1;
				endptr = info.msg.find("</SDFSUNINAME>", startptr);
				string unifile = info.msg.substr(startptr, endptr - startptr);
				fileList[myHash(ip + port)].fileUniName.push_back(unifile);
			}
			else
			{
				break;
			}
			startptr = endptr;
		}
	
	}

	mut_fileList.unlock();

	if (SDFSDEBUG)printf("leave SDFS::mclsHelper\n");
	return 0;
}

/*printfilelist
for debug
*/
void SDFS::printfileList()
{

	for (auto & replica : fileList)
	{
		//cout << replica.second._ip << ":" << replica.second._port << ":" << endl;
		for (auto & file : replica.second.fileList)
		{
			//cout << file << endl;

		}

		for (auto & file : replica.second.fileUniName)
		{
			//cout << file << endl;

		}
	
	
	}

}

/*getCurrentTime
get the current time as int in ms
@return		int		current time*/
long SDFS::getCurrentTime()
{
	


	using namespace std::chrono;
	milliseconds ms = duration_cast< milliseconds >(
		system_clock::now().time_since_epoch()
		);

	

	return ms.count() % INT_MAX;

}

/*is_file_exist
check if a file already exist in disk
@param	filename	const char*		file location
@return	bool	TRUE when exists*/
bool SDFS::is_file_exist(const char *fileName)
{
	std::ifstream infile(fileName);
	return infile.good();
}