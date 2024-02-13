/*
*	名称：		Dawn for Dawn_Charlotte_OS 文件压缩工具
*	版权归属：  ChZL
*	制作者：    ChZL
*	日期：		2024-02-13 22:08
*	简介：		为了让 Dawn_OS 的软盘不那么拥挤，需要一个有效
*				的压缩算法。可惜原先的 dawn 解压时速度太慢（时间复杂度不
*				到 O(n)） 所以打算重新开发一个解压速度 fast enough 的算法。
*				于是，这个版本的 dawn 就被制作了出来。
*
*/
#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <cmath>
#include <windows.h>
#include <cstring>
#include <fstream>

#define QWORD unsigned long long 
#define DWORD unsigned
#define WORD unsigned short
#define BYTE unsigned char

#define UNKNOWERROR 2//ops
#define INFILENOTFOUND 3
#define OK 5
#define COMPRESSFILEBREAK 7
#define FILEBREAK 7
#define MB 1000000
#define returnif(a,ret) if((a)) return ret
#define sethighest(a,set) ((a)=(((a)&0x7f)|(set)<<7)) 
#define gethighest(a) ((a)&0x80)
#define init() lastdis[0] = 0;\
	lastdis[1] = 1;\
	lastdis[2] = 2;\
	lastdis[3] = 3;\
	for (int i = 0; i < 256; i++)\
	  lastlen[i] = i

using namespace std;

BYTE bitput = 0;
DWORD bitp = 0, lastbitp = 0, lastdis[4] = { 0,1,2,3 }, lastlen[256];// lastlen & lastdis: MTF 算法 
vector<BYTE> outbuf;

QWORD r7 = pow(2, 7), r14 = pow(2, 14), r21 = pow(2, 21), r28 = pow(2, 28), r35 = pow(2, 35), r42 = pow(2, 42);
QWORD R7 = r7, R14 = r7 + r14, R21 = r7 + r14 + r21, R28 = r7 + r14 + r21 + r28, R35 = r7 + r14 + r21 + r28 + r35, R42 = r7 + r14 + r21 + r28 + r35 + r42;
QWORD r[7] = { 1,r7,r14,r21,r28,r35,r42 };
QWORD R[10] = { 0,R7,R14,R21,R28,R35,R42 };

int int2vlq(QWORD num, unsigned char *vlq)//返回vlq的size 
{
	QWORD facevl, sz = 0;
	for (int i = 0; i < 7; i++)
		if (num < R[i])
		{
			facevl = num - R[i - 1];
			sz = i;
			break;
		}
	memset(vlq, 0, sz);
	for (int i = 0; i < sz; i++)
	{
		for (int j = 0; j < 7; j++)
		{
			vlq[i] += (facevl % 2) << j;
			facevl /= 2;
		}
		sethighest(vlq[i], 1);//全部置一 
	}
	sethighest(vlq[sz - 1], 0);//最后一位置零
	return sz;
}

QWORD vlq2int(unsigned char *vlq)
{
	QWORD size = 0, facevl = 0;
	for (; gethighest(vlq[size]); size++);
	for (int i = size; i >= 0; i--)
		facevl += (vlq[i] & 0x7f) << (i * 7);
	return facevl + R[size];
}

QWORD freadvlq(FILE *rd)//返回读取到的参数 
{
	BYTE buf[8], p = 0;
	memset(buf, 0, 8);
	BYTE ch;
	if (!fread(&ch, 1, 1, rd))
		return -1;
	buf[p++] = ch;
	while (gethighest(ch))
	{
		if (!fread(&ch, 1, 1, rd))
			return -1;
		buf[p++] = ch;
	}
	return vlq2int(buf);
}

QWORD fwritevlq(QWORD num, FILE *wt)//返回输出了几位 
{
	BYTE buf[8];
	int sz = int2vlq(num, buf);
	fwrite(buf, 1, sz, wt);
	return sz;
}

QWORD getfilesize(const char *file)
{
	QWORD ret = 0;
	ifstream fin(file, ifstream::ate | ifstream::binary);
	ret = fin.tellg();
	fin.close();
	return ret;
}

// 读入指定数目的 bit
inline DWORD bitread(BYTE *buf, DWORD &bufp, BYTE &read, DWORD &bitp, DWORD needbit)//读取bit
{
	DWORD ret = 0, ti = needbit;
	while (needbit)
	{
		ret += (read & 1) << (ti - needbit);
		read >>= 1;
		if (++bitp == 8)
		{
			read = buf[bufp++];
			bitp = 0;
		}
		needbit--;
	}
	return ret;
}

