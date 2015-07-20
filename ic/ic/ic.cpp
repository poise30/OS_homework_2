#include "stdafx.h"
#include <stdint.h>
#include <conio.h> //ÄÜ¼Ö IO
#include "StopWatch.h"
#include "mmio.h"


/*
uint32_t mymain(int argc, wchar_t** argtv)
{
	
	
	return 0;
	
}
*/
int _tmain(int argc, _TCHAR* argv[])
{
	printf("hello~ _tmain");
	
	_ASSERTE(create_very_big_file(L"big.txt", 5000));
	
	StopWatch sw;
	sw.Start();
	printf("file_copy_using_read_write\n");
	_ASSERTE(file_copy_using_read_write(L"big.txt", L"big2.txt"));
	sw.Stop();
	print("file_copy_using_read_write info] time elapsed = %f", sw.GetDurationSecond());
	
	StopWatch sw2;
	sw2.Start();
	printf("file_copy_using_memory_map\n");
	_ASSERTE(file_copy_using_memory_map(L"big.txt", L"big3.txt"));
	sw2.Stop();
	print("file_copy_using_memory_map info] time elapsed = %f", sw2.GetDurationSecond());
	getchar();
	return 0;
}

