/*Stream Processing in class Project "crane" wrote by
 *Hongchuan Li and JunQing Deng in UIUC, CS425
 *This file is written by Hongchuan Li
 *students who are taking cs425 in UIUC should not copying this file for their project
 */
#include "inode.h"



ibolt::ibolt()
{
	
}

/* execute
 * and interface method
 * user should not overwrite this function, instead use
 * void ibolt::execute(myTuple tuple)
 */
int ibolt::execute()
{

	int avail;
	execute(_icollector->getNextTuple(avail));
	return avail;
}


/* _prepare
* and interface method
* user should not overwrite this function, instead use
* void ibolt::prepare(ioCollector & collector)
*/

void ibolt::_prepare(ioCollector* collec)
{
	_icollector = collec;
	_icollector->start();
	prepare(_icollector);
}

/*_close
* and interface method
* user should not overwrite this function, instead use
* void ibolt::close() (however this method will not be used(called) in the current version)
*/
void ibolt::_close()
{
	_icollector->close();
	close();
}


/*_clone
* and interface method
* user should not overwrite this function, instead use
* ibolt* ibolt::clone()
*/
ibolt* ibolt::_clone()
{
	ibolt* ret = clone();
	ret->_icollector = this->_icollector;
	return ret;
}

/*clone
 * user are required to overwrite this fuction for their own class in order to achieve multi-thread
 */
ibolt* ibolt::clone()
{
	ibolt* ret = new ibolt();
	ret->_icollector = this->_icollector;
	return ret;
}




/* execute
* and interface method
* user should overwrite this function
*/

int ispout::execute()
{
	nextTuple();
}


/* _prepare
* and interface method
* user should not overwrite this function, instead use
* void ispout::prepare(ioCollector & collector)
*/
void ispout::_prepare(ioCollector* collec)
{
	_icollector = collec;
	prepare(_icollector);
}

/*_close
* and interface method
* user should not overwrite this function, instead use
* void ispout::close() (however this method will not be used(called) in the current version)
*/
void ispout::_close()
{
	close();
}


/*_clone
* and interface method
* user should not overwrite this function, instead use
* ibolt* ispout::clone()
*/
ispout* ispout::_clone()
{
	ispout* ret = clone();
	ret->_icollector = this->_icollector;
	return ret;
}


/*clone
* user are required to overwrite this fuction for their own class in order to achieve multi-thread
*/
ispout* ispout::clone()
{
	ispout * ret = new ispout();
	ret->_icollector = this->_icollector;
	return ret;
}