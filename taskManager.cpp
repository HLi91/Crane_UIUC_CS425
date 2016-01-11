/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Hongchuan Li
*students who are taking cs425 in UIUC should not copying this file for their project
*/


#include "taskManager.h"

/* join
 * connect to introducer and join the group. if the ip is the local machine's ip address
 * the current process will be the introducer and master
 * @param	ip		string	the introducer's ip address
 * @param	port	string	the introducer's port number
 * @param	hbPort	string	the port using for heartbeat, port+1 will also be reserve for data transimission
 */
void taskManager::join(string ip, string port, string hbPort)
{
	spoutLog.setFile("spout.log");
	if (TMDEBUG) cout << "join enter" << endl;
	
	FileSysModule = new SDFS();
	FileSysModule->join(hbPort, ip, port);
	
	myip = FileSysModule->myip();
	myport = to_string(stoi(hbPort)+1);
	communicationMudule = new TMcommunication();
	using namespace std::placeholders;
	
	communicationMudule->regHandler(bind(&taskManager::TMReceiveMsgCallback, this, _1));
	communicationMudule->start(myport);
	pipeModule = new TMPipe();
	if (ip.compare(FileSysModule->myip()) == 0 && hbPort.compare(port) == 0)	//I am master
	{
		masterModule = new TMMaster(FileSysModule, communicationMudule, pipeModule, this);
	}
	else
	{
		masterModule = NULL;
		masterIP = ip;
		masterPort = to_string(stoi(port) + 1);
	}
	if (TMDEBUG) cout << "join leave" << endl;

}

/* processUserInput
 * take the user input and give it to the corresponding entity
 * @param	input	string		user input, usually the name of an crane executable file
 */
void taskManager::processUserInput(string input)
{
	if (TMDEBUG) cout << "processUserInput enter" << endl;
	FileSysModule->put(input, input);
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	//for sdfs to sync
	if (masterModule != NULL)
	{
		masterModule->processUserInput(input);
	}
	else
	{
		TMSendUserInput(input);
	}
	if (TMDEBUG) cout << "processUserInput leave" << endl;
}

/* _TMCtupleRecvHelper
 * a helper function for TMCtupleRecvHelper
 */
void taskManager::_TMCtupleRecvHelper(string tuple)
{
	myTuple cur;
	cur.fromNet(string(tuple, 1), tuple.size());
	//cout << cur.toString() << endl;
	TMCtupleRecvHelper(cur);
}

/* TMReceiveMsgCallback
 * message manager, takes message from TMCommunication Layer and assigns the message to corresponding function
 * for further processing
 * @param	info	TMCInfo		containing the info type and data, created my TMCommunication module
 */
int taskManager::TMReceiveMsgCallback(TMCInfo info)
{
	if (TMDEBUG) cout << "TMReceiveMsgCallback enter" << endl;
	
	switch (info.type)
	{
	case TMCCMD:
		//cout << "TMCCMD " << info.msg << endl;
		TMCcmdRecvHelper(info.msg);
		break;
	case TMCTUPLE:
		//TMCtupleRecvHelper(myTuple(info.msg, 1));
		_TMCtupleRecvHelper(info.msg);
		break;
	case TMCTOPOLOGY:
		//cout << "TMCTOPOLOGY " << info.msg << endl;
		TMCtopoRecvHelper(myTopology(info.msg));
		break;
	default:
		//cout << "default " <<  info.msg << endl;
		break;
	}
	if (TMDEBUG) cout << "TMReceiveMsgCallback leave" << endl;
	return 0;
}

/* TMCcmdRecvHelper
 * handle the commend message(executable file name)
 * hand it to the master module if necessary
 * @param	cmd		string		the executable file name
 */
void taskManager::TMCcmdRecvHelper(string cmd)
{
	if (TMDEBUG) cout << "TMCcmdRecvHelper enter" << endl;
	stringstream ss(cmd);
	string filenName;
	ss >> filenName;
	if (masterModule != NULL)
	{
		masterModule->processUserInput(filenName);
	}
	else
	{
		cout << "error: received cmd " << cmd << endl;
		return;
	}
	if (TMDEBUG) cout << "TMCcmdRecvHelper leave" << endl;
}

