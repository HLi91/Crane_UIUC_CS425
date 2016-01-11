/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Hongchuan Li
*students who are taking cs425 in UIUC should not copying this file for their project
*/
#ifndef _WORKERTOPOLOGYBUILDER_H_
#define _WORKERTOPOLOGYBUILDER_H_

#include <sstream>
#include "inode.h"

#define WTDEBUG 0

using namespace std;
class workerTopologyBuilder;
/* WInstance
 * contains a vertex of the directed graph and the edge from that vertex
 */
typedef struct WInstance
{
	string identifer;
	vector<string> child;
	iNode* node;
	int thread;
	workerTopologyBuilder* builder;
	WInstance();
	WInstance(const WInstance& obj);
	WInstance(string _identifier, iNode * _node, int _thread, workerTopologyBuilder*_builder);
	WInstance & operator=(const WInstance& obj);
	void shuffleGrouping(string parent);
	string toString();
}WInstance_t;

class workerTopologyBuilder
{
	map<string, WInstance> topologyNode;
	string fileName = "unknown";
	//map<string, vector<string>> topologyTree;

public:
	WInstance_t & setBolt(string identifier, iNode* node, int thread);
	WInstance_t & setSpout(string identifier, iNode* node, int thread);
	void shuffleGrouping(string child,  string parent);
	void setFileName(string _fileName){ fileName = _fileName; }
	WInstance_t & operator[](const string identifier);
	//const iNode * operator[](const string identifier) const;

	string toString();
	bool exist(const string identifier);



};


#endif