// 输出指定数目个 bit 
inline void bitwrite(vector<BYTE> &buf, BYTE &read, DWORD &bitp, DWORD needbit, DWORD write)
{
	DWORD ret = 0, ti = needbit;
	while (needbit)
	{
		read |= (write & 1) << (bitp++);
		write >>= 1;
		if (bitp == 8)
		{
			buf.push_back(read);
			read = bitp = 0;
		}
		needbit--;
	}
}

//输出一个信息块
inline void outputablock(DWORD distance, WORD length, DWORD nds)
{
	if (distance == 0)//length中存放着literal
	{
		bitwrite(outbuf, bitput, bitp, 1, 0);
		bitwrite(outbuf, bitput, bitp, 8, length);
	}
	else//length中是len 
	{
		length -= 3;
		distance--;
		if (lastdis[0] == distance)
			distance = 0;
		else
		{
			if (lastdis[1] == distance)
			{
				lastdis[1] = lastdis[0];
				lastdis[0] = distance;
				distance = 1;
			}
			else
			{
				if (lastdis[2] == distance)
				{
					lastdis[2] = lastdis[1];
					lastdis[1] = lastdis[0];
					lastdis[0] = distance;
					distance = 2;
				}
				else
				{
					if (lastdis[3] == distance)
					{
						lastdis[3] = lastdis[2];
						lastdis[2] = lastdis[1];
						lastdis[1] = lastdis[0];
						lastdis[0] = distance;
						distance = 3;
					}
					else
					{
						lastdis[3] = lastdis[2];
						lastdis[2] = lastdis[1];
						lastdis[1] = lastdis[0];
						lastdis[0] = distance;
						distance += 4;
					}
				}
			}
		}

		int tlength = lastlen[length];
		for (int i = 0; i < 256; i++)
			if (lastlen[i] < tlength)
				lastlen[i]++;
		lastlen[length] = 0;
		length = tlength;

		// 对distance进行分类讨论，分为：
		// 1、0        --   1 << 6 - 1
		// 2、1 << 6   --   1 << 11 - 1
		// 3、1 << 11  --   1 << ceil(log2(nds + 4)

		bitwrite(outbuf, bitput, bitp, 1, 1);
		if (distance < (1 << 6) - 1)
		{
			bitwrite(outbuf, bitput, bitp, 1, 0);
			bitwrite(outbuf, bitput, bitp, 6, distance);
		}
		else
		{
			bitwrite(outbuf, bitput, bitp, 1, 1);
			bitwrite(outbuf, bitput, bitp, 1, (distance < (1 << 11) - 1 && ceil(log2(nds + 4) > 11)) ? 0 : 1);
			bitwrite(outbuf, bitput, bitp, (distance < (1 << 11) - 1 && ceil(log2(nds + 4) > 11)) ? 11 : ceil(log2(nds + 4)), distance);
		}

		//length就不必分类那么多了。人家最高不过才8bit多一点点，你再切，越切越大。
		//这里只对它出现频率最高的部分分类。通常length < 16，故而分为一类。 
		bitwrite(outbuf, bitput, bitp, 1, (length < 8) ? 0 : 1);
		bitwrite(outbuf, bitput, bitp, (length < 8) ? 3 : 8, length);
	}
}

//获取输出一个 DL 对的大小
/*
这个函数在longest匹配中会被多次调用，所以这个函数的时间复杂度将直接、严重的影响到最后压缩的速度。
使用inline，期待这个函数的时间复杂度能稍微低一点。。。
*/
inline short getoutputsize(DWORD distance, DWORD length, DWORD nds)
{
	returnif(length < 3, 0xfff);
	length -= 3;
	distance--;

	length = lastlen[length];

	distance = (lastdis[0] == distance) ? 0 : (lastdis[1] == distance) ? 1 : (lastdis[2] == distance) ? 2 : (lastdis[3] == distance) ? 3 : distance + 4;

	DWORD sz = (distance < (1 << 6) - 1) ? 7 : (distance < (1 << 11) - 1 && ceil(log2(nds + 4) > 11)) ? 13 : 2 + ceil(log2(nds + 4));
	sz += (length < 8) ? 4 : 9;
	return sz;//sz / 8;
}