/* TMCtupleRecvHelper
 * handle the tuples received from other node, give them to the pipeModule
 * @param	tuple	myTuple		the tuple instance
 */
void taskManager::TMCtupleRecvHelper(myTuple tuple)
{
	if (TMDEBUG) cout << "TMCtupleRecvHelper enter" << endl;
	//cout << tuple.toString() << endl;
	if (tupleSpoutMapAck(tuple))
	{
		//cout << tuple.toString() << endl;
		if (TMDEBUG) cout << "TMCtupleRecvHelper leave(ack tuple)" << endl;

		static int speedCounter = 0;
		speedCounter++;
		int curTime = FileSysModule->getCurrentTime();
		static int backupTime = curTime-1;
		if (curTime - backupTime > 1000)
		{
			int speed = 1000 * speedCounter / (curTime - backupTime);
			speedCounter = 0;
			backupTime = curTime;
			//printf("speed: %d tuples/s\t\t\r", speed);
			cout << "\r" << "speed: " << speed << "tuples/s\t\t" << flush;
		}

		//cout << curTime - backupTime << endl;

		return;
	}
	int jobID = tuple.getJobID();
	if (topologyMap.find(jobID) == topologyMap.end())
	{
		cout << "error: topology doesnt exist" << endl;
		return;
	}
	string entity = tuple["identifier"];
	vector<TMTopologyInstance*> inst = topologyMap[jobID][entity];
	if (inst.size() != 1)
	{
		cout << "error: physical plan corrupted" << endl;
		return;
	}
	if (inst[0]->ip.compare(FileSysModule->myip()) != 0)
	{
		cout << "error: physical plan corrupted" << endl;
		return;
	}
	//cout << tuple.toString() << endl;
	pipeModule->send(tuple.getJobID(), tuple["identifier"], tuple);

	if (TMDEBUG) cout << "TMCtupleRecvHelper leave" << endl;
}

/* TMCtopoRecvHelper
 * handle the physical plan received from master
 * downloading executing the corresponding files if required
 * @param	topo	myTopology		the physical plan data structure
 */
void taskManager::TMCtopoRecvHelper(myTopology topo)
{
	if (TMDEBUG) cout << "TMCtopoRecvHelper enter" << endl;
	//cout << "TMCtopoRecvHelper enter" << endl;
	TMLock.lock();
	if (topologyMap.find(topo.getjobID()) != topologyMap.end())
	{
		//todo:replace topology
		myTopology backup = topologyMap[topo.getjobID()];
		topologyMap[topo.getjobID()] = topo;
		TMLock.unlock();
		vector<TMTopologyInstance*> runningList = backup[myip + myport];
		vector<TMTopologyInstance*> newList = topo[myip + myport];
		for (auto & newjob : newList)
		{
			bool find = false;
			for (auto & running : runningList)
			{
				if (newjob->identifier.compare(running->identifier) == 0)
				{
					find = true;
					break;
				}

			}
			if (!find)
			{
				//execute(topo.getjobID(), newjob->identifier, topo.fileName);
				execute(topo.getjobID(), newjob->identifier, topo.fileName, newjob->thread);
			}
		}
		
		if (TMDEBUG) cout << "TMCtopoRecvHelper leave" << endl;
		return;
	}
	topologyMap[topo.getjobID()] = topo;
	TMLock.unlock();
	vector<TMTopologyInstance*> insts = topo[myip + myport];
	
	FileSysModule->fetch(topo.fileName, topo.fileName);
	if (insts.size() == 0)
	{
		cout << "idle" << endl;
		if (TMDEBUG) cout << "TMCtopoRecvHelper leave" << endl;
		return;
	}
	
	for (auto it : insts)
	{
		//execute(topo.getjobID(), it->identifier, topo.fileName);
		execute(topo.getjobID(), it->identifier, topo.fileName), it->thread;
	}
	if (TMDEBUG) cout << "TMCtopoRecvHelper leave" << endl;
}

