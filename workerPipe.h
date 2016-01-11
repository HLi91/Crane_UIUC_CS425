/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Hongchuan Li, modified by Junqing Deng
*students who are taking cs425 in UIUC should not copying this file for their project
*/
#ifndef _WORKERPIPE_H_
#define _WORKERPIPE_H_

#include <string>
#include <vector>
#include <map>
#include <deque>
#include <mutex>
#include <thread>
#include <iostream>
#define PIPETUPLE '0'
#define PIPETOPO '1'
#define PEPEMSG '2'
#include "tuple.h"

using namespace std;

class ioCollector
{
	//map<int, vector<string>> tupleMap;
	deque<myTuple> buffer;
	std::thread* readThread;
	volatile int stopFlag = 0;
	std::mutex readLock;
	std::mutex waitOne;
	int startFlag = 0;


	int jobID;
	string identifier;

	void recvWorker();



public:
	//start a thread to read incomming tuple/msg
	void start();

	//close the read thread
	void close();

	//void getNextTuple(int & tupleID, vector<string> & tuple);

	//void emit(int tupleID);
	
	//void remove(int tupleID);

	//return a tuple if available, else block, fill count with 
	myTuple getNextTuple(int& count);
	
	//send the tuple back, needed to end with ; as a special char
	void emit(myTuple tuple);
	void ack(myTuple tuple);
	void emit(string msg);
	void requestFileFromSDFS(string fileName);
	void putFileIntoSDFS(string fileName);
	void setJobID(int id){ jobID = id; }
	void setIdentifier(string id){ identifier = id; }

};


#endif