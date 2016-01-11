/*Stream Processing in class Project "crane" wrote by
*Hongchuan Li and JunQing Deng in UIUC, CS425
*This file is written by Junqing Deng and modified by Hongchuan Li
*students who are taking cs425 in UIUC should not copying this file for their project
*/

#include "TMPipe.h"

//map<string, vector<int>> pipeMap;

pipeInfo::pipeInfo(int _jobID, string _identifier) {
    jobID = _jobID;
    identifier = _identifier;
}

pipeInfo & pipeInfo::operator=(const pipeInfo &rhs) {
    jobID = rhs.jobID;
    identifier = rhs.identifier;
    type = rhs.type;
    msg = rhs.msg;
}

TMPipe::TMPipe() {
}

//initiate a new pipe for both parent and child, and insert into pipeMap with identifier append with jobID as key
vector<int> TMPipe::add(int jobID, string identifier)
{
    //std::vector<int> pipefd(2,0);
	vector<int> ret;
	int pipefdsend[2];
	int pipefdrecv[2];
	if (pipe(pipefdsend) != 0 || pipe(pipefdrecv) != 0) {
        //cerr << "Failed to pipe\n";
        //exit(1);
    }
    else {
		
		ret.push_back(pipefdsend[0]);
		ret.push_back(pipefdsend[1]);
		ret.push_back(pipefdrecv[0]);
		ret.push_back(pipefdrecv[1]);
		for (auto &it : ret)
		{
			cout << it << " ";
		}
		cout << endl;

		pipeMap[identifier + std::to_string(jobID)] = ret;
        stop[identifier+std::to_string(jobID)] = 0;

    }

	
	return ret;
}

//called by the child to map the pipe to stdin/out
void TMPipe::child(vector<int> pipefd)
{
    dup2(pipefd[0], STDIN_FILENO);
    dup2(pipefd[3], STDOUT_FILENO);
    dup2(pipefd[3], STDERR_FILENO);
	close(pipefd[1]);
	close(pipefd[2]);
	close(pipefd[3]);
	close(pipefd[0]);
}

//new process to keep reading from pipe, call callback function with pipeInfo
void TMPipe::parent(std::function<int(pipeInfo)> cb, int jobID, string identifier)
{
	vector<int> pipefds = pipeMap[identifier + std::to_string(jobID)];
	vector<int> newfd;
	newfd.push_back(pipefds[2]);
	newfd.push_back(pipefds[1]);
	close(pipefds[0]);
	close(pipefds[3]);
	pipeMap[identifier + std::to_string(jobID)] = newfd;
    pipeInfo* workerPipe = new pipeInfo(jobID, identifier);
	workerReceList[identifier + std::to_string(jobID)] = (new std::thread(&TMPipe::receiveWorker,this, *workerPipe, cb));
    delete(workerPipe);
}

//remove the pipe from pipemap, stop the corresponding thread, including the pipeMap, stop flag and workerReceList
void TMPipe::remove(int jobID, string identifier)
{
	return;
	/*toshijun
	the function cause the program to crash after executing 
	workerReceList[temp]->join();
	and throw the following error
	terminate called after throwing an instance of 'std::system_error'
	  what():  Resource deadlock avoided
	Aborted (core dumped)
	*/
    std::string temp = identifier+std::to_string(jobID);
    stopLock.lock();
    stop.erase(temp);
    stop[temp] = 1;
    stopLock.unlock();
    workerReceList[temp]->join();
    workerReceList.erase(temp);
    pipeMap.erase(temp);
}
/******************
toshijun
you need to distinguish a tuple message and a cmd message

fixed in the latest version
******************/
//send a tuple to a specific process, parent to child
void TMPipe::send(int jobID, string identifier, myTuple tuple)
{
	string temp = PIPETUPLE + tuple.toString();
    temp += ";\n";
	//cout << "send tuple " << temp << endl;
	//cout << "pipe is " << pipeMap[identifier + std::to_string(jobID)][1] << endl;
	//cout << "temp.c_str() " << temp.c_str() << endl;
	//cout << "temp.length() " << temp.length() << endl;
    write(pipeMap[identifier+std::to_string(jobID)][1], temp.c_str(), temp.length());
	//cout << "finish send tuple" << endl;
}

//send a msg to a process
void TMPipe::send(int jobID, string identifier, string msg)
{
	string temp = PEPEMSG + msg;
    temp += ";\n";
    write(pipeMap[identifier+std::to_string(jobID)][1], temp.c_str(), temp.length());
}
/******************
toshijun
require new functionality
block when the buffer reach a certain point (64kb for example) until drained
this should hold true for the worker side read thread too

reply
why would we need a buffer here
Since every time a new tuple get scanned, it would be proccessed right away
******************/
//thread helper for parent when receiving new results from child worker
void TMPipe::receiveWorker(pipeInfo info, std::function<int(pipeInfo)> cb)
{
	//cout << "receiveWorker enter" << endl;
    stopLock.lock();
    int flag = stop[info.identifier+std::to_string(info.jobID)];
    stopLock.unlock();

    char buf[520];
    memset(buf, 0, 520);
    string reader = "";
    
    while (!flag) {
		//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		int tempsize = read(pipeMap[info.identifier + std::to_string(info.jobID)][0], buf, 512);
		//cout << buf << endl;
        string temp(buf, buf+tempsize);
		
        reader += temp;
        int pos = reader.find(';');
        

		if (reader.size() != 0)
		{
			
			//cout << reader << endl;

		}


        while(pos != std::string::npos){
            string to = reader.substr(0, pos);
            info.type = to[0];
            info.msg = to.substr(1);
			//cout << "received from worker" << endl;
            cb(info);
            if (pos+1 < reader.length()) {
                reader = reader.substr(pos+1);
            }
			else if (++pos == reader.size())
			{
				reader = string();
				break;
			}
            pos = reader.find(';');
        }
        
        stopLock.lock();
        flag = stop[info.identifier+std::to_string(info.jobID)];
        stopLock.unlock();

        memset(buf, 0, 520);
    }

}

