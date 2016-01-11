#include "heartBeat.h"


/*join
join the current process into the heartbeat network
when the ip address is the local ip, the current is treat as introducer
@param	listeningPort	input	string	the port number used for receiving udp connection
@param	ip				input	string	introducer's ip address
@param	port			input	string	introducer's ip port
@param	cb				input	hbCallback	callback function, used to inform user when something happened(note fail for example)
@return 0 for success
*/
int heartBeat::join(string listeningPort, string ip, string port, std::function<int(hbInfo&)> cb)
{
	if (HBDEBUG)printf("enter heartBeat::join\n");
	if (heartBeatTable.size() != 0)
	{
		cout << "already in the group" << endl;
		return 0;
	}
	myInfo.cb.push_back( cb);
	heartBeatTable.clear();
	hbtable_t initial;
	initial.ip = ip;
	initial.port = port;
	initial.name = 0;
	initial.Hcounter = -1;
	initial.time = -1;
	initial.alive = ALIVEINTRO;
	heartBeatTable.insert(pair<string, hbtable_t>(ip + port, initial));

	hbtable_t me;
	me.ip = myip();
	me.port = listeningPort;
	me.name = (int)(getCurrentTime());
	me.Hcounter = 0;
	me.time = -1;
	me.alive = ALIVEME;
	if (heartBeatTable.find(myip() + listeningPort) != heartBeatTable.end())
		heartBeatTable.clear();
	heartBeatTable.insert(pair<string, hbtable_t>(myip() + listeningPort, me));

	myport = listeningPort;
	myName = me.name;

	sender = new udpWrapper();		//just initialize
	listener = new udpWrapper();
	listener->udpRecSetup(listeningPort.c_str());
	using namespace std::placeholders;
	listener->udpRecAsyn(bind(&heartBeat::recHBMsg, this, _1), 0, NULL);

	qFlag = new mutFlagUDP();
	

	

	joinWork = new std::thread(&heartBeat::joinHelper, this);

	if (HBDEBUG)printf("leave heartBeat::join\n");
	return 0;

}


int heartBeat::broadcast(string msg)
{
	return multicastNaive(msg, 1, 2*heartBeatTable.size() - 2);
}
int heartBeat::multicastNaive(string msg, int ttl, int nodeNum)
{
	mut_multiMsgList.lock();
	multiMsgList.push_back(multicastMsg_t(msg, ttl, nodeNum));
	mut_multiMsgList.unlock();
	return 0;
}

/*myip
Get my ip address
@return	addr	string	my ip address
*/
string heartBeat::myip()
{
	if (HBDEBUG)printf("enter heartBeat::myip\n");
	if (curIP.length() != 0)
		return curIP;
	struct ifaddrs *ifap, *ifa;
	struct sockaddr_in *sa;
	char *addr;

	getifaddrs(&ifap);
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_INET) {
			sa = (struct sockaddr_in *) ifa->ifa_addr;
			addr = inet_ntoa(sa->sin_addr);
			printf("Interface: %s\tAddress: %s\n", ifa->ifa_name, addr);
		}
	}

	freeifaddrs(ifap);

	curIP = addr;
	if (HBDEBUG)printf("leave heartBeat::myip\n");
	return addr;

}

/*getCurrentTime
@return	time	long	the system time in ms(used as timestamp)
*/
long heartBeat::getCurrentTime()
{
	if (HBDEBUG)printf("enter heartBeat::getCurrentTime\n");


	using namespace std::chrono;
	milliseconds ms = duration_cast< milliseconds >(
		system_clock::now().time_since_epoch()
		);

	if (HBDEBUG)printf("leave heartBeat::getCurrentTime\n");

	return ms.count() % INT_MAX;

}


/*joinHelper
helper function for join*/
void heartBeat::joinHelper()
{
	if (HBDEBUG)printf("enter heartBeat::joinHelper\n");


	using namespace std::this_thread; // sleep_for, sleep_until
	using namespace std::chrono;
	while (1)
	{
		qFlag->mut.lock();
		if (qFlag->quit == true)
		{
			qFlag->mut.unlock();
			return;

		}
		qFlag->mut.unlock();

		updateMe();
		sendGossip();		
		sleep_for(milliseconds(FREQ));			//todo:adjust this w/ size

		if (HBDEBUG)sleep_for(milliseconds(5000));
	}
	if (HBDEBUG)printf("leave heartBeat::joinHelper\n");
}

