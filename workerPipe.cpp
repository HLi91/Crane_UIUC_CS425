/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Junqing Deng
*students who are taking cs425 in UIUC should not copying this file for their project
*/
#include "workerPipe.h"

//start a thread to read incomming tuple/msg
void ioCollector::start()
{
	if (startFlag == 0) { 
		readThread = new std::thread(&ioCollector::recvWorker, this);
		startFlag = 1;
	}
}

//close the read thread
void ioCollector::close()
{
	stopFlag = 1;
	readThread->join();
	startFlag = 0;
}

//return a tuple if available, else block, fill count with 
myTuple ioCollector::getNextTuple(int& count)
{
	//cout << "getNextTuple" << endl;
	
	readLock.lock();
	volatile int temp_size = buffer.size();

	if (temp_size == 0) 
	{
		
		waitOne.try_lock();
		readLock.unlock();
		waitOne.lock();
		readLock.lock();
		//waitOne.unlock();

		/*
		readLock.unlock();
		while(!temp_size) {
			readLock.lock();
			temp_size = buffer.size();
			if (temp_size >= 1) {
				break;
			}
			readLock.unlock();
		}*/
	}

	myTuple temp = buffer.front();
	buffer.pop_front();
	count = buffer.size();
	readLock.unlock();

	return temp;

}

//send the tuple back, needed to end with ; as a special char
void ioCollector::emit(myTuple tuple)
{

	tuple["identifier"] = identifier;
	tuple.setJobID(jobID);
	cout << PIPETUPLE <<tuple.toString() << ";";
	cout << flush;
}

void ioCollector::ack(myTuple tuple)
{

	tuple["identifier"] = identifier;
	tuple.setJobID(jobID);
	tuple.setDrop();
	cout << PIPETUPLE << tuple.toString() << ";";
	cout << flush;
}

void ioCollector::emit(string msg)
{
	cout << PIPETOPO << msg << ";";
	cout << flush;
}

//keep reading from stdin to buffer
void ioCollector::recvWorker() {
	
	std::string reader = "";
	std::string temp;
	int pos = 0;
	//int counter = 0;
	while (!stopFlag) {

		std::getline(std::cin, temp);
		
		if (temp[temp.length()-1] != ';') temp += '\n';
		reader += temp;
		temp.clear();

		pos = reader.find(';');
		if (pos != std::string::npos) {
			char type = reader[0];
			string tuple = reader.substr(1, pos);
			if (type == PIPETUPLE) {
				readLock.lock();
				buffer.push_back(myTuple(tuple));
				waitOne.unlock();
				readLock.unlock();
			}
			reader = reader.substr(pos+1);
			
		}

	}

}


void ioCollector::requestFileFromSDFS(string fileName)
{
	stringstream ss;
	ss << "1" << " " << fileName;
	emit(ss.str());
}
void ioCollector::putFileIntoSDFS(string fileName)
{
	stringstream ss;
	ss << "0" << " " << fileName;
	emit(ss.str());
}
