/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Hongchuan Li
*students who are taking cs425 in UIUC should not copying this file for their project
*/


#include <vector>
#include <map>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "TMCommunication.h"

#include "TMPipe.h"
#include "Res/SDFS/SDFS.h"
#include "tuple.h"
#include "topology.h"
#include "Res/heartBeat/heartBeat.h"
#define SDFSPORT "8898"

#define TMDEBUG 0
#define TIMEOUTCONSTANT 100
#define TIMEOUTUPPERBOUND 1000		//if a could still get the ack back, there is no need to bound the timeout factor
#define TIMEOUTLOWERBOUND 0
#define SLEEPCONST 25
#define SLOWDEBUG 0

using namespace std;
class TMMasterMetadata;
class TMMaster;
class taskManager
{
	//mylog* spoutLog = new mylog("spout.log");
	class RTO_t
	{
		const double alpha = 0.125;
		const double beta = 0.25;
		double estimatedRTT;
		double devRTT;
		mutex rtomut;	
	public:
		RTO_t(int initialVal) :estimatedRTT(initialVal), devRTT(0){};
		RTO_t(const RTO_t & obj)
		{
			estimatedRTT = obj.estimatedRTT;
			devRTT = obj.devRTT;
		}
		void putNewSample(int sendTime, int ackTime);


		int getRTO();		//int should be enough

	};
	mylog spoutLog;
	class tupleMapElem
	{
	public:
		int time;
		myTuple tuple;
		int timeoutCounter;
		tupleMapElem():timeoutCounter(0){};
		tupleMapElem(int _time, myTuple _tuple) : time(_time), tuple(_tuple), timeoutCounter(0) {};
	};
	friend TMMaster;
	TMcommunication * communicationMudule;
	TMMaster *masterModule;
	TMPipe * pipeModule;
	SDFS * FileSysModule;
	map<int, myTopology> topologyMap;
	string masterIP;
	string masterPort;
	

	string myip;
	string myport;
	map<int, map<int, tupleMapElem>> tupleSpoutMap;		//tupleSpoutMap[jobID][tupleID]
	

	mutex TMLock;
	double waitTimeInv = 0;
	int ssThresdhold = 80;
	int dynamicTimeoutBoundry = TIMEOUTCONSTANT;
	RTO_t RTO = RTO_t(TIMEOUTCONSTANT);
	int TCPWindowSize = 1;
	mutex waitOne;
public:
	void join(string ip, string port, string hbPort);
	void processUserInput(string input);
	

private:
	int TMReceiveMsgCallback(TMCInfo info);
	void TMCcmdRecvHelper(string cmd);
	void TMCtupleRecvHelper(myTuple tuple);
	void _TMCtupleRecvHelper(string tuple);
	void TMCtopoRecvHelper(myTopology topo);



	
	void execute(int jobID, string identifier, string fileName, int thread);
	void execute(int jobID, string identifier, string fileName);
	void execute(int jobID, string fileName);

	int pipeRecvCB(pipeInfo info);
	void pipeTupleRecvHelper(myTuple tuple);
	void pipeTopoRecvHelper(myTopology topo, pipeInfo & info);
	void pipeMSGRecvHelper(string msg);
	

	

	void pipeSendTuple(myTuple tuple);
	void TMSendUserInput(string input);


	vector<myTuple> tupleSpoutMapSet(myTuple tuple);
	bool tupleSpoutMapAck(myTuple ack);
	void sendAck(myTuple tuple);

};























class TMMasterMetadata
{
public: class jobInfo
{
public:
	int jobID;
	int thread;
	string identifier;
	jobInfo(int _jobID, int _thread, string _identifier)
	{
		jobID = _jobID;
		thread = _thread;
		identifier = _identifier;
	}
};
public:
	string ip;
	string port;
	vector <jobInfo> jobList;
	TMMasterMetadata(){};
	TMMasterMetadata(string _ip, string _port)
	{
		ip = _ip;
		port = _port;
	}

};

class TMMaster
{
	SDFS* fileSysModule;
	TMcommunication* communicationModule;
	TMPipe* pipeModule;
	heartBeat* heartBeatModule;
	map<string, TMMasterMetadata > processList;
	mutex processListLock;					//also for jobid
	string myip, myport;
	int jobID;
	taskManager * parent;

public:
	TMMaster(SDFS * fileModule, TMcommunication* commuModule, TMPipe * pipeModu, taskManager* tm);

	int hbCallback(hbInfo& info);
	int processUserInput(string input);

	int createTopology(myTopology topo);
	void TMSendTopology(myTopology topo);
	void failRecHelper(hbInfo& info);
	void addRecHelper(hbInfo& info);

private:
	vector<string> getFreeMachine();
	void updateProcessList(string ip, string port, int jobID, int thread, string identifier);

};
