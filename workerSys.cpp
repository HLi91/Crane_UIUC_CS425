/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Hongchuan Li
*students who are taking cs425 in UIUC should not copying this file for their project
*/

#include "workerSys.h"


workerSys::workerSys()
{
	node = NULL;
}


workerSys::workerSys( workerTopologyBuilder & _builder)
{
	init(_builder);
}
/* init
 * initiate the worker system
 * @param	_builder		workerTopologyBuilder		contains the topology information
 */
void workerSys::init(workerTopologyBuilder & _builder)
{
	builder = _builder;
	collector = new ioCollector();
}

/* execute
 * hand the control to the system, should be called by the user at the end of the main function
 * the execute function will not return
 * @param	argc	int		same argc passed to the main function
 * @param	argv	char*	same argv passed to the main function
 */
void workerSys::execute(int argc, char *argv[])
{

	if (argc == 3)
	{
		builder.setFileName(argv[2]);

		collector->emit(builder.toString());
		return;

	}
	else if (argc == 4)
	{
		collector->setJobID(stoi(argv[1]));
		collector->setIdentifier(argv[3]);
		WInstance_t inst = builder[argv[3]];
		if (inst.node == NULL)
			return;
		node = inst.node;
		process();
	}
	else if (argc == 5)
	{
		collector->setJobID(stoi(argv[1]));
		collector->setIdentifier(argv[3]);
		WInstance_t inst = builder[argv[3]];
		if (inst.node == NULL)
			return;
		node = inst.node;
		process(stoi(argv[4]));
	}


	/*
	if (argc == 2)
	{
		if (!builder.exist(argv[1]))
		{
			builder.setFileName(argv[1]);
			
			collector->emit(builder.toString());
			return;
		}
		WInstance_t inst = builder[argv[1]];
		if (inst.node == NULL)
			return;
		node = inst.node;
		process();
	}
	else
	{
		collector->emit( builder.toString());
	}*/
}

/* process
 * start the worker in single thread mode
 */
void workerSys::process()
{
	node->_prepare(collector);
	while (1)
	{
		node->execute();	
	}
	node->_close();
}
/* process
 * start the worker in multi thread mode
 * @param	thread		int		the number of thread 
 */
void workerSys::process(int thread)
{
	node->_prepare(collector);
	for (int i = 0; i < thread; i++)
	{
		std::thread * curThread = new std::thread(&workerSys::threadProcessHelper, this);
		threadList.push_back(curThread);
	}
	for (;;) pause();
}


/* threadProcessHelper
 * helper function for multi-thread
 */
void workerSys::threadProcessHelper()
{
	iNode* curNode = node->_clone();
	while (1)
	{
		//cout << "threadProcessHelper" << endl;
		
		curNode->execute();
		
	}
	curNode->_close();

}