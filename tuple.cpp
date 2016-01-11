/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Hongchuan Li
*students who are taking cs425 in UIUC should not copying this file for their project
*/

#include "tuple.h"


myTuple::myTuple()
{

	//myTuple(-1, "spoutIP", "spoutPort");
	jobID = -1;
	ack = 0;
	static mutex staticTupleIDLock;
	staticTupleIDLock.lock();
	static int staticTupleID = 1;
	id = staticTupleID++;
	staticTupleIDLock.unlock();
	SpoutIP = "spoutIP";
	SpoutPort = "spoutPort";
	editorIP = SpoutIP;
	editorPort = SpoutPort;
	identifier = "unknown";
	return;
	jobID = -1;

}
/* constructor
 * construct a tuple from a human readable string (which satifies the protocol
 * @param	msg		string		the data in string format
 * @param	val		int			for debug purpose, 0 should be fine for this parameter
 */
myTuple::myTuple(string msg, int val)
{
	compactTuple_t * cur = (compactTuple_t *)msg.c_str();
	jobID = cur->jobID;
	id = cur->id;
	SpoutPort = cur->spoutPort;
	SpoutIP = to_string(cur->spoutIP[0]) + "." + to_string(cur->spoutIP[1]) + "." + to_string(cur->spoutIP[2]) + "." + to_string(cur->spoutIP[3]);
	ack = cur->flag;
	string remains = string((char*)&(cur->remaining), cur->remainSize);


	stringstream ss(remains);
	//ss >> jobID >> id >> SpoutIP >> SpoutPort>> ack >> editorIP >> editorPort >> identifier;
	//ss >> jobID >> id >> SpoutIP >> SpoutPort >> ack >> identifier;
	ss >> identifier;
	string field, value;
	while (ss >> field >> value)
	{
		tupleData[field] = value;
	}

}

/* constructor
* construct a tuple from a compacted binary string (which satifies the protocol
* @param	msg		string		the data in string format
* @param	val		int			for debug purpose, 0 should be fine for this parameter
*/
void myTuple::fromNet(string msg, int val)
{
	compactTuple_t * cur = (compactTuple_t *)msg.c_str();
	jobID = cur->jobID;
	id = cur->id;
	SpoutPort = to_string( cur->spoutPort);
	SpoutIP = to_string(cur->spoutIP[0]) + "." + to_string(cur->spoutIP[1]) + "." + to_string(cur->spoutIP[2]) + "." + to_string(cur->spoutIP[3]);
	ack = cur->flag;
	string remains = string((char*)&(cur->remaining), cur->remainSize);


	stringstream ss(remains);
	//ss >> jobID >> id >> SpoutIP >> SpoutPort>> ack >> editorIP >> editorPort >> identifier;
	//ss >> jobID >> id >> SpoutIP >> SpoutPort >> ack >> identifier;
	ss >> identifier;
	string field, value;
	while (ss >> field >> value)
	{
		tupleData[field] = value;
	}

}

/* constructor
* construct a tuple from a human readable string (which satifies the protocol
* @param	msg		string		the data in string format
*/
myTuple::myTuple(string msg)
{
	stringstream ss(msg);
	//ss >> jobID >> id >> SpoutIP >> SpoutPort>> ack >> editorIP >> editorPort >> identifier;
	ss >> jobID >> id >> SpoutIP >> SpoutPort >> ack >> identifier;
	string field, value;
	while (ss >> field >> value)
	{
		tupleData[field] = value;
	}

}

myTuple::myTuple(int _jobID, string _spoutIP, string _spoutPort)
{
	static mutex staticTupleIDLock;
	staticTupleIDLock.lock();
	static int staticTupleID = 1;
	id = staticTupleID++;
	staticTupleIDLock.unlock(); 
	jobID = _jobID;
	SpoutIP = _spoutIP;
	SpoutPort = _spoutPort;
	ack = 0;
	editorIP = _spoutIP;
	editorPort = _spoutPort;
	identifier = "unknown";
}

/* toString
 * convert the tuple to a human readable string, which contains all the information about the tuple and revertable
 * @param		string		the object in string format
 */