//获取当前匹配位置最长的串
void longest(BYTE *readbuf, DWORD n, vector<DWORD> *hash_tab, int i, int lessthan, long long &dstc, long long &lgth, DWORD windowsz)//fast:1~3
{
	dstc = lgth = 0;
	DWORD hash = *((DWORD*)(readbuf + i)) & 0xffffff, lastoutputsize = 0;
	for (long long j = hash_tab[hash].size() - 1; j >= 0 && i - hash_tab[hash][j] <= windowsz; j--)//进行一些小小的优化，从最近的地方开始匹配，如果超出了最大长度则直接返回，
	{//原以为这不会有多大的优化作用，没想到这么写完了之后速度将近快了一倍。。。
		DWORD last = hash_tab[hash][j] + 3, k;
		
		for (k = i + 3; *((DWORD *)(readbuf + k)) == *((DWORD *)(readbuf + last)) && k < n && k - i <= lessthan; k += 4, last += 4);// 快跑，每次比较 4 byte 
		for (; readbuf[k] == readbuf[last] && k < n && k - i <= lessthan; k++, last++);// 慢跑，每次比较 1 byte，但是执行到慢跑时最多也就比较4 byte就可以了。 

		if ((((long long)(k - i)) << 3) - getoutputsize(i - hash_tab[hash][j], k - i, i) > (long long)(lgth << 3) - getoutputsize(i - dstc, lgth, i) || dstc == 0)
		{
			lgth = k - i;
			dstc = hash_tab[hash][j];
		}

		if (lgth >= lessthan)
		{
			lgth = lessthan;
			return;
		}
	}
	return;
}

// 将输入文件压缩成输出文件
short dawncompress(char *inf, char *outf, DWORD windowsz)
{
	init();
	QWORD dsize = getfilesize(inf);
	bitput = bitp = 0;

	outbuf.reserve(500 * 1024 * 1024);// 500KB 预备值 

	BYTE *readbuf = new BYTE[dsize + 5];
	DWORD lessthan = 258, morethan = 3;// DL 数对中 length 需要小于 258 。否则将在输出 length 的时候出错（事实上，长度大于 256 的都是分罕见了。。。）
	FILE *rd = fopen(inf, "rb");
	QWORD n = fread(readbuf, 1, dsize, rd);

	// 是的，原先这里是一个循环，但是考虑到输入的 dsize 就等于 infilesize ，所以就不进行循环了 这样可能会快一点。

	vector<DWORD> *hash_tab = new vector<DWORD>[0x1000000];//哈希表优化查找（即便如此也是分缓慢。。。）

	if (n < morethan)
	{
		cout << "无法压缩小于 3 byte 的文件" << endl;
		return UNKNOWERROR;
	}

	long long laststart = 0, lastlen = 0, start = 0, len = 0;
	bool ard = false;
	for (QWORD i = 0; i < n; i++)
	{
		laststart = start;
		lastlen = len;

		longest(readbuf, n, hash_tab, i, lessthan, start, len, windowsz);
		hash_tab[*((DWORD*)(readbuf + i)) & 0xffffff].push_back(i);

		//不再是 lastlen >= morethan 啦！原先仿照 deflate 的哪个版本，由于不知道最终输出的长度，所以只能这么写。但是现在
		//我们可以通过 getoutputsize 函数确定最终输出的 DL 对大小。这样一来，“ DL 对的长度大于原先的长度”这样的事情就可避免了。 
		if ((lastlen << 3) > getoutputsize(i - laststart - 1, lastlen, i - 1) && lastlen >= morethan && \
			(len << 3) - getoutputsize(i - start, len, i) <= (lastlen << 3) - getoutputsize(i - laststart - 1, lastlen, i - 1))// 上一次匹配足够好
		{
			for (int j = 1; j < lastlen - 1; j++)
				hash_tab[*((DWORD*)(readbuf + i + j)) & 0xffffff].push_back(j + i);

			outputablock(i - laststart - 1, lastlen, i - 1);

			i += lastlen - 2;
			len = 0;
			ard = false;
		}
		else
			if (ard)// ard == true :不是开始或是上一个为 dstc + lgth 压缩串 
				outputablock(0, readbuf[i - 1], i);// 上一次的匹配并不是那么好，将上一个匹配的首个字符输出，并将第二个字符作为首位并继续匹配 
			else
				ard = true;
		if (i % 1000 == 0)
		{
			cout << i << '/' << dsize << endl;
			cout << outbuf.size() << endl;
		}
	}
	if (ard)// 最后有没输出的
		outputablock(0, readbuf[n - 1], 0);

	delete[] readbuf;

	if (bitp)
		outbuf.push_back(bitput);

	FILE *wt = fopen(outf, "wb");
	fwrite("CZL", 1, 3, wt);

	QWORD filesize = dsize;
	fwritevlq(filesize, wt);

	for (int i = 0; i < outbuf.size(); i++)// 输出整体数据
		fwrite(&outbuf[i], 1, 1, wt);
	fclose(wt);

	return OK;
}