/*sendGossip
send gossip msg to some random selected node, the currently value is set to 2 note per function call
the node is select in a random starving free fashion*/
void heartBeat::sendGossip()
{

	if (HBDEBUG)printf("enter heartBeat::sendGossip\n");


	
	//cout << msg << endl;
	int counter = 2;
	while (counter>0)
	{
		string ip, port;
		if (randomIP(ip, port, counter)!=0)
		{
			if (HBDEBUG)printf("leave \n");
			return;
		}
		string msg = constructList();
		sender->udpSend(msg, ip, port);
		//counter--;
	}

	if (HBDEBUG)printf("leave heartBeat::sendGossip\n");
}
/*updateMe
update the heartBeatTable
increament the current process' entry
take care of the timeout node*/
void heartBeat::updateMe()
{
	if (HBDEBUG)printf("enter heartBeat::updateMe\n");


	if (heartBeatTable.find(myip() + myport) == heartBeatTable.end())
	{
		if (HBDEBUG)printf("leave heartBeat::updateMe error\n");
		return;
	}
	//heartBeatTable[myip() + myport].Hcounter++;

	//long curTime = getCurrentTime();
	mut_HBT.lock();
	for (auto& it : heartBeatTable)
	{
	
		if (it.first.compare(myip() + myport) == 0)
		{
			if (heartBeatTable[myip() + myport].alive && ALIVEMASK == ALIVEFALSE)
			{
				heartBeatTable[myip() + myport].alive |= ALIVETRUE;
				heartBeatTable[myip() + myport].name = (int)(getCurrentTime());
				myName = heartBeatTable[myip() + myport].name;
				heartBeatTable[myip() + myport].Hcounter = 0;
			
			}
			heartBeatTable[myip() + myport].Hcounter++;
		}
		else
		{
			
			long diff = getCurrentTime() - it.second.time;
			//cout << "diff ip is " << it.second.ip << endl;
			//cout << "diff is " << diff << endl;
			if (diff > 2 * TIMEOUT)
			{
				if ((it.second.alive & ALIVEINPUTMASK) == ALIVEINTRO)
					continue;
				//cout << "alive is " << (int)it.second.alive << endl;
				myInfo.msgType = CBDELETE;
				myInfo.msg = "client at ip: " + it.second.ip + ":" + it.second.port + "name: " + to_string(it.second.name) + "\tdeleted\t" + to_string(getCurrentTime()) + "\n";
				
				//myInfo.cb(myInfo);
				for (auto & it : myInfo.cb)
					it(myInfo);
				heartBeatTable.erase(it.first);
				myInfo.msg.clear();
			}
			else if (diff > TIMEOUT)
			{
				if (it.second.alive != ALIVEFALSE)
				{
					//if ((it.second.alive & ALIVEINPUTMASK) == (ALIVEINTRO &ALIVEINPUTMASK))
					//	continue;
					if ((it.second.alive & ALIVEMASK) == ALIVEFALSE)
						continue;
					it.second.alive &= ALIVEINPUTMASK;
					myInfo.msgType = CBTIMEOUT;
					myInfo.msg = "client at ip: " + it.second.ip + ":" + it.second.port + "name: " + to_string(it.second.name) + "\ttimeout\t" + to_string(getCurrentTime()) + "\n";
					myInfo.ip = it.second.ip;
					myInfo.port = it.second.port;
					//myInfo.cb(myInfo);
					for (auto & it : myInfo.cb)
						it(myInfo);
					myInfo.msg.clear();
				}
			}
		
		}
	
	}
	mut_HBT.unlock();
	if (HBDEBUG)printf("leave heartBeat::updateMe\n");
}
/*randomIP
select the ip from the list randomly in a starving free fashion
@param	ip		out		string		the random ip
@param	port	out		string		the port number related to the ip
@param	counter	i/o		int			the total number of ip that need to be generate, will be decreament each time after a random ip is return*/
int heartBeat::randomIP(string & ip, string & port, int & counter)
{
	if (HBDEBUG)printf("enter heartBeat::randomIP\n");
	if (randList.size() == 0)
	{
		mut_HBT.lock();
		for (auto& it : heartBeatTable)
		{
			if (it.first.compare(myip() + myport)!=0)
				randList.push_back(pair<string, string>(it.second.ip, it.second.port));		
		}
		mut_HBT.unlock();

	}

	if (randList.size() == 0)
	{
		if (HBDEBUG)printf("leave heartBeat::randomIP\n");
		return -1;
	}
	if (counter >= randList.size())
		counter = randList.size();

	srand(time(NULL));
	int index = rand() % randList.size();
	ip = randList[index].first;
	port = randList[index].second;

	randList.erase(randList.begin() + index);
	counter--;
	if (HBDEBUG)printf("leave heartBeat::randomIP\n");

	return 0;
	
}

