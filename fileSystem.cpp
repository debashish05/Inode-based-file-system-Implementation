#include <iostream>
#include <cstring>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>

using namespace std;
map<string, int> fileToinode; //file Name to inode number
map<int, string> inodeTofile;
set<string> fileRead, fileWrite, fileAppend;
string diskName = "";
#define BLOCK_SIZE 100000 // 100KB

// ************************* fileSystem.h *************************

// 500 *(10**6) / 100000 = 5000 blocks will be available
// 100 kb block size * 300  MAX FILE SIZE

struct inode
{
	// 4*300 + 8 + 42 = 1250B
	int blocksPtr[300]; //300 blocks ptr it can accomdoate so
	int offset;			//will see it later
	char fileName[38];
	int currindex;
	int iNodeNum;
};

struct superblock // It has size of 13 blocks approx
{
	char message[42]; //any extra message can be stored here
	int numOfinodes;  //1000
	int numOfblocks;  //4986 (super block)

	inode Inode[1000]; // it can support upto 500 files. 1 inode for one file
};

struct dataBlock // 4987 data blocks
{
	bool avil;
	char data[BLOCK_SIZE - 1];
};

superblock sb;
dataBlock *db; // pointer to the datablocks

// ************************* End *************************

bool filePresent(string fileName)
{
	if (FILE *file = fopen(fileName.c_str(), "r"))
	{
		fclose(file);
		return true;
	}
	return false;
}

void initializeFS()
{
	// This will be called the first time when new disk is created.
	// Rest we need to read from FS

	sb.numOfinodes = 1000; //	#inodes
	sb.numOfblocks = 4980; //  #data blocks
	strcpy(sb.message, "Hello this is a basic File system");
	for (int i = 0; i < sb.numOfinodes; ++i)
	{
		for (int j = 0; j < 300; ++j)
			sb.Inode[i].blocksPtr[j] = -1;

		sb.Inode[i].iNodeNum = -1;
		sb.Inode[i].currindex = 0;
		sb.Inode[i].offset = -1;
		strcpy(sb.Inode[i].fileName, "");
	}

	db = (dataBlock *)malloc(sizeof(dataBlock) * sb.numOfblocks);

	for (int i = 0; i < sb.numOfblocks; ++i)
	{
		db[i].avil = true;
	}
}

void sync(string fileName = "")
{
	FILE *file;
	if (fileName == "")
		fileName = diskName;

	file = fopen(fileName.c_str(), "w+");

	fwrite(&sb, sizeof(superblock), 1, file); // super Node
	for (int i = 0; i < sb.numOfblocks; ++i)
	{
		fwrite(&db[i], sizeof(dataBlock), 1, file);
	}
	fclose(file);
}

void mount(string fileName)
{
	// load data from file to structure
	FILE *file = fopen(fileName.c_str(), "r");
	diskName = fileName;
	fread(&sb, sizeof(superblock), 1, file); // super Node

	// extract file names
	fileToinode.clear();
	fileAppend.clear();
	fileWrite.clear();
	fileRead.clear();
	inodeTofile.clear();

	for (int i = 0; i < sb.numOfinodes; ++i)
	{
		string name = sb.Inode[i].fileName;
		if (name != "" && sb.Inode[i].iNodeNum != -1 && sb.Inode[i].offset != -1)
		{
			fileToinode[name] = i;
			inodeTofile[i] = name;
		}
	}

	for (int i = 0; i < sb.numOfblocks; ++i)
	{
		fread(&db[i], sizeof(dataBlock), 1, file); // 1 means write byte by byte
	}
	fclose(file);
}

void printfs() //print stucture of file systems
{
	cout << "superblock: \n";
	cout << sb.numOfinodes << endl; //	#inodes
	cout << sb.numOfblocks << endl; //  #data blocks
	cout << sb.message << endl;

	for (int i = 0; i < sb.numOfinodes; ++i)
	{
		for (int j = 0; j < 300; ++j)
			cout << sb.Inode[i].blocksPtr[j] << " ";
		cout << sb.Inode[i].offset << " ";
		cout << sb.Inode[i].fileName << endl;
	}
	cout << "Data block: \n";

	for (int i = 0; i < sb.numOfblocks; ++i)
	{
		cout << db[i].avil << " ";
	}
}

