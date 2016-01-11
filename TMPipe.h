/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Hongchuan Li
*students who are taking cs425 in UIUC should not copying this file for their project
*/

#ifndef _TMPIPE_H_
#define _TMPIPE_H_
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <thread>
#include <unistd.h>
#include <mutex>
#include <sstream>
#include <cstring>
#include <iostream>
#include <time.h>
#include <chrono>

#include "tuple.h"
using namespace std;
#define PIPETUPLE '0'
#define PIPETOPO '1'
#define PEPEMSG '2'


class pipeInfo
{
public:
	char type = PIPETUPLE; //0 for tuple, 1 for string

    int jobID;
    string identifier;
    string msg = "";

    pipeInfo(int _jobID, string _identifier);
    pipeInfo & operator=(const pipeInfo &rhs);
};

class TMPipe
{
	map<string, vector<int>> pipeMap;
    map<string, int> stop;
    map<string, std::thread*> workerReceList;
    std::mutex stopLock;
    
public:
    TMPipe();
    
	//initiate a new pipe for both parent and child, and insert into pipeMap with identifier append with jobID as key
	vector<int> add(int jobID, string identifier);
    
	//called by the child to map the pipe to stdin/out
	void child(vector<int> pipefd);

    //new process to keep reading from pipe, call callback function with pipeInfo
	void parent(std::function<int(pipeInfo)> cb, int jobID, string identifier);
    
	//remove the pipe from pipemap, stop the corresponding process
	void remove(int jobID, string identifier);
    
	//send a tuple to a specific process, parent to child
	void send(int jobID, string identifier, myTuple tuple);

	//send a msg to a process
	void send(int jobID, string identifier, string msg);


private:
	//thread helper for parent when receiving new results from child worker
	void receiveWorker(pipeInfo info, std::function<int(pipeInfo)> cb);

	//void receive(string identifier, string& msg);

};

#endif
