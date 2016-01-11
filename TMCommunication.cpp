/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Hongchuan Li
*students who are taking cs425 in UIUC should not copying this file for their project
*/



#include "TMCommunication.h"


TMcommunication::TMcommunication()
{
	//listener = new tcpwrapper();
	udpLister = new udpWrapper();
	sender = new udpWrapper();
}

/* start
 * setup the listening port, start to listen the incomming udp message
 * @param	listenPort		string	the port number
 */
int TMcommunication::start(string listenPort)
{
	char *cstr = new char[listenPort.length() + 1];
	strcpy(cstr, listenPort.c_str());
	//listener->tcpListen(cstr);
	udpLister->udpRecSetup(cstr);
	delete cstr;
	using namespace std::placeholders;
	udpLister->udpRecAsyn(bind(&TMcommunication::receiveHelper, this, _1), 0, 0);
	//listener->tcpAcceptRecAsyncSingleThread(bind(&TMcommunication::receiveHelper, this, _1), 0, 0);
}

/* regHandler
 * register the handler for incomming message
 * @param	cb		the function object
 */
int TMcommunication::regHandler( std::function<int(TMCInfo)> cb)
{
	callbackList.push_back(cb);
}

/* send
 * send the message to the corresponding machine
 * @param	ip		string		receiver's ip address
 * @param	port	string		receiver's port number
 * @param	msg		string		the message
 */
int TMcommunication::send(string ip, string port, string msg)
{
	//tcpwrapper* sender = new tcpwrapper();
	udpWrapper* sender = new udpWrapper();
	char *cstr = new char[port.length() + 1];
	strcpy(cstr, port.c_str());
	
	//sender->tcpConnect(ip.c_str(), cstr);
	delete cstr;

	string container = to_string(TMCCMD) +" "+ msg;

	cstr = new char[container.length() + 1];
	strcpy(cstr, container.c_str());
	sender->udpSend(container, ip, port);
	
	//sender->tcpSend(cstr, container.length());

	delete sender;
	
}


/* send
* send the tuple to the corresponding machine
* this send will cache the tuple
* todo: i should also try to cache all the ack message to speed up the program
* @param	ip		string		receiver's ip address
* @param	port	string		receiver's port number
* @param	tuple	myTuple		the tuple data structure
*/

int TMcommunication::sendTuple(string ip, string port, myTuple tuple)
{
	//udpWrapper* sender = new udpWrapper();
	//tcpwrapper* sender = new tcpwrapper();
	//cout << "sendTuple enter" << endl;
	string container = to_string(TMCTUPLECOMPECT) + " ";
	cache[ip + port].push_back(tuple);
	if (cache[ip + port].size() > 10)
	{
		for (auto & it : cache[ip + port])
		{
			string temp = it.toStringNetwork();
			uint32_t msgSize = temp.size();
			temp = to_string(msgSize) + " " + temp;
			container += temp;

		}
		cache.erase(ip + port);
		//cout << "msg sent" << endl;
	}
	else
	{
		
		//cout << "msg reserved" << endl;
		return 0;
	}
	//cout << container.size() << endl;


	//container = to_string(TMCTUPLE) + " " + tuple.toStringNetwork();

	char *cstr = new char[container.length() + 1];
	strcpy(cstr, container.c_str());
	//sender->tcpSend(cstr, container.length());
	sender->udpSend(container, ip, port);
	delete cstr;
	//cout << "sendTuple leave" << endl;
	//delete sender;
	
}

/* send
* send the topology to the corresponding machine
* @param	ip		string		receiver's ip address
* @param	port	string		receiver's port number
* @param	topo	myTopology	the topology's data structure
*/
int TMcommunication::sendTopology(string ip, string port, myTopology topo)
{
	//tcpwrapper* sender = new tcpwrapper();
	udpWrapper* sender = new udpWrapper();
	char *cstr = new char[port.length() + 1];
	strcpy(cstr, port.c_str());
	//sender->tcpConnect(ip.c_str(), cstr);
	delete cstr;

	string container = to_string(TMCTOPOLOGY) + " " + topo.toString();

	cstr = new char[container.length() + 1];
	strcpy(cstr, container.c_str());
	//sender->tcpSend(cstr, container.length());
	sender->udpSend(container, ip, port);

	delete sender;
}

/* send
* send the ack to the corresponding machine
* @param	ip		string		receiver's ip address
* @param	port	string		receiver's port number
* @param	tuple	myTuple		the tuple data structure
*/
int TMcommunication::sendAck(myTuple tuple)
{
	udpWrapper* sender = new udpWrapper();
	//tcpwrapper* sender = new tcpwrapper();
	char *cstr = new char[tuple["spoutIP"].length() + 1];
	strcpy(cstr, tuple["spoutPort"].c_str());
	//sender->tcpConnect(tuple["spoutIP"].c_str(), cstr);
	delete cstr;

	string container = to_string(TMCTUPLE) + " " + tuple.toString();

	cstr = new char[container.length() + 1];
	strcpy(cstr, container.c_str());
	//sender->tcpSend(cstr, container.length());
	sender->udpSend(container, tuple["spoutIP"], tuple["spoutPort"]);
	delete sender;
}

/* receiveHelper
 * use to handle message received from udp layer
 * @param	info	udpInfo&	contains the received message
 */
int TMcommunication::receiveHelper(udpInfo & info)
{
	/*
	if (info.type != REC)
		return 0;
	if (TMCDEBUG) cout << "receiveHelper enter" << endl;
	if (info.state == TCPCLOSE || info.state == TCPERROR)
	{
		if (TMCDEBUG) cout << "receiveHelper leave" << endl;
		return 0;
	}

	if (info.state = SOCKCLOSE && info.msg.size() == 0)
	{
		if (TMCDEBUG) cout << "receiveHelper leave" << endl;
		return 0;
	}
	*/
	//cout << "receive: ";
	//cout << info.msg.c_str() << endl;
	stringstream ss(info.msg);
	int type;
	ss >> type;
	TMCInfo tmcinfo;
	tmcinfo.type = type;

	if (type == TMCTUPLECOMPECT)
	{
		int msgSize;


		while (ss >> msgSize)
		{
			//cout << msgSize << endl;

			char* data = new char[msgSize + 1];
			ss.readsome(data, msgSize);

			TMCInfo ret;
			ret.type = TMCTUPLE;
			ret.msg = string(data, data + msgSize);

			delete data;

			if (ret.msg.size() > 400)
			{
				cout << "error" << endl;
				while (1);
			}

			for (auto & it : callbackList)
			{
				it(ret);
			}
			//std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		}
		return 0;
	}


	tmcinfo.msg = string(ss.str().substr(ss.tellg()));
	//cout << "tmcinfo.msg size" << tmcinfo.msg.size() << endl;
	//cout << tmcinfo.msg << endl;
	for (auto & it : callbackList)
	{
		it(tmcinfo);
	}
	if (TMCDEBUG) cout << "receiveHelper leave" << endl;
}