/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Hongchuan Li
*students who are taking cs425 in UIUC should not copying this file for their project
*/
#include "workerTopologyBuilder.h"

WInstance::WInstance()
{
	identifer = string();
	node = NULL;
	thread = 1;
	builder = NULL;
	child = vector<string>();
}
WInstance::WInstance(string _identifier, iNode * _node, int _thread, workerTopologyBuilder*_builder)
{
	identifer = _identifier;
	node = _node;
	thread = _thread;
	builder = _builder;
	child = vector<string>();
}

WInstance::WInstance(const WInstance& obj)
{
	identifer = obj.identifer;
	node = obj.node;
	thread = obj.thread;
	child = obj.child;
	builder = obj.builder;

}

WInstance & WInstance::operator= (const WInstance& obj)
{
	identifer = obj.identifer;
	node = obj.node;
	thread = obj.thread;
	child = obj.child;
	builder = obj.builder;
}

/* shuffleGrouping
 * attach the instance to the topology(to the parent), specify the grouping method to shuffle grouping
 * @param	parent		string		the name of parent vertex
 */
void WInstance::shuffleGrouping(string parent)
{
	if (builder != NULL)
		builder->shuffleGrouping(identifer, parent);
}

/* toString
 * convert the instance to a string
 * @return	string		a human readable string in the same format as TMTopologyInstance
 */
string WInstance::toString()
{
	stringstream ss;
	ss << "ip " << "port " << identifer << " " << thread << " " << child.size() << " ";
	for (auto & it : child)
	{
		ss << it << " ";
	}
	return ss.str();
}





/* setBolt
 * create a bold
 * @param	identifier		string	the name of the bolt, must be unique
 * @param	node			iNode*	bolt object
 * @param	thread			int		number of thread to run the bolt
 * @return	WInstance_t &			the bolt object created by setBolt method
 */
WInstance_t & workerTopologyBuilder::setBolt(string identifier, iNode* node, int thread)
{
	if (WTDEBUG) cout << "setBolt enter" << endl;
	if (topologyNode.find(identifier) != topologyNode.end())
		return topologyNode[identifier];
	WInstance_t inst(identifier, node, thread, this);
	topologyNode.insert(pair<string, WInstance_t>(identifier, inst));
	//topologyNode[identifier] = inst;
	if (WTDEBUG) cout << "topologyNode size() " << topologyNode.size() << endl;
	if (WTDEBUG) cout << "setBolt leave" << endl;
	return topologyNode[identifier];


}


/* setBolt
* create a spout
* @param	identifier		string	the name of the spout, must be unique
* @param	node			iNode*	bolt object
* @param	thread			int		number of thread to run the spout
* @return	WInstance_t &			the spout object created by setSpout method
*/
WInstance_t & workerTopologyBuilder::setSpout(string identifier, iNode* node, int thread)
{
	if (WTDEBUG) cout << "setSpout enter" << endl;
	if (topologyNode.find(identifier) != topologyNode.end())
		return topologyNode[identifier];
	WInstance_t inst(identifier, node, thread, this);
	topologyNode.insert(pair<string, WInstance_t>(identifier, inst));
	if (WTDEBUG) cout << "topologyNode size() " << topologyNode.size()<< endl;
	if (WTDEBUG) cout << "setSpout leave" << endl;
	return topologyNode[identifier];

}

/* shuffleGrouping
 * set the edge of the graph
 * user should not call this method directly, instead use WInstance.shuffleGrouping()
 * @param	child	string		the child name
 * @param	parent	string		the parent name
 */
void workerTopologyBuilder::shuffleGrouping(string child, string parent)
{
	if (WTDEBUG)cout << topologyNode.size() << endl;
	if (topologyNode.find(parent) == topologyNode.end())
	{
		cout << "something wrong" << endl;
	}

	topologyNode[parent].child.push_back(child);
}

/* exist
 * check if the identifier already exist
 * @param	identifier		const string	the identifier's name
 * @return	true if exist
 */
bool workerTopologyBuilder::exist(const string identifier)
{
	if (topologyNode.find(identifier) != topologyNode.end())
		return true;
	return false;
}

/* operator[]
 * get the WInstance_t object using the identifier
 * @param	identifier	string		name of the bolt/spout
 * @return	WInstance_t		the WInstance store in the builder
 */
WInstance_t & workerTopologyBuilder::operator[](const string identifier)
{
	if (topologyNode.find(identifier) != topologyNode.end())
		return topologyNode[identifier];
	return topologyNode[identifier];
}
//const iNode * workerTopologyBuilder::operator[](const string identifier) const;


/* toString
 * convert the builder to human readable string format, same as myTopology
 * @return		string		human readable string
 */
string workerTopologyBuilder::toString()
{
	stringstream ss;
	ss << 0 << " " << fileName<< "\n";
	for (auto & it : topologyNode)
	{
		ss << it.second.toString() << "\n";
	}
	return ss.str();
}