/* execute
 * execute the crane executable as a child process
 * setup the pipe connection from parent to child
 * @param	jobID		int			the job id (assign by master when create the job)
 * @param	identifier	string		the worker id (stored in the executable) jobID+identifier uniquely define a worker
 * @param	fileName	string		the executable name
 * @param	thread		int			the number of thread for the worker
 */
void taskManager::execute(int jobID, string identifier, string fileName, int thread)
{
	if (TMDEBUG) cout << "execute enter" << endl;
	//string filePath = FileSysModule->storeFolderPath() + fileName;
	/*****************************/
	//identifier = "increamentSpout";
	/*****************************/
	vector<int> pipe = pipeModule->add(jobID, identifier);
	pid_t pid;
	pid = fork();
	if (pid == 0)
	{
		//this is the child process

		pipeModule->child(pipe);

		execl((FileSysModule->storeFolderPath() + fileName).c_str(), (FileSysModule->storeFolderPath() + fileName).c_str(),
			to_string(jobID).c_str(), fileName.c_str(), identifier.c_str(), (char*)0);
		perror("execute2");
		cout << "error: this is never going to happen" << endl;
		exit(1);
	}
	using namespace std::placeholders;
	pipeModule->parent(bind(&taskManager::pipeRecvCB, this, _1), jobID, identifier);

	if (TMDEBUG) cout << "execute leave" << endl;

}

/* execute
* execute the crane executable as a child process
* setup the pipe connection from parent to child
* @param	jobID		int			the job id (assign by master when create the job)
* @param	identifier	string		the worker id (stored in the executable) jobID+identifier uniquely define a worker
* @param	fileName	string		the executable name
*/
void taskManager::execute(int jobID, string identifier, string fileName)
{
	if (TMDEBUG) cout << "execute enter" << endl;
	//string filePath = FileSysModule->storeFolderPath() + fileName;
	/*****************************/
	//identifier = "increamentSpout";
	/*****************************/
	vector<int> pipe = pipeModule->add(jobID, identifier);
	pid_t pid;
	pid = fork();
	if (pid == 0)
	{
		//this is the child process
		
		pipeModule->child(pipe);
		
		execl((FileSysModule->storeFolderPath() + fileName).c_str(), (FileSysModule->storeFolderPath() + fileName).c_str(),
			to_string(jobID).c_str(), fileName.c_str() , identifier.c_str(), (char*)0);
		perror("execute2");
		cout << "error: your program crash at start" << endl;
		exit(1);
	}
	using namespace std::placeholders;
	pipeModule->parent(bind(&taskManager::pipeRecvCB, this, _1), jobID, identifier);

	if (TMDEBUG) cout << "execute leave" << endl;

}

/* execute
 * a special execute function use for master to retrieve the topology structure from crane exectuable file
 * @param	jobID		int			the job id (assign by master when create the job)
 * @param	identifier	string		the worker id (stored in the executable) jobID+identifier uniquely define a worker
 */
void taskManager::execute(int jobID, string fileName)
{
	if (TMDEBUG) cout << "execute2 enter" << endl;
	//string filePath = FileSysModule->storeFolderPath() + fileName;
	//cout << "jobID is " << jobID << endl;
	vector<int> pipe = pipeModule->add(jobID, "_TMMaster");



	/***********************/
	//execl((FileSysModule->storeFolderPath() + fileName).c_str(), (FileSysModule->storeFolderPath() + fileName).c_str(), (char*)0);
	/*******for debug********/
	pid_t pid;
	pid = fork();
	if (pid == 0)
	{
		//this is the child process
		string filepath = FileSysModule->storeFolderPath().c_str();
		if (TMDEBUG)cout << "filePath " << filepath << endl;
		if (TMDEBUG)cout << "filename " << fileName << endl;
		pipeModule->child(pipe);
		
		execl((FileSysModule->storeFolderPath() + fileName).c_str(), (FileSysModule->storeFolderPath() + fileName).c_str(),
			to_string(jobID).c_str(), fileName.c_str(), (char*)0);
		perror("execute2");
		cout << "error: your program crash at start" << endl;
		exit(1);
	}
	using namespace std::placeholders;
	pipeModule->parent(bind(&taskManager::pipeRecvCB, this, _1), jobID, "_TMMaster");

	if (TMDEBUG) cout << "execute2 leave" << endl;

}

