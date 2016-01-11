/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Hongchuan Li
*students who are taking cs425 in UIUC should not copying this file for their project
*/

#include "topology.h"


TMTopologyInstance::TMTopologyInstance()
{
	//cout << "TMTopologyInstance:this constructor should not be called" << endl;
}


/* copy constructor*/
TMTopologyInstance::TMTopologyInstance(const TMTopologyInstance & obj)
{
	ip = obj.ip;
	port = obj.port;
	identifier = obj.identifier;
	thread = obj.thread;
	childIndex = obj.childIndex;
	child = obj.child;
}

/* constructor
 * construct a instance from a string satifies the protocol
 * @param		input		string
 */
TMTopologyInstance::TMTopologyInstance(string input)
{
	stringstream ss(input);
	int childSize;
	ss >> ip >> port >> identifier >> thread >> childSize;
	string childName;
	while (ss >> childName)
	{
		child.push_back(childName);
	}
}


TMTopologyInstance & TMTopologyInstance::operator=(const TMTopologyInstance & obj)
{
	this->ip = obj.ip;
	this->port = obj.port;
	this->identifier = obj.identifier;
	this->thread = obj.thread;
	this->childIndex = obj.childIndex;
	this->child = obj.child;
}

TMTopologyInstance::TMTopologyInstance(string _ip, string _port, string _identifier)
{
	ip = _ip;
	port = _port;
	identifier = _identifier;
}
/* pushChild
 * add an edge to the list
 * @param	_child		string		the name of a vertex where the edge pointed to
 */
void TMTopologyInstance::pushChild(string _child)
{
	instanceLock.lock();
	child.push_back(_child);
	instanceLock.unlock();
}

/* toString
 * convert the object into a human readable string
 * this method is revertable
 * this method is also used to transfer the data structure via internet
 * @return	string		the human readable string contains all the info about the object
 */
string TMTopologyInstance::toString()
{
	instanceLock.lock();
	stringstream ss;
	ss << ip << " " << port << " " << identifier << " " << thread << " " << child.size() << " ";
	for (auto & it : child)
	{
		ss << it << " ";
	}
	instanceLock.unlock();
	return ss.str();
}

/* getNextChild
 * return a child, in round robin fashion
 * @return	string		name of the next vertex
 */
string TMTopologyInstance::getNextChild()
{
	instanceLock.lock();
	if (child.size() == 0)
	{
		instanceLock.unlock();
		return string();
	}
	childIndex = (childIndex + 1) % child.size();
	string ret = child[childIndex];

	instanceLock.unlock();
	return ret;
}



/* constructor
 * construct the topology from a string
 * @param	input	string		the data in string format
 */
myTopology::myTopology(string input)
{
	stringstream ss(input);
	ss >> jobID >> fileName;
	string instStr;
	while (getline(ss, instStr))
	{
		if (instStr.size() == 0)
			continue;
		TMTopologyInstance inst(instStr);
		topoData[inst.identifier] = inst;
	}
}

/* toString
* convert the topology into a human readable string
* this method is revertable
* this method is also used to transfer the data structure via internet
* @return	string		the human readable string contains all the info about the topology
*/
string myTopology::toString()
{
	stringstream ss;
	ss << jobID <<  " " << fileName << "\n";
	for (auto & it : topoData)
	{
		ss << it.second.toString() << "\n";
	}
	return ss.str();
	
}


/* operator[]
 * retrieve an vertex using the vertex name or its ip+port
 * @param	key		string		the vertex name or its ip+port
 * @return	vector				return the vertex that satisfies the condition
 */
//maybe I should use the object instead of pointer, but I ll leave it for now
vector<TMTopologyInstance*> myTopology::operator[](string key)
{
	vector<TMTopologyInstance*> ret;
	if (topoData.find(key) != topoData.end())
	{
		ret.push_back(&topoData[key]);
		return ret;
	}
	//not identifier but ip+port
	for (auto & it : topoData)
	{
		if (key.compare(it.second.ip + it.second.port) == 0)
			ret.push_back(&(it.second));
	}

	return ret;
}

int myTopology::getjobID()
{
	return jobID;
}