int findEmptyDataNode() //find empty data block
{
	int i = 0;
	for (i = 0; i < sb.numOfblocks; ++i)
	{
		if (db[i].avil)
		{
			db[i].avil = false; //caputiring the data block
			break;
		}
	}
	if (i == sb.numOfblocks)
		i = -1;
	return i;
}

int findEmptyiNode() // find empty inode
{
	int i = 0;
	for (i = 0; i < sb.numOfinodes; ++i)
	{
		if (sb.Inode[i].iNodeNum == -1)
		{
			sb.Inode[i].iNodeNum = i; //capturing the block
			break;
		}
	}
	if (i == sb.numOfinodes)
		i = -1;
	return i;
}

int allocateFile(string fileName)
{
	//allocate file, first find empty inode, claim it, then find and claim block
	// return file discriptor
	long inode = findEmptyiNode();
	long dblock = findEmptyDataNode();

	if (inode == -1)
		return -1; //inode full max limit reached

	sb.Inode[inode].blocksPtr[0] = dblock; // firs block allocated
	sb.Inode[inode].offset = 0;
	strcpy(sb.Inode[inode].fileName, fileName.c_str());

	return inode;
}

int filesystem(string fileName)
{
	int choice;
	//printfs();
	while (true)
	{
		cout << "\n***************************\n";
		cout << "1: Create File\n";
		cout << "2: Open File\n";
		cout << "3: Read File\n";
		cout << "4: Write File\n";
		cout << "5: Append File\n";
		cout << "6: Close File\n";
		cout << "7: Delete File\n";
		cout << "8: List of Files\n";
		cout << "9: List of Opened Files\n";
		cout << "10: Unmount\n";
		cout << "***************************\n";

		cin >> choice;
		if (choice == 1)
		{
			string fileName;
			cout << "Enter File Name :\n";
			cin.ignore();
			getline(cin, fileName);
			if (fileToinode.find(fileName) != fileToinode.end())
			{
				cout << "file name already exists try different file Name" << endl;
				continue;
			}
			int descrip = allocateFile(fileName);
			fileToinode[fileName] = descrip;
			inodeTofile[descrip] = fileName;
			cout << "File Node created Successfully with ";
			cout << " Inode Number: " << fileToinode[fileName] << endl;
			//sync();
		}
		else if (choice == 2)
		{
			string fileName;
			cout << "Enter File Name to Open: \n";
			cin.ignore();
			getline(cin, fileName);
			int mode;
			cout << "Choose Mode\n";
			cout << "0: read mode\n";
			cout << "1: write mode\n";
			cout << "2: append mode\n";
			cin >> mode;
			if (fileToinode.find(fileName) == fileToinode.end())
			{
				cout << "Files Doesn't exists\n";
				continue;
			}
			//file exists
			if (mode == 0)
			{
				//fileRead.push_back(fileName);
				fileRead.insert(fileName);
			}
			else if (mode == 1)
			{
				fileWrite.insert(fileName);
			}
			else if (mode == 2)
			{
				fileAppend.insert(fileName);
			}
			else
			{
				cout << "Wrong mode entered\n";
				continue;
			}
			cout << "File " << fileName << " Opened With File descriptor: ";
			cout << fileToinode[fileName] << endl;

			inodeTofile[fileToinode[fileName]] = fileName;
		}
		else if (choice == 3)
		{
			int descrip;
			cout << "Enter File descriptor to READ from : ";
			cin >> descrip;

			if (inodeTofile.find(descrip) == inodeTofile.end())
			{
				cout << "No descriptor is avaiable corresponding to this file\n";
				continue;
			}
			string fileName = inodeTofile[descrip];
			auto it = find(fileRead.begin(), fileRead.end(), fileName);

			if (it == fileRead.end())
			{
				cout << "file is not opened in read mode, first open then Read\n";
				continue;
			}

			cout << "Content of the file is as follows: " << endl;

			//descrip is the inode number
			int indexLimit = sb.Inode[descrip].currindex;
			int offsetLimit = sb.Inode[descrip].offset;
			sb.Inode[descrip].iNodeNum = descrip;
			int offset = 0, ptrNum = 0, count = 0;

			while (ptrNum <= indexLimit && offset < offsetLimit)
			{
				if (offset >= BLOCK_SIZE - 1)
				{
					ptrNum++;
					offset = 0;
				}
				int index = sb.Inode[descrip].blocksPtr[ptrNum];
				cout << db[index].data[offset];
				++count;
				++offset;
			}
			cout << endl;
			cout << count << " Bytes Read Successfully.\n";
		}
		else if (choice == 4)
		{
			int descrip;
			cout << "Enter File descriptor to write: ";
			cin >> descrip;

			if (inodeTofile.find(descrip) == inodeTofile.end())
			{
				cout << "No descriptor is avaiable corresponding to this file\n";
				continue;
			}
			string fileName = inodeTofile[descrip];
			auto it = find(fileWrite.begin(), fileWrite.end(), fileName);

			if (it == fileWrite.end())
			{
				cout << "file is not opened in write mode, first open then Write\n";
				continue;
			}

			string msg;
			cout << "Enter the Content Here: (To stop write, $$ and hit ENTER)" << endl;
			// end of the write by $$
			char c;
			bool first = false;
			while ((c = getchar()) != EOF)
			{
				if (first && c == '$')
					break;
				if (first && c != '$')
					first = false;
				if (c == '$')
					first = true;
				msg += c;
			}
			msg.pop_back(); // remove extra $ at end
			//cout<<msg<<endl;

			for (int i = 1; i < sb.Inode[descrip].currindex; ++i)
			{
				// freeing rest of the datablocks
				int index = sb.Inode[descrip].blocksPtr[i];
				db[index].avil = true;
			}

			//descrip is the inode number
			sb.Inode[descrip].currindex = 0;
			sb.Inode[descrip].offset = 0;
			sb.Inode[descrip].iNodeNum = descrip;
			int offset = 0, ptrNum = 0;

			//free pointers

			for (int i = 0; i < msg.length(); ++i, ++offset)
			{
				if (offset >= BLOCK_SIZE - 1)
				{
					ptrNum++;
					offset = 0;
					int dataNode = findEmptyDataNode();
					sb.Inode[descrip].blocksPtr[ptrNum] = dataNode;
				}
				int index = sb.Inode[descrip].blocksPtr[ptrNum];
				//cout << i << " " << index << endl;
				db[index].data[offset] = msg[i];
			}

			sb.Inode[descrip].currindex = ptrNum;
			sb.Inode[descrip].offset = offset;

			cout << endl;
			cout << msg.length() << " Bytes Written Successfully.\n";
			sync();
		}
		else if (choice == 5)
		{
			int descrip;
			cout << "Enter File descriptor to Append: ";
			cin >> descrip;

			if (inodeTofile.find(descrip) == inodeTofile.end())
			{
				cout << "No descriptor is avaiable corresponding to this file\n";
				continue;
			}
			string fileName = inodeTofile[descrip];

			if (fileAppend.find(fileName) == fileAppend.end())
			{
				cout << "file is not opened in Append mode, first open then Append\n";
				continue;
			}

			string msg;
			cout << "Enter the Content Here: (To stop append, $$ and hit ENTER)" << endl;
			// end of the write by $$
			char c;
			bool first = false;
			while ((c = getchar()) != EOF)
			{
				if (first && c == '$')
					break;
				if (first && c != '$')
					first = false;
				if (c == '$')
					first = true;
				msg += c;
			}
			msg.pop_back(); // remove extra $ at end

			//descrip is the inode number

			sb.Inode[descrip].iNodeNum = descrip;
			int offset = sb.Inode[descrip].offset;
			int ptrNum = sb.Inode[descrip].currindex;

			for (int i = 0; i < msg.length(); ++i, ++offset)
			{
				if (offset >= BLOCK_SIZE - 1)
				{
					ptrNum++;
					offset = 0;
					int dataNode = findEmptyDataNode();
					sb.Inode[descrip].blocksPtr[ptrNum] = dataNode;
				}
				int index = sb.Inode[descrip].blocksPtr[ptrNum];
				db[index].data[offset] = msg[i];
			}

			sb.Inode[descrip].currindex = ptrNum;
			sb.Inode[descrip].offset = offset;

			cout << endl;
			cout << msg.length() << " Bytes Appended Successfully." << endl;
			sync();
		}
		else if (choice == 6)
		{
			int descrip;
			cout << "Enterthe file Descriptor Which you want to Close:\n";
			cin >> descrip;

			if (inodeTofile.find(descrip) == inodeTofile.end())
			{
				cout << "File doesnt created. So create it first then close\n";
				continue;
			}
			string fileName = inodeTofile[descrip];
			if (fileAppend.find(fileName) != fileAppend.end())
				fileAppend.erase(fileName);
			if (fileWrite.find(fileName) != fileWrite.end())
				fileWrite.erase(fileName);
			if (fileRead.find(fileName) != fileRead.end())
				fileRead.erase(fileName);
			cout << "\nFile has been successfully closed" << endl;
		}
		else if (choice == 7)
		{
			string fileName;
			cout << "Enter the file Name that need to be DELETED (Close it before delete)\n";
			cin.ignore();
			getline(cin, fileName);

			if (fileToinode.find(fileName) == fileToinode.end())
			{
				cout << "File doesnt created. So create it first then Delete\n";
				continue;
			}

			int descript = fileToinode[fileName];
			inodeTofile.erase(descript);
			fileToinode.erase(fileName);

			for (int i = 0; i <= sb.Inode[descript].currindex; ++i)
			{
				// freeing rest of the datablocks
				int index = sb.Inode[descript].blocksPtr[i];
				db[index].avil = true;
			}

			//descrip is the inode number
			sb.Inode[descript].currindex = 0;
			sb.Inode[descript].offset = -1;
			sb.Inode[descript].iNodeNum = -1;
			strcpy(sb.Inode[descript].fileName, "");

			for (int j = 0; j < 300; ++j)
				sb.Inode[descript].blocksPtr[j] = -1;

			cout << fileName << " DELETED successfully!!!" << endl;
			//sync();
		}
		else if (choice == 8)
		{
			cout << "File Names are as follows:" << endl;
			for (pair<string, int> p : fileToinode)
				cout << p.first << " with inode : " << p.second << endl;
		}
		else if (choice == 9)
		{
			cout << "File Names Which are Open, Descriptor and Modes are as follows:" << endl;
			for (pair<string, int> p : fileToinode)
			{
				bool Read = (fileRead.find(p.first) != fileRead.end());
				bool Write = (fileWrite.find(p.first) != fileWrite.end());
				bool Append = (fileAppend.find(p.first) != fileAppend.end());
				if (Read || Write || Append)
					cout << p.first << " with inode : " << p.second;
				if (Read)
					cout << "\tREAD";
				if (Write)
					cout << "  WRITE";
				if (Append)
					cout << "  APPEND";
				cout << endl;
			}
			cout << endl;
		}
		else if (choice == 10)
		{
			sync();
			initializeFS();
			diskName = "";
			break;
		}
	}
	return 0;
}

