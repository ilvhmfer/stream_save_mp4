// upload_test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>

#include<cstdlib>
#include<string>
#include<cstdio>
#include<cstring>
#include<iostream>
#include<algorithm>

#include <thread>
#include <queue>
#include <atomic>
#include <fstream>

extern "C"
{
#include "libavutil/timestamp.h"
#include "libavformat/avformat.h"
}

int save_mp4_new(const char* out_filename, int len);

#define IO_BUFFER_SIZE 200000

using namespace std;

int main()
{
	save_mp4_new("output.mp4", 25*10);

    std::cout << "save end!\n"; 
	getchar();

	return 0;
}

