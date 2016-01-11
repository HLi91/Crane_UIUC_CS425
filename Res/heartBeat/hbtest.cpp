#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string>
#include <iostream>

#include <fstream>
#include <streambuf>
#include <mutex>

#include "heartBeat.h"



using namespace std;

int callback(hbInfo &info)
{
	if ((info.msgType & (CBCHANGEMASK | CBDELETEMASK)) == 0)
		return 0;
	cout << "Received: \n" << info.msg << endl;

	std::ofstream out("output.txt", std::ofstream::out | std::ofstream::app);
	out << info.msg;
	out.close();

}

int main(int argc, char *argv[])
{
	heartBeat * hb = new heartBeat();
	hb->join(argv[1], argv[2], argv[3], callback);
	while (1)
	{
		string cmd;
		cin >> cmd;
		if (cmd.compare("quit") == 0)
		{
		
			hb->leave();
			
			//return 0;
		
		}

		if (cmd.compare("ls") == 0)
		{
			map < string, hbtable_t > list = hb->getTable();
			cout <<"IP\t\t:port\t\tName\t\tAlive" << endl;
			for (auto it : list)
			{
				cout << it.second.ip << "\t:" << it.second.port << "\t\t" << it.second.name << "\t";
				if ((it.second.alive & ALIVEMASK) == true)
				{
					cout << "TRUE" << endl;
				}
				else cout << "False" << endl;
			
			}
			continue;

		}
		if (cmd.compare("me") == 0)
		{
			hbtable_t me = hb->getMe();
			cout << me.ip << "\t:" << me.port << "\t\t" << me.name << "\t";
			if ((me.alive & ALIVEMASK) == true)
			{
				cout << "TRUE" << endl;
			}
			else cout << "False" << endl;
			continue;

		}
		if (cmd.compare("join") == 0)
		{
			hb->join(argv[1], argv[2], argv[3], callback);
			continue;

		}
	
	}



}