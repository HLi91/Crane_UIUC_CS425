/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Hongchuan Li, modified by Junqing Deng
*students who are taking cs425 in UIUC should not copying this file for their project
*/

#include "workerSys.h"
#include <sstream>
#include <fstream>
//#include <string>
#include <iostream>
//#include <stdlib.h>


/* increamentSpout
 * user code example
 * user defined spout should inherent from ispout
 * user defined spout must contain a prepare method, nuxtTuple method and a clone method
 */
class increamentSpout : public ispout
{
	ioCollector* _collector;
	int counter;
	void prepare(ioCollector *Collector)
	{
		_collector = Collector;
		counter = 0;
	}

	void nextTuple()
	{
		myTuple tuple;
		tuple["val"] = to_string(counter);
		counter++;
		_collector->emit(tuple);
	}

	void close()
	{

	}
	
	increamentSpout* clone()
	{
		increamentSpout* copy = new increamentSpout();
		copy->_collector = this->_collector;
		copy->counter = this->counter;
		return copy;
	}
	

};



/* exclaim
* user code example
* user defined bolt should inherent from ibolt
* user defined bolt must contain a prepare method, execute method and a clone method
*/
class exclaim : public ibolt
{
	ioCollector* _collector;
	void close()
	{

	}
	void execute(myTuple tuple)
	{
		tuple["val"] = tuple["val"] + "!!!";
		_collector->emit(tuple);

	}
	void prepare(ioCollector *Collector)
	{
		_collector = Collector;
	}
	exclaim* clone()
	{
		exclaim* copy = new exclaim();
		copy->_collector = this->_collector;
		return copy;
	}
};


class quest : public ibolt
{
	ioCollector* _collector;
	void close()
	{

	}
	void execute(myTuple tuple)
	{
		tuple["val"] = tuple["val"] + "???";
		_collector->emit(tuple);

	}
	void prepare(ioCollector *Collector)
	{
		_collector = Collector;
	}
	quest* clone()
	{
		quest* copy = new quest();
		copy->_collector = this->_collector;
		return copy;
	}
};


/* filter
 * user code example
 * to filter and destory a tuple, ack method must be called
 * to passed the tuple to the next bolt if exist, emit method must be called
 */
class filter : public ibolt
{
	ioCollector* _collector;
	void close()
	{

	}
	void execute(myTuple tuple)
	{
		//tuple["val"] = tuple["val"] + "???";
		//_collector->emit(tuple);
		int val = stoi(tuple["val"]);
		if (val % 2)
		{
			_collector->emit(tuple);
		}
		else
		{
			_collector->ack(tuple);
		}

	}
	void prepare(ioCollector *Collector)
	{
		_collector = Collector;
	}
	filter* clone()
	{
		filter* copy = new filter();
		copy->_collector = this->_collector;
		return copy;
	}
};


/* main
 * user code example
 * user should build the topology by using the workerTopologyBuilder
 * user must create the workerSys object, pass the argc,argv to the execute method to hand the control to the system
 */
int main(int argc, char *argv[])
{
	workerTopologyBuilder* builder = new workerTopologyBuilder();
	
	builder->setSpout("increamentSpout", new increamentSpout(), 1);
	builder->setBolt("ex1", new filter(), 1).shuffleGrouping("increamentSpout");
	builder->setBolt("ex2", new exclaim(), 1).shuffleGrouping("ex1");
	//builder->setBolt("qu3", new quest(), 1).shuffleGrouping("ex1");
	
	workerSys os;
	os.init(*builder);
	os.execute(argc, argv);


	return 0;
}