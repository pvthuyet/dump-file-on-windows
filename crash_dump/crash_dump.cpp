// crash_dump.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "minidump.h"
using namespace died::Minidump;

int main()
{
	startMonitering("");
	int* p = nullptr;
	*p = 1;
    return 0;
}

