/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Hongchuan Li
*students who are taking cs425 in UIUC should not copying this file for their project
*/

#ifndef _TOPOLOGY_H_
#define _TOPOLOGY_H_

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <mutex>
#include <iostream>

using namespace std;


/* TMTopologyInstance
 *  each instance contains a vertex object of the directed graph and the edges from that vertex
 */
class TMTopologyInstance
{
	vector<string> child;		//identifier
	int childIndex = 0;
	mutex instanceLock;
public:
	string ip, port;
	string identifier;
	int thread;
	

	string toString();
	TMTopologyInstance();
	TMTopologyInstance(const TMTopologyInstance & obj);
	TMTopologyInstance & operator=(const TMTopologyInstance & obj);
	TMTopologyInstance(string input);
	TMTopologyInstance(string _ip, string _port, string _identifier);
	void pushChild(string _child);
	string getNextChild();
};


/* myTopology
 * the topology data structure and corresponding method
 */
class myTopology
{
	
	
	
public:

	map<string, TMTopologyInstance> topoData;
	int jobID;
	string fileName;
	mutex topologyMutex;
	myTopology() :jobID(-1){};
	
	myTopology(string input);
	myTopology(const myTopology & obj)
	{
		
		topoData = obj.topoData;
		jobID = obj.jobID;
		fileName = obj.fileName;
		
	}
	myTopology& operator=(const myTopology& obj)
	{

		topoData = obj.topoData;
		jobID = obj.jobID;
		fileName = obj.fileName;

	}
	string toString();
	vector<TMTopologyInstance*> operator[](string key);//find obj both using key and ipport
	int getjobID();
	int getThread();
	//todo:implement iterator
};

#endif