/*constructList
construct the gossip msg base on the current heartBeatTable
@return		out		string		the gossip msg*/
string heartBeat::constructList()
{
	if (HBDEBUG)printf("enter heartBeat::constructList\n");


	mut_HBT.lock();
	string out;
	out += "\r\n<body>\r\n";
	for (auto& it : heartBeatTable)
	{
		out += "\t<gmsg>\r\n";
		out += "\t\t<ip>"+it.second.ip+"</ip>\r\n";
		out += "\t\t<port>" + it.second.port + "</port>\r\n";
		out += "\t\t<name>" + to_string(it.second.name) + "</name>\r\n";
		out += "\t\t<counter>" + to_string(it.second.Hcounter) + "</counter>\r\n";
		out += "\t\t<alive>" + to_string(it.second.alive);
		out += "</alive>\r\n";
		out += "\t</gmsg>\r\n";
	
	}
	mut_HBT.unlock();

	mut_multiMsgList.lock();

	
	if (multiMsgList.size() > 0)
	{
		
		multiMsgList[0]._nodeNum--;
		out += "\t <multicast>\r\n";
		out += "\t\t<ttl>" + to_string(multiMsgList[0]._ttl) + "</ttl>\r\n";
		out += "\t\t<mmsg>" + multiMsgList[0]._msg + "</mmsg>\r\n";
		out += "\t </multicast>\r\n";
		if (multiMsgList[0]._nodeNum == 0)
		{
			multiMsgList.erase(multiMsgList.begin());
		}
		//cout << out << endl;
		//cout << "multiMsgList.size() = " << multiMsgList.size() << endl;
		//cout << "multiMsgList[0]._nodeNum" << multiMsgList[0]._nodeNum << endl;
	}
	
	mut_multiMsgList.unlock();

	out += "</body>\r\n";
	if (HBDEBUG)printf("leave heartBeat::constructList\n");
	return out;

}


/*processInfo
decrypt the msg, each node at a time
@param	info		i/o		udpInfo		the msg the current position is stored at
@param	ip			out		string		the ip address
@param	port		out		string		the port number
@param	name		out		int			the name related to the node
@param	Hcounter	out		int			heartBeat counter
@param	alive		out		char		the status of the node
@return	0 when complete		positive number when there are more to process*/
int heartBeat::processInfo(udpInfo & info, string & ip, string &port, int & name, int & Hcounter, char & alive)
{
	size_t begin = info.msg.find("<gmsg>", info.pos);
	if (begin == string::npos)
		return 0;
	info.pos = info.msg.find("</gmsg>", info.pos) + 7;
	size_t endBody = info.msg.find("</body>", info.pos) + 7;


	size_t startptr = info.msg.find("<ip>", begin)+4;
	size_t endptr = info.msg.find("</ip>", startptr);
	if (startptr < info.pos)
		ip = info.msg.substr(startptr, endptr - startptr);

	startptr = info.msg.find("<port>", begin) + 6;
	endptr = info.msg.find("</port>", startptr);
	if (startptr < info.pos)
		port = info.msg.substr(startptr, endptr - startptr);

	startptr = info.msg.find("<name>", begin) + 6;
	endptr = info.msg.find("</name>", startptr);
	if (startptr < info.pos)
		name = stoi(info.msg.substr(startptr, endptr - startptr));

	startptr = info.msg.find("<counter>", begin) + 9;
	endptr = info.msg.find("</counter>", startptr);
	if (startptr < info.pos)
		Hcounter = stoi(info.msg.substr(startptr, endptr - startptr));

	startptr = info.msg.find("<alive>", begin) + 7;
	endptr = info.msg.find("</alive>", startptr);
	if (startptr < info.pos)
		alive = stoi(info.msg.substr(startptr, endptr - startptr));

	return info.msg.length() - info.pos;



}
/*processMulticast
resolve the multicast msg from the received heartbeat, potentially this function could be used to continue multicast the msg when ttl is not 0, not test yet
@param	info	udpInfo&	containing the packet the received from UDP layer
@param	msg		string&		output		the multicast msg
@return	int		1: there is a multicast msg, 0: there isnt
*/
int heartBeat::processMulticast(udpInfo & info, string & msg)
{
	size_t begin = info.msg.find("<multicast>", 0);
	if (begin == string::npos)
		return 0;
	size_t startptr = info.msg.find("<ttl>", begin) + 5;
	size_t endptr = info.msg.find("</ttl>", startptr);
	int ttl = stoi(info.msg.substr(startptr, endptr - startptr)) - 1;

	startptr = info.msg.find("<mmsg>", begin) + 6;
	endptr = info.msg.find("</mmsg>", startptr);
	string recvMsg = info.msg.substr(startptr, endptr - startptr);

	
	if (ttl > 0)
	{
		mut_multiMsgList.lock();
		multiMsgList.push_back(multicastMsg_t(recvMsg, ttl, heartBeatTable.size() - 1));
		mut_multiMsgList.unlock();
	}

	msg = recvMsg;
	return 1;


}

