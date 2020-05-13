#include<iostream>
#include<fstream>
#include<stdlib.h>

#define CacheSize 256 
#define BlockSize 8
#define BlockOffset 3
#define BlockNum 32
#define logBlockNum 5
#define ReplaceWay 1  //relpace way=0表示 LRU ，replace way=1表示FIFO
using namespace std;

typedef unsigned int ui;

struct simCache
{
	char dirty = 0;
	char valid = 0;
	ui time = 0;		//刻画时间，就是每次读取的时间，用每次读取的次数来表示，比如第一次读取time=0...
	ui frq = 0;			//刻画频率，每次读取frq++；替换置零时再=1；
	ui tag = 0;
};


void iCache(ui, ui, string, string);
void cache_read(simCache* c, ui, ui, int&, int);
void cache_write(simCache* c, ui, ui, int&, int);
int cache_REPLACE_LRU(simCache* c, ui);				//替换最久未被读取的
int cache_REPLACE_FIFO(simCache*, ui);				//替换频率最低的

int main()
{
	string fout_name = "result_FIFO_ikj.txt";
	string fin_name = "Data_ikj.out";

	cout << "Now begin to test:" << endl;
	fstream ff;
	ff.open(fout_name.c_str(), ios::out);
	if (!ff.is_open())
	{
		cout << "Can't open the file" << endl;
		exit(-1);
	}
	ff << "CacheSize :" << CacheSize << "B ；" << "BlockSize: " << BlockSize << "Byte." << '\n';
	ff.close();
	int j = 0;
	for (ui i = 1; i <= BlockNum; i = i * 2)
	{
		cout << "The " << j + 1 << " test" << endl;
		iCache(i, j, fout_name, fin_name);
		j++;
	}
	cout << "Over" << endl;
}


void iCache(ui setSize, ui lable, string fout_name, string fin_name)
{
	//组数等于总块数/每块的大小
	ui setNum = BlockNum / setSize;

	//用来帮助后面获取tag位
	ui logsetNum = logBlockNum - lable;
	cout << "logsetNum==" << logsetNum << endl;
	cout << "setNum==" << setNum << endl;
	//cache的初始化，一个二维数组
	simCache** cache = NULL;
	cache = new simCache * [setNum];
	for (int i = 0; i < setNum; i++)
	{
		cache[i] = new simCache[setSize];
	}

	fstream fout;
	fout.open(fout_name.c_str(), ios::app);
	if (!fout.is_open())
	{
		cout << "Can't open the file !" << endl;
		exit(-1);
	}

	fstream fin;
	fin.open(fin_name.c_str());
	if (!fin.is_open())
	{
		cout << "Can't open the file !" << endl;
		exit(-1);
	}

	int hit_num = 0;
	double hit_rate = 0;
	int total = 0;

	int read_num = 0;
	int write_num = 0;

	int read_hit = 0;
	int write_hit = 0;

	double read_hit_rate = 0;
	double write_hit_rate = 0;

	//用于读文件
	char info[20];
	char* add;
	char* str;
	ui address = 0;
	char type = 0;

	ui set = 0;
	ui tag = 0;
	ui getTag = BlockOffset + logsetNum;
	while (fin.getline(info, 20))
	{
		//每次读文件，总次数+1
		total++;
		//读取文件中的访存信息，是写还是读，以及获得地址
		type = info[0];
		add = info + 2;
		address = strtol(add, &str, 16);

		set = (address >> BlockOffset) & (setNum - 1);
		tag = address >> getTag;

		if (type == 'r')
		{
			read_num++;
			cache_read(cache[set], setSize, tag, read_hit, total);
		}
		else
		{
			write_num++;
			cache_write(cache[set], setSize, tag, write_hit, total);
		}
	}
	fout << " --------------THE " << lable + 1 << " test-------------- \n";
	fout << "There is " << setNum << " sets, " << "and every set has " << setSize << " cache blocks\n";
	fout << "Read_Hit_Rate " << '\t' << "Write_Hit_Rate" << "\t" << "Hit_Rate\t" << "\n";

	hit_rate = (double)(read_hit + write_hit) / total;

	read_hit_rate = (double)read_hit / read_num;

	write_hit_rate = (double)write_hit / write_num;


	fout << read_hit_rate << "\t" << "\t" << write_hit_rate << "\t" << "\t" << hit_rate << '\n';
	fout << endl;
	for (int i = 0; i < setNum; i++)
	{
		delete[]cache[i];
	}
	delete[]cache;
	fout.close();
	fin.close();


}

void cache_read(simCache* cache, ui setSize, ui tag, int& read_hit, int times)
{
	int i = 0;
	for (i; i < setSize; i++)
	{
		if (cache[i].valid == 0)
		{
			cache[i].valid = 1;
			cache[i].tag = tag;
			cache[i].frq = 1;
			cache[i].time = times;
			return;
		}
		if (cache[i].valid == 1 && cache[i].tag == tag)
		{
			cache[i].frq++;
			cache[i].time = times;
			read_hit++;
			return;
		}

	}
	int replace;

#if ReplaceWay==0
	replace = cache_REPLACE_LRU(cache, setSize);
#endif // 0
#if ReplaceWay==1
	replace = cache_REPLACE_FIFO(cache, setSize);
#endif // ReplaceWay==1


	cache[replace].valid = 1;
	cache[replace].frq = 1;
	cache[replace].tag = tag;
	cache[replace].time = times;
	cache[replace].dirty = 0;

}

void cache_write(simCache* cache, ui setSize, ui tag, int& write_hit, int times)
{
	int i = 0;
	for (i = 0; i < setSize; i++)
	{
		if (cache[i].valid == 0)
		{
			cache[i].valid = 1;
			cache[i].dirty = 1;
			cache[i].tag = tag;
			cache[i].frq = 1;
			cache[i].time = times;
			return;
		}
		if (cache[i].valid == 1 && cache[i].tag == tag)
		{
			cache[i].dirty = 1;
			cache[i].frq++;
			cache[i].time = times;
			write_hit++;
			return;
		}
	}

	int replace;
#if ReplaceWay==0
	replace = cache_REPLACE_LRU(cache, setSize);
#endif // 0

#if ReplaceWay==1
	replace = cache_REPLACE_FIFO(cache, setSize);
#endif // ReplaceWay==1

	cache[replace].valid = 1;
	cache[replace].frq = 1;
	cache[replace].tag = tag;
	cache[replace].time = times;
	cache[replace].dirty = 1;

}

//时间替换算法
int cache_REPLACE_LRU(simCache* cache, ui setSize)
{
	int i = 0;
	ui minTime = cache[0].time;
	int num = 0;
	for (i = 0; i < setSize; i++)
	{
		if (cache[i].valid == 0)
		{
			return i;
		}
		if (cache[i].time < minTime)
		{
			num = i;
			minTime = cache[i].time;
		}
	}
	return num;
}

//频率替换算法
int cache_REPLACE_FIFO(simCache* cache, ui setSize)
{
	int i = 0;
	ui minFrq = cache[0].frq;
	int num = 0;
	for (i = 0; i < setSize; i++)
	{
		if (cache[i].valid == 0)
		{
			return i;
		}
		if (cache[i].frq < minFrq)
		{
			num = i;
			minFrq = cache[i].frq;
		}
	}
	return num;
}
