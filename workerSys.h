/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Hongchuan Li
*students who are taking cs425 in UIUC should not copying this file for their project
*/

#ifndef _WORKERSYS_H_
#define _WORKERSYS_H_

#include <thread>
#include <unistd.h>
#include "workerTopologyBuilder.h"
#include "tuple.h"
#include "workerPipe.h"
#include "inode.h"

class workerSys
{
	iNode * node;
	workerTopologyBuilder builder;
	ioCollector* collector;

	vector<std::thread *> threadList;

public:
	workerSys();
	workerSys(workerTopologyBuilder & _builder);
	void init(workerTopologyBuilder & _builder);
	void execute(int argc, char *argv[]);

private:
	void process();
	void process(int thread);
	void threadProcessHelper();
};

#endif