/* pipeRecvCB
 * message dispatcher, receive messages from pipeModule and dispatch them to different function
 * @param	info	pipeInfo	contain the message type and data
 */
int taskManager::pipeRecvCB(pipeInfo info)
{
	if (TMDEBUG) cout << "pipeRecvCB enter" << endl;
	//cout << info.msg << endl;
	//cout << info.type << endl;
	switch (info.type)
	{
	case PIPETUPLE:
		
		pipeTupleRecvHelper(myTuple(info.msg));
		break;
	case PIPETOPO:
		pipeTopoRecvHelper(myTopology(info.msg), info);
		break;
	case PEPEMSG:
		pipeMSGRecvHelper(info.msg);
		break;
	default:
		break;
	}
	if (TMDEBUG) cout << "pipeRecvCB leave" << endl;
}

/* pipeMSGRecvHelper
 * receive and handle messages from worker, currently, downloading/uploading files from SDFS
 * @param	msg		string		message received from worker
 */
void taskManager::pipeMSGRecvHelper(string msg)
{
	stringstream ss(msg);
	string flag, remain;
	ss >> flag >> remain;
	if (flag.compare("1"))
		FileSysModule->fetch(remain, remain);
	else
		FileSysModule->put(remain, remain);
}

/* pipeTupleRecvHelper
 * receive and handle tuples from workers, dispatch tuples to different node via TMCommunication module
 * for spout, using a tcp selective repeat - like algorithm to manage the speed and tuple drop
 * @param	tupleNew	myTuple		tuple data sturcture
 */
void taskManager::pipeTupleRecvHelper(myTuple tupleNew)
{
	if (TMDEBUG) cout << "pipeTupleRecvHelper enter" << endl;
	//cout << tupleNew.toString() << endl;

	if (tupleNew.checkDrop())
	{
		sendAck(tupleNew);
		return;
	}
	vector<myTuple> tupleList = tupleSpoutMapSet(tupleNew);
	for (auto & tuple : tupleList)
	{
		int jobID = tuple.getJobID();
		string ident = tuple["identifier"];

		vector<TMTopologyInstance*> insts = topologyMap[jobID][ident];
		if (insts.size() != 1)
		{
			cout << "error: topology corrupted" << endl;
			return;
		}
		string nextIdent = insts[0]->getNextChild();
		myTuple copy(tuple.toString());
		if (nextIdent.size() == 0)
		{
			//communicationMudule->sendAck(tuple);
			//cout << tuple["val"] << endl;
			//cout << tupleList.size() << endl;
			//cout << tupleNew.toString() << endl;
			//cout << copy.toString() << endl;
			
			if (TMDEBUG) cout << "tuple reach sink" << endl;
			sendAck(tuple);
			return;
		}
		
		if (tuple["spoutIP"].compare("spoutIP") == 0)
		{
#if SLOWDEBUG>0
			std::this_thread::sleep_for(std::chrono::milliseconds(SLOWDEBUG));
#else
			//std::this_thread::sleep_for(std::chrono::milliseconds(RTO.getRTO() - (int)waitTimeInv));
			TMLock.lock();
			if (tupleSpoutMap.find(tuple.getJobID()) == tupleSpoutMap.end())
			{
				if (TCPWindowSize < tupleSpoutMap[tuple.getJobID()].size())
				{
					waitOne.try_lock();
					TMLock.unlock();
					waitOne.lock();
					TMLock.lock();
					waitOne.unlock();
				}


			}

			if (tupleSpoutMap.find(tuple.getJobID()) == tupleSpoutMap.end() || tupleSpoutMap[tuple.getJobID()].find(tuple.getTupleID()) == tupleSpoutMap[tuple.getJobID()].end())
				tupleSpoutMap[tuple.getJobID()][tuple.getTupleID()] = tupleMapElem(FileSysModule->getCurrentTime(), tuple);
			TMLock.unlock();
			//cout << RTO.getRTO() - (int)waitTimeInv << endl;
#endif
			
			tuple["spoutIP"] = myip;
			tuple["spoutPort"] = myport;

		}

		tuple["identifier"] = nextIdent;
		string nextIp = topologyMap[jobID][nextIdent][0]->ip;
		string nextPort = topologyMap[jobID][nextIdent][0]->port;
		tuple["editorIP"] = myip;
		tuple["editorPort"] = myport;
		if (tuple["spoutIP"].compare("spoutIP") == 0)
		{
			cout << "interesting" << endl;
		}
		//cout << "send " << tuple.toString() << endl;
		if (nextIp.compare(myip) == 0 && nextPort.compare(myport) == 0)
		{
			TMCtupleRecvHelper(tuple);
		}
		else
		{
			communicationMudule->sendTuple(nextIp, nextPort, tuple);
		}
		
		
	}
	if (TMDEBUG) cout << "pipeTupleRecvHelper leave" << endl;
}