/*getTable
create a copy of the current heartBeatTable
@return copy of heartBeatTable*/
map < string, hbtable_t > heartBeat::getTable()
{

	mut_HBT.lock();
	map < string, hbtable_t > temp = heartBeatTable;
	mut_HBT.unlock();
	return temp;
}

/*getMe
create a copy of structure that contain info about the current process*/
hbtable_t heartBeat::getMe()
{
	mut_HBT.lock();
	hbtable temp = heartBeatTable[myip() + myport];
	mut_HBT.unlock();
	return temp;
}

/*recHBMsg
the callback function passed to the udpWrapper class to handle the incomming msg
@info		in		udpinfo		contain the received packet*/
int heartBeat::recHBMsg(udpInfo &info)
{
	if (HBDEBUG)printf("enter heartBeat::recHBMsg\n");
	//printf("enter heartBeat::recHBMsg\n");
	//cout << info.msg << endl;
	int numbyte;
	string ip, port;
	int name, Hcounter;
	char alive;
	info.pos = 0;

	qFlag->mut.lock();
	if (qFlag->quit == true)
	{
		qFlag->mut.unlock();
		return 0;

	}
	qFlag->mut.unlock();
	if (MSGDROPRATE != 0)
	{
		srand(myip()[myip().length()-1] + time(NULL));
		int var = rand() % 100;
		if (var < MSGDROPRATE)
		{
			if(DEBUG)cout << "msg manually dropped" << endl;
			return 0;
		
		}
	
	
	}

	while (numbyte = processInfo(info, ip, port, name, Hcounter, alive))
	{
		
		

		if (ip.length() != 0 && port.length() != 0)
		{
			mut_HBT.lock();

			if (heartBeatTable.find(ip + port) != heartBeatTable.end())			//find same ip&port
			{
				if (heartBeatTable[ip + port].name == name)						//name in list
				{
					if ((heartBeatTable[ip + port].alive&ALIVEMASK) == ALIVEFALSE)	//already fail
					{

						mut_HBT.unlock();
						continue;

					}
					if (alive == 4)
					{
						alive = 4;			//for debug
					
					}

					if ((ip + port).compare((myip() + myport)) == 0 && name == myName)	//its me
					{
						
						
						if ((alive&ALIVEMASK) == ALIVEFALSE)
						{
							heartBeatTable[ip + port].alive |= ALIVETRUE;
							heartBeatTable[ip + port].Hcounter = 0;
							heartBeatTable[ip + port].name = (int)getCurrentTime();
							myName = heartBeatTable[ip + port].name;

							cout << "restart" << endl;
						
						
						}
						mut_HBT.unlock();
						continue;
					}

					if ((alive&ALIVEMASK) == ALIVEFALSE)							//incoming fail
					{
						//myInfo.msgType = CBFAIL;
						myInfo.msg = "client at ip: " + ip + ":" + port + "name: " + to_string(name) + "\tfail\t" + to_string(getCurrentTime())+ "\n";

						heartBeatTable[ip + port].alive &= ALIVEINPUTMASK;
						mut_HBT.unlock();
						myInfo.msgType = CBFAIL;
						myInfo.ip = ip;
						myInfo.port = port;
						//myInfo.cb(myInfo);
						for (auto & it : myInfo.cb)
							it(myInfo);
						myInfo.msg.clear();
						continue;
					
					}

					if ((ip + port).compare((myip() + myport)) == 0 && name == myName)	//its me
					{

						/*if ((alive&ALIVEINPUTMASK) == ALIVEINTRO)
						{

						heartBeatTable[ip + port].alive = 3;
						mut_HBT.unlock();
						continue;

						}
						if ((alive&ALIVEMASK) == ALIVEFALSE)
						{
						heartBeatTable[ip + port].alive |= ALIVETRUE;
						heartBeatTable[ip + port].Hcounter = 0;
						heartBeatTable[ip + port].name = (int)getCurrentTime();


						}*/
						mut_HBT.unlock();
						continue;
					}

					

					
					if (heartBeatTable[ip + port].Hcounter < Hcounter)					//update
					{
					
						heartBeatTable[ip + port].Hcounter = Hcounter;
						heartBeatTable[ip + port].time = getCurrentTime();
						//heartBeatTable[ip + port].alive &= alive | ALIVEINPUTMASK;
						myInfo.msgType = CBOTHER;
						myInfo.msg = "client at ip: " + ip + ":" + port + "name: " + to_string(name) + "\tupdated\t";
					
					}
				
				}
				else															//name is not in list
				{
					if (heartBeatTable[ip + port].name < name)					//the new name is higher
					{
						heartBeatTable[ip + port].name = name;
						heartBeatTable[ip + port].Hcounter = Hcounter;
						heartBeatTable[ip + port].time = getCurrentTime();
						heartBeatTable[ip + port].alive |= alive & ALIVEMASK;
						myInfo.msgType = CBREPLACE;
						myInfo.msg = "client at ip: " + ip + ":" + port + "name: " + to_string(name) + "\treplaced\t";
					
					}
				
				}
			
			}
			else
			{																	//new commer, adding
				if ((alive & ALIVEMASK) == ALIVEFALSE)
				{
					mut_HBT.unlock();
					continue;
				
				}
				hbtable_t newTable;
				newTable.alive = alive & ALIVEMASK;
				newTable.Hcounter = Hcounter;
				newTable.ip = ip;
				newTable.port = port;
				newTable.name = name;
				newTable.time = getCurrentTime();

				heartBeatTable.insert(pair<string, hbtable_t>(ip + port, newTable)); 
				myInfo.msgType = CBADD;
				myInfo.ip = ip;
				myInfo.port = port;
				myInfo.msg = "client at ip: " + ip + ":" + port + "name: " + to_string(name) + "\tadded\t";
			}
			
			
			mut_HBT.unlock();
			if (myInfo.msg.length() != 0)
			{
				myInfo.msg += to_string(getCurrentTime()) + "\n";
				//myInfo.cb(myInfo);
				for (auto & it : myInfo.cb)
					it(myInfo);
			}
			myInfo.msg.clear();
		}
	
	
	}
	string multicastMsg;
	if (processMulticast(info, multicastMsg) != 0)
	{
		//cout << "receive multicast msg" << endl;
		hbInfo copyinfo(myInfo);
		copyinfo.msgType = CBMULTICAST;
		copyinfo.msg = multicastMsg;
		//copyinfo.cb(copyinfo);
		for (auto & it : copyinfo.cb)
			it(copyinfo);
		copyinfo.msgType = CBOTHER;
		copyinfo.msg.clear();
	}
	

	if (HBDEBUG)printf("leave heartBeat::recHBMsg\n");
	return 0;

}

/*leave
leave the current heartBeat group
inform some of the remain node
clear all the ramaining data*/
int heartBeat::leave()
{
	if (heartBeatTable.size() == 0)
	{
	
		cout << "not in any group" << endl;
		return 0;
	}

	mut_HBT.lock();
	heartBeatTable[myip() + myport].alive &= ALIVEINPUTMASK;
	mut_HBT.unlock();
	qFlag->mut.lock();
	qFlag->quit = true;
	qFlag->mut.unlock();
	sendGossip();
	listener->quit();
	sender->quit();
	delete listener;
	delete sender;
	joinWork->join();
	joinWork->~thread();
	delete qFlag;
	heartBeatTable.clear();

}


int heartBeat::setHandler(std::function<int(hbInfo&)> cb)
{
	mut_multiMsgList.lock();
	myInfo.cb.push_back(cb);
	mut_multiMsgList.unlock();
}