int main()
{
	// Driver Code
	initializeFS();
	while (true)
	{
		int choice;
		cout << "***************************\n";
		cout << "1: Create Disk\n";
		cout << "2: Mount Disk\n";
		cout << "3: Exit\n";
		cout << "***************************\n";

		cin >> choice;

		if (choice == 3)
			break;

		if (choice == 1)
		{
			string fileName;
			cout << "Enter Diskname :\n";
			cin.ignore();
			getline(cin, fileName); //If getline is used after cin >>,
			//the getline() sees this newline character as leading whitespace
			//and it just stops reading any further.
			//cout<<fileName<<"\n";

			if (filePresent(fileName))
			{
				cout << "Try another name this name exists\n";
				continue;
			}

			sync(fileName);
			cout << "Virtual Disk Created!!!\n";
		}
		else if (choice == 2)
		{
			string fileName;
			cout << "Enter Diskname :\n";
			cin.ignore();
			getline(cin, fileName);

			if (!filePresent(fileName))
			{
				cout << "Disk doesn't exists. First create it then mount it.\n";
				continue;
			}
			// Load file system from file
			mount(fileName);
			cout << fileName << " is Mounted !!!\n";
			filesystem(fileName);
		}
		else
		{
			cout << "Enter Valid choices\n";
		}
	}
	cout << "Thanks for visiting\n";
	return 0;
}