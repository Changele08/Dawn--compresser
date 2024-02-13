#include <bits/stdc++.h>
#define QWORD unsigned long long
#define DWORD unsigned
using namespace std;

extern short dawncompress(char *inf, char *of, DWORD windowsz);
extern short dawndecompress(char *inf,char *outf);
extern QWORD getfilesize(const char *file);

DWORD c_str2int(const char *a)
{
	DWORD ret = 0, len = strlen(a);
	for (int i = 0; i < len; i++)
	  ret = ret * 10 + a[i] - '0';
	return ret;
}

int main(int argc,char *argv[])//dawncompress.exe a.in c a.out
{
	cout<<"****************************************"<<endl;
    cout<<"*dawncompress V10 CopyRight(c) by ChZL *"<<endl;
    cout<<"*    Production time: August 27, 2023  *"<<endl;
    cout<<"* 软件版本：V10.0, 文件版本：V1（试行）*"<<endl; 
    cout<<"****************************************"<<endl;
    
    if (argc < 4)
    {
    	cout << "使用方法：" << endl;
    	cout << "dawn.exe infile [c|d] outfile (windowsize)" << endl;
    	cout << "c:压缩infile输出为outfile" << endl;
    	cout << "d:解压缩infile输出为outfile" << endl;
    	cout << "windowsize：设置字典长度，数值越大速度越慢，压缩后的文件越小" << endl;
		return 0;
	}
    
	if (argv[2][0] == 'c')
	{
		DWORD windowsz;
		if (argc < 5)
		  windowsz = getfilesize(argv[1]);
		else
		  windowsz = c_str2int(argv[4]);
		
		dawncompress(argv[1], argv[3], windowsz);
	}
	else
	  dawndecompress(argv[1], argv[3]);
	return 0;
} 