/* pipeTopoRecvHelper
 * receive and handle the topology (configuration files)
 * hand it to the master module
 * @param	topo	myTopology		physical plan data structure
 */
void taskManager::pipeTopoRecvHelper(myTopology topo, pipeInfo & info)
{
	if (TMDEBUG) cout << "pipeTopoRecvHelper enter" << endl;
	if (masterModule == NULL)
	{
		cout << "error: pipeTopoRecvHelper mastermodule doesnt exist" << endl;
		return;
	}
	//cout << "jobID is " << info.jobID << endl;
	pipeModule->remove(info.jobID, "_TMMaster");
	int newID = masterModule->createTopology(topo);
	
	if (TMDEBUG) cout << "pipeTupleRecvHelper leave" << endl;
}





/* TMSendUserInput
 * send the user input to master
 * @param	input	string		user input
 */
void taskManager::TMSendUserInput(string input)
{

	communicationMudule->send(masterIP, masterPort, input);
}




/* tupleSpoutMapSet
 * record the tuple sended (only for spout)
 * keep track of whether the tuple has being processed
 * regenerate any lost tuples
 * @param	tuple	myTuple		tuple data structure
 * @return	vector<myTuple>		a vector of myTuple that contains tuple just generate from spout worker and timeout tuple
 *								that need to be sent to other nodes
 */
vector<myTuple> taskManager::tupleSpoutMapSet(myTuple tuple)
{
	

	vector<myTuple> ret;
	ret.push_back(tuple);
	if (tuple["spoutIP"].compare("spoutIP") != 0)
	{
		if (TMDEBUG) cout << "tupleSpoutMapSet leave" << endl;
		return ret;
	}
	
	TMLock.lock();
	int jobID = tuple.getJobID();
	int tupleID = tuple.getTupleID();
	int time = (long)FileSysModule->getCurrentTime();
	
	for (auto & job : tupleSpoutMap)
	{
		for (auto & tupleTimeStamp: job.second)
		{
			if (tupleTimeStamp.second.time < time - RTO.getRTO())
			{
				if (TMDEBUG) cout << "tuple timeout id:" << tupleTimeStamp.second.tuple.getTupleID() << endl;
				//cout << "tuple timeout id:" << tupleTimeStamp.second.tuple.getTupleID() << "rto is " << RTO.getRTO() << endl;
				ret.push_back(tupleTimeStamp.second.tuple);
				tupleTimeStamp.second.time = time + RTO.getRTO()*tupleTimeStamp.second.timeoutCounter++;
				
			}
		}
	}
	//tupleSpoutMap[jobID][tupleID] = tupleMapElem(time, tuple);
	if (ret.size() > 2)
	{
		ssThresdhold = waitTimeInv * 0.9;
		ssThresdhold = ssThresdhold > 50 ? ssThresdhold : 50;
		waitTimeInv = 0;
		//waitTime = SLEEPCONST*ret.size();
		//waitTime = waitTime > RTO.getRTO() ? RTO.getRTO() : waitTime;
	}
	

	if (TMDEBUG) cout << "tupleSpoutMapSet enter" << endl;
	for (auto & job : tupleSpoutMap)
	{
		for (auto & tupleTimeStamp : job.second)
		{

			//if (TMDEBUG) cout << "tuple timeout id:" << tupleTimeStamp.second.tuple.getTupleID() << endl;
			//cout << "tuple timeout id:" << tupleTimeStamp.second.tuple.getTupleID() << endl;
			//ret.push_back(tupleTimeStamp.second.tuple);
			//tupleTimeStamp.second.time += (RTO.getRTO() - waitTimeInv)*ret.size();


		}
	}


	TMLock.unlock();
	
	
	if (TMDEBUG) cout << "tupleSpoutMapSet leave" << endl;
	return ret;
	
}

