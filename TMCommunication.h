/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Hongchuan Li
*students who are taking cs425 in UIUC should not copying this file for their project
*/



#ifndef _TMCOMMUNICATION_H_
#define _TMCOMMUNICATION_H_
#include <functional>

#include "tuple.h"
#include "topology.h"
#include "Res/TCPserver/tcpWrapper.h"
#include "Res/heartBeat/UdpWrapper.h"


#define TMCCMD 0
#define TMCTUPLE 1
#define TMCTOPOLOGY 2
#define TMCTUPLECOMPECT 3

#define TMCDEBUG 0

using namespace myTCP;

class TMCInfo
{
public:
	int type;
	string msg;

};

class TMcommunication
{
	tcpwrapper * listener;
	udpWrapper * udpLister;
	udpWrapper * sender;
	
	vector <std::function<int(TMCInfo)> > callbackList;
	map <string, vector<myTuple>> cache;
public:
	TMcommunication();
	int start(string listenPort);
	int regHandler(std::function<int(TMCInfo)> cb);
	int send(string ip, string port, string msg);
	int sendTuple(string ip, string port, myTuple tuple);
	int sendTopology(string ip, string port, myTopology topo);
	int sendAck(myTuple tuple);

private:
	int receiveHelper(udpInfo & info);

};

#endif
