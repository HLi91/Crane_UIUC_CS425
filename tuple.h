/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Hongchuan Li
*students who are taking cs425 in UIUC should not copying this file for their project
*/

#ifndef _TUPLE_H_
#define _TUPLE_H_

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <mutex>
#include <stdint.h>
#include <iostream>
#include <stdio.h>
#include <cstring>
using namespace std;
#define TUPLEMAXSIZE 1024

class myTuple
{

	typedef struct __attribute__((packed)) compactTuple
	{
		uint32_t jobID;
		uint32_t id;
		uint8_t spoutIP[4];
		uint32_t remainSize;
		uint16_t spoutPort;
		uint8_t	flag;
		uint8_t remaining[TUPLEMAXSIZE];

	}compactTuple_t;

	map<string, string> tupleData;
	string SpoutIP;
	string SpoutPort;
	int id;
	int jobID;
	int ack;
	string editorIP, editorPort;		//maybe useful for debug?
	string identifier;

public:
	myTuple();
	//myTuple(vector<string> _tuple);
	myTuple(string msg);
	myTuple(string msg, int val);
	myTuple(int _jobID, string _spoutIP, string _spoutPort);
	string toString();
	string toStringNetwork();
	string& operator[](string field);
	int getJobID();
	void setJobID(int id);
	int getTupleID();
	bool checkAck();
	void setAck();
	void fromNet(string msg, int val);
	void setDrop();
	bool checkDrop();
	//int operator[](string field) const;

	//should be able to access editor


};

#endif