/* tupleSpoutMapAck
 * receive the ackownledgement if a tuple reach its sink
 * adjust the timeout parameters
 * @param	ack		myTuple		the tuple data structure
 * @return	bool	if true, the tuple is an ack, if false, the tuple is a data tuple
 */
bool taskManager::tupleSpoutMapAck(myTuple ack)
{
	if (TMDEBUG) cout << "tupleSpoutMapAck enter" << endl;
	if (!ack.checkAck())
	{
		if (TMDEBUG) cout << "tupleSpoutMapAck leave" << endl;
		return false;
	}
	TMLock.lock();
	int jobID = ack.getJobID();
	int tupleID = ack.getTupleID();
	//cout << "job id " << jobID << " tuple id " << tupleID << endl;
	if (tupleSpoutMap.find(jobID) != tupleSpoutMap.end() && tupleSpoutMap[jobID].find(tupleID) != tupleSpoutMap[jobID].end())
	{
		RTO.putNewSample(tupleSpoutMap[jobID][tupleID].time, FileSysModule->getCurrentTime());
	}
	tupleSpoutMap[jobID].erase(tupleID);
	TMLock.unlock();
	if (TMDEBUG) cout << "tupleSpoutMapAck leave" << endl;
	stringstream ss;
	if (!ack.checkDrop())
		ss << "tuple " << ack.toString() << "acked back";
	else
		ss << "tuple " << ack.toString() << "dropped";
	spoutLog(ss.str());
	//disable the log to increase the speed
	
	if (waitTimeInv > ssThresdhold)
	{
		waitTimeInv += (RTO.getRTO() - waitTimeInv) / RTO.getRTO();
	}
	else
	{
		waitTimeInv = waitTimeInv++;
	}


	waitOne.unlock();
	return true;
}


/* sendAck
 * send an ack to the tuple's spout
 * @param	tuple	myTuple		tuple data structure
 */
void taskManager::sendAck(myTuple tuple)
{
	if (TMDEBUG) cout << "sendAck enter" << endl;
	
	if (tuple["spoutIP"].compare("spoutIP") == 0)
	{
		cout << "i shouldnt be here" << endl;
		cout << tuple.toString() << endl;
		return;
	}
	tuple.setAck();
	tuple["editorIP"] = myip;
	tuple["editorPort"] = myport;

	if (tuple["spoutIP"].compare(myip) == 0 && tuple["spoutPort"].compare(myport) == 0)
	{
		TMCtupleRecvHelper(tuple);
	}
	else
	{
		communicationMudule->sendTuple(tuple["spoutIP"], tuple["spoutPort"], tuple);
	}


	
	if (TMDEBUG) cout << "sendAck leave" << endl;
}


















/*to avoid circular include, I decide to move the Master module to this file*/






/* constructor
 * only one master is allow, master is manully selected when started
 * @param	fileModule		SDFS*				the pointer to the file system
 * @param	commuModule		TMCommunication*	the tcp/udp communication module pointer
 * @param	pipeModu		TMPipe*				pipe module pointer
 * @param	_parant			taskManager			the taskManager module pointer
 */
TMMaster::TMMaster(SDFS * fileModule, TMcommunication* commuModule, TMPipe* pipeModu, taskManager * _parent)
{
	if (TMDEBUG) cout << "TMMaster enter" << endl;
	fileSysModule = fileModule;
	communicationModule = commuModule;
	pipeModule = pipeModu;
	heartBeatModule = fileSysModule->hb;
	using namespace std::placeholders;

	
	heartBeatModule->setHandler(bind(&TMMaster::hbCallback, this, _1));
	parent = _parent;
	myip = parent->myip;
	myport = parent->myport;
	processList[myip + myport] = TMMasterMetadata(myip, myport);
	jobID = 0;
	
	if (TMDEBUG) cout << "TMMaster leave" << endl;
}


