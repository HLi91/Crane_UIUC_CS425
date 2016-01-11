/*Stream Processing in class Project "crane" wrote by
 *Hongchuan Li and JunQing Deng in UIUC, CS425
 *This file is written by Hongchuan Li*/

#ifndef _INODE_H_
#define _INODE_H_

#include <string>

#include "workerPipe.h"
#include "tuple.h"

using namespace std;

/*iNode
 *		the base class for all user object, should not be used directly
 *		user should inherent from ibolt or ispout
 */
class iNode
{
protected:
	ioCollector* _icollector;
public:
	virtual void _prepare(ioCollector* collec) = 0;
	virtual int execute() = 0;
	virtual void _close() = 0;
	virtual iNode* _clone()  = 0;

};

class ibolt : public iNode
{
	

public:
	ibolt();
	virtual int execute();
	virtual void _prepare(ioCollector* collec);
	virtual void _close();
	virtual ibolt* _clone();
	virtual ibolt* clone();
	virtual void close() { return; };//do not delete ioCollector
	virtual void execute(myTuple tuple) { return; };
	virtual void prepare(ioCollector *Collector) { return; };


};

class ispout : public iNode
{
public:

	virtual int execute();
	virtual void _prepare(ioCollector* collec);
	virtual void _close();
	virtual ispout* _clone();
	virtual ispout* clone();
	virtual void nextTuple(){ return; };
	virtual void prepare(ioCollector *Collector) { return; };
	virtual void close() { return; };	//do not delete ioCollector


};

#endif
