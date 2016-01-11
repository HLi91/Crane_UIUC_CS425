#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include "UdpWrapper.h"
#include <map>
#include <time.h>
#include <chrono>
#include <functional>
#include <sys/types.h>
#include <ifaddrs.h>
#include <climits>


#define ALIVETRUE 1
#define ALIVEFALSE 0
#define ALIVEINTRO 2
#define ALIVEME 5
#define ALIVEINPUTMASK 6

#define ALIVEMASK 0x01

/********************/
/*change this value to adjust the freq(time interval between msg in ms) and the timeout variable in ms*/
#define TIMEOUT 2500
#define FREQ 500
/********************/

#define CBFAIL 0x08
#define CBTIMEOUT 0x08
#define CBDELETE 0x0c
#define CBDELETEMASK 0x08

#define CBADD 0x02
#define CBREPLACE 0x03
#define CBCHANGEMASK 0x02

#define CBMULTICAST 0x11

#define CBOTHER 0x10

#define MSGDROPRATE	0



#define HBDEBUG 0

using namespace std;

class heartBeat;
class hbInfo;


typedef int(*hbCallback)(hbInfo&);
typedef struct hbtable
{
	string ip;
	string port;
	int name;
	int Hcounter;
	long time;
	char alive;
}hbtable_t;

class hbInfo
{
public:
	int msgType;
	string msg;
	string ip;
	string port;
	string name;
	//hbCallback cb;
	vector<std::function<int(hbInfo&)>> cb;
};

typedef struct multicastMsg
{
	int _ttl;
	int _nodeNum;
	string _msg;

	multicastMsg(string msg, int ttl, int nodeNum)
	{
		_ttl = ttl;
		_nodeNum = nodeNum;
		_msg = msg;
	}
}multicastMsg_t;





class heartBeat
{

public:
	int join(string listeningPort, string ip, string port, std::function<int(hbInfo&)> cb);
	int leave();
	map < string, hbtable_t > getTable();
	hbtable_t getMe();
	int broadcast(string msg);
	int multicastNaive(string msg, int ttl, int nodeNum);
	int setHandler(std::function<int(hbInfo&)> cb);		
	

	//int stop();

private:
	map < string, hbtable_t > heartBeatTable;
	int recHBMsg(udpInfo &info);
	std::thread* joinWork;						
	udpWrapper* listener, *sender;				//todo:destructor
	string myport, curIP;
	int myName;
	mutex mut_HBT;

	vector<pair<string, string>> randList;

	hbInfo myInfo;

	mutFlagUDP* qFlag;

	vector<multicastMsg_t> multiMsgList;
	mutex mut_multiMsgList;


private:
	
	string myip();
	long getCurrentTime();
	void sendGossip();
	void updateMe();
	void joinHelper();
	string constructList();
	int randomIP(string &ip, string &port, int & counter);
	int processInfo(udpInfo & info, string & ip, string &port, int & name, int & Hcounter, char & alive);
	int processMulticast(udpInfo & info, string & msg);

	

};


#endif