short dawndecompress(char *inf, char *outf)
{
	init();

	FILE *rd = fopen(inf, "rb"), *wt = fopen(outf, "wb");
	WORD p = 0;
	QWORD dp = 0, filesize = getfilesize(inf), allsize = 0, allin = 0;//dsize p
	DWORD bp;
	BYTE ch, read, *buf, *readbuf = new BYTE[filesize];

	fread(readbuf, 1, 3, rd);

	returnif(strncmp((char *)readbuf, "CZL", 3), COMPRESSFILEBREAK);// 获取标志（就是本人名字的缩写啦~ :-P ）

	allin = freadvlq(rd);

	buf = new BYTE[allin + 5];

	fread(readbuf, 1, filesize, rd);
	fclose(rd);

	bitp = 0;
	bitput = readbuf[bp++];

	while (dp < allin)// 主循环，循环一次处理一个信息块
	{
		BYTE flag = bitread(readbuf, bp, bitput, bitp, 1);
		if (flag == 0)//literal
		{
			read = bitread(readbuf, bp, bitput, bitp, 8);
			fwrite(&read, 1, 1, wt);
			buf[dp++] = read;
		}
		else//length
		{
			DWORD dstc, len;
			//distance
			BYTE flag = bitread(readbuf, bp, bitput, bitp, 1);
			if (flag == 0)
				dstc = bitread(readbuf, bp, bitput, bitp, 6);
			else
			{
				flag = bitread(readbuf, bp, bitput, bitp, 1);
				dstc = (flag == 0) ? bitread(readbuf, bp, bitput, bitp, 11) : bitread(readbuf, bp, bitput, bitp, ceil(log2(dp + 4)));
			}
			//length
			flag = bitread(readbuf, bp, bitput, bitp, 1);
			len = (flag == 1) ? bitread(readbuf, bp, bitput, bitp, 8) : bitread(readbuf, bp, bitput, bitp, 3);

			if (dstc < 4)
			{
				DWORD tdstc = lastdis[dstc];
				for (int i = dstc; i > 0; i--)
					lastdis[i] = lastdis[i - 1];
				lastdis[0] = dstc = tdstc;
			}
			else
			{
				dstc -= 4;
				lastdis[3] = lastdis[2];
				lastdis[2] = lastdis[1];
				lastdis[1] = lastdis[0];
				lastdis[0] = dstc;
			}

			int tlen = lastlen[len];
			for (int i = len; i > 0; i--)
				lastlen[i] = lastlen[i - 1];
			lastlen[0] = len = tlen;

			len += 3;
			dstc++;

			if (dstc > dp || dp + len >= allin + 10)
			{
				cout << "解压缩时遇到错误，不是Dawn 格式或文件已损坏" << endl;
				fclose(rd);
				fclose(wt);
				return COMPRESSFILEBREAK;
			}

			for (QWORD i = 0; i < len; i++)//这里，mingw64的memcpy是骗人的。。。用了memcpy会出错。。。
				buf[dp + i] = buf[dp - dstc + i];

			fwrite(buf + dp, 1, len, wt);
			dp += len;
			if (dp > allin)
				cout << "ERROR:" << dstc << " " << len << endl << dp << endl;
		}
	}
	delete[] readbuf;
	delete[] buf;
	fclose(wt);
	return OK;
}
/*
* =================
* 总而言之，算是超过了《 30 天》中的tek2算法。想的脑阔疼，而且是“算是”超过，在
* 同字典大小下仍然要比 tek2 大一点。 :(
* 但是最终还算是完成了一个不用huffman等算法辅助，解压缩时间复杂度达到了 O(n) 的算法。:)
*/