/* hbCallback
 * the callback function registered to the heartbeat module
 * in order to monitor the newly joined node and handle any failure
 * @param	info	hbInfo&		containing the heartbeat info type and data
 */
int TMMaster::hbCallback(hbInfo& info)
{
	
	switch (info.msgType)
	{
	case CBADD:
		addRecHelper(info);
		break;
	case CBFAIL:

		failRecHelper(info);
		break;
	case CBMULTICAST:


		break;
	default:
		//if (SDFSDEBUG) cout << "ignored" << endl;
		break;
	}
	//if (SDFSDEBUG)printf("leave SDFS::callback\n");
	
	return 0;
}

/* processUserInput
 * entrance function for a job, handle user's input
 * @param	input	string		the file name of executuable
 */
int TMMaster::processUserInput(string input)
{
	if (TMDEBUG) cout << "TMMaster::processUserInput enter" << endl;
	int thisjobID;
	processListLock.lock();
	jobID++;
	thisjobID = jobID;
	processListLock.unlock();


	parent->execute(thisjobID, input);
	//parent->execute(jobID, input, input);
	if (TMDEBUG) cout << "TMMaster::processUserInput leave" << endl;
	return 0;


}

/* createTopology
 * create the physical plan (assign job to different machines) base on the info from the executable
 * @param	topo	myTopology		the topology data structure
 * @return	int		id of the new job
 */
int TMMaster::createTopology(myTopology topo)
{
	if (TMDEBUG) cout << "TMMaster::createTopology enter" << endl;
	int thisjobID;
	processListLock.lock();
	jobID++;
	thisjobID = jobID;
	processListLock.unlock();
	topo.jobID = thisjobID;
	for (auto & it : topo.topoData)
	{
		vector<string> ipport = getFreeMachine();
		if (ipport.size() == 0)
		{
			cout << "error createTopology: no suitable machine?" << endl;
		}
		it.second.ip = ipport[0];
		it.second.port = ipport[1];
		updateProcessList(ipport[0], ipport[1], thisjobID, it.second.thread, it.second.identifier);
	}
	if (TMDEBUG) cout << topo.toString() << endl;
	//parent->topologyMap[thisjobID] = topo;
	TMSendTopology(topo);
	if (TMDEBUG) cout << "TMMaster::createTopology leave" << endl;
	return thisjobID;
}

/* TMSendTopology
 * broadcast the physical to all existing machines
 * @param	topo	myTopology		the topology data structure
 */
void TMMaster::TMSendTopology(myTopology topo)
{
	if (TMDEBUG) cout << "TMMaster::TMSendTopology enter" << endl;
	cout << topo.toString() << endl;
	processListLock.lock();
	for (auto & it : processList)
	{

		communicationModule->sendTopology(it.second.ip, it.second.port, topo);
	}


	processListLock.unlock();
	if (TMDEBUG) cout << "TMMaster::TMSendTopology leave" << endl;
}

/* failRecHelper
 * handle the failure, reassign the failed workers to different machines
 * @param	info	hbInfo&		containing the heartbeat info type and data
 */
void TMMaster::failRecHelper(hbInfo& info)
{
	if (TMDEBUG) cout << "TMMaster::failRecHelper enter" << endl;
	processListLock.lock();
	//cout << "I am not blocked by lock 1" << endl;
	string failPort = to_string(stoi(info.port)+1);
	TMMasterMetadata backup = processList[info.ip + failPort];
	auto it = processList.find(info.ip + failPort);
	processList.erase(it);
	processListLock.unlock();
	map<int, myTopology> affectedmap;
	for (auto & jobs : backup.jobList)
	{
		affectedmap[jobs.jobID] = parent->topologyMap[jobs.jobID];
	}
	for (auto & jobs : backup.jobList)
	{
		vector<string> ipport = getFreeMachine();
		affectedmap[jobs.jobID][jobs.identifier][0]->ip = ipport[0];
		affectedmap[jobs.jobID][jobs.identifier][0]->port = ipport[1];
		updateProcessList(ipport[0], ipport[1], jobs.jobID, jobs.thread, jobs.identifier);
	}
	processListLock.lock();
	//cout << "I am not blocked by lock 2" << endl;
	for (auto & machine : processList)
	{
		for (auto & topo : affectedmap)
		{
			communicationModule->sendTopology(machine.second.ip, machine.second.port, topo.second);
		}
	}

	processListLock.unlock();
	//cout << "TMMaster::failRecHelper leave" << endl;
	if (TMDEBUG) cout << "TMMaster::failRecHelper leave" << endl;

}

