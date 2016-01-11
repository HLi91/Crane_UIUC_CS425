/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Junqing Deng
*students who are taking cs425 in UIUC should not copying this file for their project
*/

/*this is an example of user code*/

#include "workerSys.h"
#include <sstream>
#include <fstream>
//#include <string>
#include <iostream>
//#include <stdlib.h>


//=======================================================================================

class packerSizeReaderSpout : public ispout
{
	ioCollector* _collector;
	ifstream* myfile;
	int count = 0;

	void prepare(ioCollector *Collector)
	{
		_collector = Collector;
		myfile = new ifstream();
		myfile->open("/home/jdeng5/mp4/data/BC-pAug89.TL");
	}

	void nextTuple()
	{
		myTuple tuple;
		string line;
		string time;
		string size;
		if (myfile->is_open())
		{
			if (getline(*myfile, line)) {
				stringstream ss(line);
				ss >> time;
				ss >> size;
				tuple["time"] = time;
				tuple["size"] = size;
				_collector->emit(tuple);
			}
			else if (count < 10000){
				tuple["time"] = "0";
				tuple["size"] = "0";
				count++;
				_collector->emit(tuple);
			}
		}
	}

	void close()
	{
		myfile->close();
		delete(myfile);
	}

	packerSizeReaderSpout* clone()
	{
		packerSizeReaderSpout* copy = new packerSizeReaderSpout();
		copy->_collector = this->_collector;
		copy->myfile = this->myfile;
		return copy;
	}


};
/**/
class packetSizeFilter : public ibolt
{
	ioCollector* _collector;
	void close()
	{

	}
	void execute(myTuple tuple)
	{
		int size = atoi(tuple["size"].c_str());
		if (size < 10) {
			_collector->ack(tuple);
			return;
		}
		_collector->emit(tuple);

	}
	void prepare(ioCollector *Collector)
	{
		_collector = Collector;
	}
	packetSizeFilter* clone()
	{
		packetSizeFilter* copy = new packetSizeFilter();
		copy->_collector = this->_collector;
		return copy;
	}
};


class packetSizeRange : public ibolt
{
	ioCollector* _collector;
	void close()
	{

	}
	void execute(myTuple tuple)
	{
		int size = atoi(tuple["size"].c_str());
		if (size < 100) {
			tuple["size"] = "0";
		}
		else if (size < 1000) {
			tuple["size"] = "1";
		}
		else if (size < 10000) {
			tuple["size"] = "2";
		}
		else {
			tuple["size"] = "3";
		}
		_collector->emit(tuple);

	}
	void prepare(ioCollector *Collector)
	{
		_collector = Collector;
	}
	packetSizeRange* clone()
	{
		packetSizeRange* copy = new packetSizeRange();
		copy->_collector = this->_collector;
		return copy;
	}
};

class packetSizeMaxRange : public ibolt
{
	ioCollector* _collector;
	mutex* writeLock;
	map<string, string>* myMap;
	vector<int>* count;

	void close()
	{
		delete(writeLock);
		delete(myMap);
		delete(count);
	}
	void execute(myTuple tuple)
	{

		int size = atoi(tuple["size"].c_str());
		writeLock->lock();
		if (myMap->find(tuple["time"]) == myMap->end()) {
			myMap->insert(std::pair<string, string>(tuple["time"], tuple["size"]));
			count->at(size) = count->at(size) + 1;

			int max = count->at(0);
			int range = 0;
			for (int i = 1; i < 4; i++) {
				if (count->at(i) > max) {
					max = count->at(i);
					range = i;
				}
			}

			string result = tuple["time"] + " " + tuple["size"] + " ";

			switch (range) {
			case 0:
				result += "Packet Size Range [10,100) counts most.";
				break;

			case 1:
				result += "Packet Size Range [100,1000) counts most.";
				break;

			case 2:
				result += "Packet Size Range [1000,10000) counts most.";
				break;

			case 3:
				result += "Packet Size Range [10000,Infinity) counts most.";
				break;
			}

			ofstream myfile;
			myfile.open("/home/jdeng5/mp4/data/cranePacketMaxRange.txt", std::ofstream::out | std::ofstream::app);
			myfile << result << "\n";
			myfile.flush();
			myfile.close();
			//tuple["result"] = result;
		}
		writeLock->unlock();
		_collector->emit(tuple);

	}
	void prepare(ioCollector *Collector)
	{
		_collector = Collector;
		writeLock = new mutex();
		myMap = new map<string, string>;
		count = new vector<int>(4, 0);
	}
	packetSizeMaxRange* clone()
	{
		packetSizeMaxRange* copy = new packetSizeMaxRange();
		copy->_collector = this->_collector;
		copy->writeLock = this->writeLock;
		copy->myMap = this->myMap;
		copy->count = this->count;
		return copy;
	}
};

//===============================================================================================

int main(int argc, char *argv[])
{
	workerTopologyBuilder* builder = new workerTopologyBuilder();

	builder->setSpout("packerSizeSpout", new packerSizeReaderSpout(), 1);
	builder->setBolt("filter", new packetSizeFilter(), 1).shuffleGrouping("packerSizeSpout");
	builder->setBolt("range", new packetSizeRange(), 1).shuffleGrouping("filter");
	builder->setBolt("count", new packetSizeMaxRange(), 1).shuffleGrouping("range");

	workerSys os;
	os.init(*builder);
	os.execute(argc, argv);


	return 0;
}