string myTuple::toString()
{
	stringstream ss;
	//ss << jobID << " " << id << " " << SpoutIP << " " << SpoutPort << " " << ack << " " << editorIP << " " << editorPort << " " << identifier <<" ";

	ss << jobID << " " << id << " " << SpoutIP << " " << SpoutPort << " " << ack << " " << identifier << " ";

	for (auto & it : tupleData)
	{
		ss << it.first << " " << it.second << " ";
	}
	return ss.str();

}

/* toStringNetwork
 * convert the tuple into a slightly compact binary form, stored in string
 * @return	string		binary data stored in string
 */
string myTuple::toStringNetwork()
{
	stringstream ss;
	//ss << jobID << " " << id << " " << SpoutIP << " " << SpoutPort << " " << ack << " " << editorIP << " " << editorPort << " " << identifier <<" ";

	//ss << jobID << " " << id << " " << SpoutIP << " " << SpoutPort << " " << ack << " " << identifier << " ";

	ss << identifier << " ";
	
	for (auto & it : tupleData)
	{
		ss << it.first << " " << it.second << " ";
	}
	compactTuple_t cur;
	if (ss.str().size() >= TUPLEMAXSIZE - 1)
	{
		cout << "your tuple is too large" << endl;
		return string();
	}
	string data = ss.str();
	memcpy(&cur.remaining, data.c_str(), data.size() + 1);
	cur.id = id;
	cur.jobID = jobID;
	cur.remainSize = data.size();
	cur.flag = ack;
	if (SpoutPort.compare("spoutPort") == 0)
	{
		cur.spoutPort = 0;
	}else
	cur.spoutPort = (uint16_t)stoi(SpoutPort);
	if (SpoutIP.compare("spoutIP") != 0)
	{
		istringstream iss(SpoutIP);
		string val;
		for (int i = 0; i < 4; i++)
		{
			if (!getline(iss, val, '.'))
			{
				cout << "toString: ip corrupted" << endl;
			}
			//cout << val << endl;
			cur.spoutIP[i] = (uint8_t)stoi(val);
		}
	}
	else
	{
		for (int i = 0; i < 4; i++)
		{
			cur.spoutIP[i] = 0;
		}
	}
	int sizeofcur = sizeof(uint32_t) * 3 + sizeof(uint16_t) + sizeof(uint8_t) * 5 + cur.remainSize;
	string ret = string((char *)&cur, sizeofcur);



	return ret;

}

/* operator[]
 * to retrieve/assign the tuple data using a key
 * keywords like spoutIP are reserved and could be used to retrieve the corresponding value
 * @param	field		string		the key of a value
 * @return	string&		the data
 */
string& myTuple::operator[](string field)
{
	//if (field.compare("jobID") == 0) return to_string(jobID);
	//if (field.compare("tupleID") == 0) return to_string(id);
	if (field.compare("spoutIP") == 0) return SpoutIP;
	if (field.compare("spoutPort") == 0) return SpoutPort;
	if (field.compare("editorIP") == 0) return editorIP;
	if (field.compare("editorPort") == 0) return editorPort;
	if (field.compare("identifier") == 0) return identifier;
	//if (tupleData.find(field) != tupleData.end())
	return tupleData[field];
}

/* getJobID
 * get the job ID
 * @return	int		the job id
 */
int myTuple::getJobID()
{
	return jobID;
}

/* getTupleID
* get the tuple ID
* @return	int		the tuple id
*/
int myTuple::getTupleID()
{
	return id;
}

/* setJobID
* set JobID the tuple ID
* @set	id		int		the tuple id
*/
void myTuple::setJobID(int id)
{
	jobID = id;
}
	
/* checkAck
 * to check if the tuple is an ack or data
 * @return		true when the tuple is an ack
 */
bool myTuple::checkAck()
{
	return (ack&1) == 1;
}

/* setAck
 * set the ack flag of the tuple
 */
void myTuple::setAck()
{
	ack |= 1;
}

/* checkDrop
* to check if the tuple is an drop ack
* @return		true when the tuple is an drop ack
*/
bool myTuple::checkDrop()
{
	return (ack & 2) == 2;
}

/* setDrop
* set the drop flag of the tuple
*/
void myTuple::setDrop()
{
	ack |= 2;
}

/*
int myTuple::operator[](string field) const
{
	if (field.compare("jobID") == 0) return jobID;
	if (field.compare("tupleID") == 0) return id;
	return -1;
}
*/