/* addRecHelper
 * handle the new join event
 * @param	info	hbInfo&		containing the heartbeat info type and data
 */
void TMMaster::addRecHelper(hbInfo& info)
{
	if (TMDEBUG) cout << "TMMaster::addRecHelper enter" << endl;
	processListLock.lock();
	string addport = to_string(stoi(info.port) + 1);
	if (processList.find(info.ip + addport) != processList.end())
	{
		cout << "error: addRecHelper fail, unhandled condition" << endl;
		processListLock.lock();
		return;
	}
	processList[info.ip + addport] = TMMasterMetadata(info.ip, addport);
	processListLock.unlock();

	if (TMDEBUG) cout << "TMMaster::addRecHelper leave" << endl;
}


/* getFreeMachine
 * get a machines that are less likely to be busy
 * @return	vector<string>	the first string is the ip and second is the port	
 */
vector<string> TMMaster::getFreeMachine()
{
	if (TMDEBUG) cout << "TMMaster::getFreeMachine enter" << endl;
	processListLock.lock();
	vector<string> ret;
	if (processList.size() == 0)
	{
		processListLock.unlock();
		return ret;
	}

	int min = processList.begin()->second.jobList.size();
	string ipport = processList.begin()->first;
	for (auto &it : processList)
	{
		if (it.second.jobList.size() < min)
		{
			min = it.second.jobList.size();
			ipport = it.first;
		}
	}

	ret.push_back(processList[ipport].ip);
	ret.push_back(processList[ipport].port);
	processListLock.unlock();
	if (TMDEBUG) cout << "TMMaster::getFreeMachine leave" << endl;
	return ret;
}



void TMMaster::updateProcessList(string ip, string port, int jobID, int thread, string identifier)
{
	processListLock.lock();
	processList[ip + port].jobList.push_back(TMMasterMetadata::jobInfo(jobID, thread, identifier));
	processListLock.unlock();
}















































int main(int argc, char *argv[])
{
	taskManager *tm = new taskManager();
	if (argc!=4)
	{
		cout << "usage: remoteIP remotePort myPort" << endl;
		return 0;
	}
		
	tm->join(argv[1], argv[2], argv[3]);
	string input;
	cout << ">>>";
	cout.flush();
	while (cin >> input)
	{
		tm->processUserInput(input);
		cout << endl << ">>>";
	}
}



















/* putNewSample
 * update the round trip time and deviation
 * @param	sendTime	int		time when the tuple has been sent
 * @param	ackTime		int		time when the ack has been received
 */
void taskManager::RTO_t::putNewSample(int sendTime, int ackTime)
{
	
	int rtt = ackTime - sendTime;
	//cout << rtt << endl;
	rtomut.lock();
	estimatedRTT = (1 - alpha)*estimatedRTT + (double)rtt* alpha;
	//the order makes a difference here but Im not sure which one is better
	devRTT = (1 - beta)*devRTT + beta*abs((int)(rtt - estimatedRTT));
	rtomut.unlock();
	//cout << "rtt is set to " << estimatedRTT << "\n";
	//cout << "dev is set to " << devRTT << endl;
}

/* getRTO
 * get the estimate timeout period
 * @return	int	rto
 */
int taskManager::RTO_t::getRTO()
{
	rtomut.lock();
	int ret = (int)(estimatedRTT + 4 * devRTT);
	rtomut.unlock();
	//cout << "getRTO " << ret << endl;
	ret = ret < 300 ? ret : 300;
	return ret > 100 ? ret: 100;
}