#include "SDFS.h"

using namespace std;


int main(int argc, char *argv[])
{
	SDFS * myfilesystem = new SDFS();
	
	myfilesystem->join(argv[1], argv[2], argv[3]);

	while (1)
	{
		string cmd, argv1, argv2;
		cin >> cmd;
		if (cmd.compare("quit") == 0)
		{

			myfilesystem->leave();

			return 0;

		}
		
		if (cmd.compare("put") == 0)
		{
			cin >> argv1 >> argv2;
			myfilesystem->put(argv1, argv2);		
		
		}
		if (cmd.compare("lsvm") == 0)
		{
			cin >> argv1;
			cout << myfilesystem->lsVM(argv1);

		}
		if (cmd.compare("get") == 0)
		{
			cin >> argv1 >> argv2;
			myfilesystem->fetch(argv1, argv2);

		}
		if (cmd.compare("me") == 0)
		{
			//cin >> argv1;
			cout << myfilesystem->lsMyself();

		}
		if (cmd.compare("lsmem") == 0)
		{
			//cin >> argv1;
			cout << myfilesystem->lsMember();

		}
		if (cmd.compare("store") == 0)
		{
			//cin >> argv1;
			cout << myfilesystem->store();

		}
		if (cmd.compare("del") == 0)
		{
			cin >> argv1;
			cout << myfilesystem->deleteFile(argv1);

		}

	}


}