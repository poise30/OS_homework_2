/**----------------------------------------------------------------------------
*
*-----------------------------------------------------------------------------
* All rights reserved by Noh,Yonghwan (fixbrain@gmail.com, unsorted@msn.com)
*-----------------------------------------------------------------------------
*
**---------------------------------------------------------------------------*/

// read
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa366542(v=vs.85).aspx

#include "stdafx.h"
//#include <Windows.h> //윈도우 관련 함수, Get~ , .pch(precomplied header)
//#include <stdint.h> //uint32_t

/**
* @brief
* @param
* @see
* @remarks
* @code
* @endcode
* @return
**/
//err, 문자열, 가변인수
void print(_In_ const char* fmt, _In_ ...)
{	//_ln_ 읽기전용?


	char log_buffer[2048];
	va_list args; //가변인수

	va_start(args, fmt); //args 를 fmt 형태로 읽어들임
	HRESULT hRes = StringCbVPrintfA(log_buffer, sizeof(log_buffer), fmt, args);
	if (S_OK != hRes) //S_OK 함수가 성공하였음을 의미
	{
		//오류 출력
		fprintf(
			stderr,
			"%s, StringCbVPrintfA() failed. res = 0x%08x",
			__FUNCTION__,
			hRes
			);
		return;
	}

	OutputDebugStringA(log_buffer); //디버깅할때 사용
	fprintf(stdout, "%s \n", log_buffer);
}


bool is_file_existsW(_In_ const wchar_t* file_path)
{
	_ASSERTE(NULL != file_path);
	_ASSERTE(TRUE != IsBadStringPtrW(file_path, MAX_PATH));
	if ((NULL == file_path) || (TRUE == IsBadStringPtrW(file_path, MAX_PATH))) return false;

	WIN32_FILE_ATTRIBUTE_DATA info = { 0 };

	if (GetFileAttributesExW(file_path, GetFileExInfoStandard, &info) == 0)
		return false;
	else
		return true;
}


bool read_file_using_memory_map()
{
	// current directory 를 구한다.
	wchar_t *buf = NULL;
	uint32_t buflen = 0;
	buflen = GetCurrentDirectoryW(buflen, buf);
	if (0 == buflen)
	{
		print("err ] GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		return false;
	}

	buf = (PWSTR)malloc(sizeof(WCHAR) * buflen);
	if (0 == GetCurrentDirectoryW(buflen, buf))
	{
		print("err ] GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		free(buf);
		return false;
	}

	// current dir \\ test.txt 파일명 생성
	wchar_t file_name[260];
	if (!SUCCEEDED(StringCbPrintfW(
		file_name,
		sizeof(file_name),
		L"%ws\\test.txt",
		buf)))
	{
		print("err ] can not create file name");
		free(buf);
		return false;
	}
	free(buf); buf = NULL;

	if (true != is_file_existsW(file_name))
	{
		print("err ] no file exists. file = %ws", file_name);
		return false;
	}

	HANDLE file_handle = CreateFileW(
		(LPCWSTR)file_name,
		GENERIC_READ,
		NULL,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
	if (INVALID_HANDLE_VALUE == file_handle)
	{
		print("err ] CreateFile(%ws) failed, gle = %u", file_name, GetLastError());
		return false;
	}

	// check file size
	//
	LARGE_INTEGER fileSize;
	if (TRUE != GetFileSizeEx(file_handle, &fileSize))
	{
		print("err ] GetFileSizeEx(%ws) failed, gle = %u", file_name, GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	// [ WARN ]
	//
	// 4Gb 이상의 파일의 경우 MapViewOfFile()에서 오류가 나거나
	// 파일 포인터 이동이 문제가 됨
	// FilIoHelperClass 모듈을 이용해야 함
	//
	/**/
	_ASSERTE(fileSize.HighPart == 0);
	if (fileSize.HighPart > 0)
	{
		print("file size = %I64d (over 4GB) can not handle. use FileIoHelperClass",
			fileSize.QuadPart);
		CloseHandle(file_handle);
		return false;
	}

	DWORD file_size = (DWORD)fileSize.QuadPart;
	HANDLE file_map = CreateFileMapping(
		file_handle,
		NULL,
		PAGE_READONLY,
		0,
		0,
		NULL
		);
	if (NULL == file_map)
	{
		print("err ] CreateFileMapping(%ws) failed, gle = %u", file_name, GetLastError());
		CloseHandle(file_handle);
		return false;
	}
	PCHAR file_view = (PCHAR)MapViewOfFile(
		file_map,
		FILE_MAP_READ,
		0,
		0,
		0
		);
	if (file_view == NULL)
	{
		print("err ] MapViewOfFile(%ws) failed, gle = %u", file_name, GetLastError());

		CloseHandle(file_map);
		CloseHandle(file_handle);
		return false;
	}

	// do some io
	char a = file_view[0];  // 0x d9
	char b = file_view[1];  // 0xb3



	// close all
	UnmapViewOfFile(file_view);
	CloseHandle(file_map);
	CloseHandle(file_handle);
	return true;
}


/**
* @brief
* @param
* @see
* @remarks
* @code
* @endcode
* @return
**/
bool create_very_big_file(_In_ const wchar_t* file_path, _In_ uint32_t size_in_mb)
{
	_ASSERTE(NULL != file_path);
	if (NULL == file_path) return false;

	if (is_file_existsW(file_path))
	{
		::DeleteFileW(file_path);
	}

	// create very big file
	HANDLE file_handle = CreateFile(
		file_path,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
	if (INVALID_HANDLE_VALUE == file_handle)
	{
		print("err ] CreateFile( %ws ) failed. gle = %u", file_path, GetLastError());
		return false;
	}

	LARGE_INTEGER file_size = { 0 };
	LARGE_INTEGER ret;
	//file_size.LowPart = 0;
	//file_size.HighPart = 1;
	file_size.QuadPart = (LONGLONG)(1024 * 1024) * (LONGLONG)size_in_mb;
	if (!SetFilePointerEx(file_handle, file_size, NULL, FILE_BEGIN))
	{
		print("err ] SetFilePointerEx() failed. gle = %u", GetLastError());

		CloseHandle(file_handle);
		return false;
	}

	SetEndOfFile(file_handle);
	CloseHandle(file_handle);
	return true;
}












/**
* @brief
* @param
* @see
* @remarks
* @code
* @endcode
* @return
**/
typedef struct map_context
{
	HANDLE  handle;
	LARGE_INTEGER   size;
	HANDLE  map;
	PCHAR   view;
}*pmap_context;
/*
pmap_context open_map_context(_In_ const wchar_t* file_path)
{
	_ASSERTE(NULL != file_path);
	if (NULL == file_path) return false;
	if (!is_file_existsW(file_path)) return false;;

	pmap_context ctx = (pmap_context)malloc(sizeof(map_context));
	RtlZeroMemory(ctx, sizeof(map_context));

	bool ret = false;

#pragma warning(disable: 4127) //4127 경고 disable
	do
	{
		ctx->handle = CreateFileW(
			(LPCWSTR)file_path,
			GENERIC_READ,
			NULL,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);
		if (INVALID_HANDLE_VALUE == ctx->handle)
		{
			print("err ] CreateFile( %ws ) failed. gle = %u", file_path, GetLastError());
			break;
		}

		// check file size
		//
		LARGE_INTEGER fileSize;
		if (TRUE != GetFileSizeEx(ctx->handle, &fileSize)) //파일크기 filesize에 저장
		{
			print("err ] GetFileSizeEx( %ws ) failed. gle = %u", file_path, GetLastError());
			break;
		}

		// [ WARN ]
		//
		// 4Gb 이상의 파일의 경우 MapViewOfFile()에서 오류가 나거나
		// 파일 포인터 이동이 문제가 됨
		// FilIoHelperClass 모듈을 이용해야 함
		//
		_ASSERTE(fileSize.HighPart == 0);
		if (fileSize.HighPart > 0)
		{
			print("err ] file is too large to map. file = %ws, size = %llu", file_path, fileSize.QuadPart);
			break;
		}

		ctx->size = (DWORD)fileSize.QuadPart;
		ctx->map = CreateFileMapping(
			ctx->handle,
			NULL,
			PAGE_READONLY,
			0,
			0,
			NULL
			);
		if (NULL == ctx->map)
		{
			print("err ] CreateFileMapping( %ws ) failed. gle = %u", file_path, GetLastError());
			break;
		}

		//

		// MapViewOfFile() 함수의 dwFileOffsetLow 파라미터는 
		// SYSTEM_INFO::dwAllocationGranularity 값의 배수이어야 한다.
		// 
		LARGE_INTEGER Offset = { 0 };
		Offset.LowPart = 0;
		DWORD Size = ctx->size;

		static DWORD AllocationGranularity = 0;
		if (0 == AllocationGranularity)
		{
			SYSTEM_INFO si = { 0 };
			GetSystemInfo(&si);
			AllocationGranularity = si.dwAllocationGranularity;
		}

		DWORD AdjustMask = AllocationGranularity - 1;
		LARGE_INTEGER AdjustOffset = { 0 };
		AdjustOffset.HighPart = Offset.HighPart;

		// AllocationGranularity 이하의 값을 버림
		// 
		AdjustOffset.LowPart = (Offset.LowPart & ~AdjustMask);

		// 버려진 값만큼 매핑할 사이즈를 증가
		// 
		DWORD BytesToMap = ((Offset.LowPart & AdjustMask) + Size)/4;
		ctx->size = ((Offset.LowPart & AdjustMask) + Size)/4;

		ctx->view = (PCHAR)MapViewOfFile(
			ctx->map,
			FILE_MAP_READ,
			AdjustOffset.HighPart,
			AdjustOffset.LowPart,
			BytesToMap
			);
		if (ctx->view == NULL)
		{
			print("err ] MapViewOfFile( %ws ) failed. gle = %u", file_path, GetLastError());
			break;
		}

		ret = true;
	} while (FALSE);
#pragma warning(default: 4127)

	if (!ret)
	{
		if (NULL != ctx->view) UnmapViewOfFile(ctx->view);
		if (NULL != ctx->map) CloseHandle(ctx->map);
		if (INVALID_HANDLE_VALUE != ctx->handle) CloseHandle(ctx->handle);

		free(ctx); ctx = NULL;
	}

	return ctx;
}
*/

/**
* @brief
* @param
* @see
* @remarks
* @code
* @endcode
* @return
**/

/**
* @brief
* @param
* @see
* @remarks
* @code
* @endcode
* @return
**/
void close_map_context(_In_ pmap_context ctx)
{
	if (NULL != ctx)
	{
		if (NULL != ctx->view) UnmapViewOfFile(ctx->view);
		if (NULL != ctx->map) CloseHandle(ctx->map);
		if (INVALID_HANDLE_VALUE != ctx->handle) CloseHandle(ctx->handle);
		free(ctx);
	}
}


bool create_map_context(_In_ const wchar_t* src_path, _In_ const wchar_t* dst_path)
{
	_ASSERTE(NULL != src_path);
	if (NULL == src_path) return false;
	if (!is_file_existsW(src_path)) return false;;

	pmap_context src_ctx = (pmap_context)malloc(sizeof(map_context));
	RtlZeroMemory(src_ctx, sizeof(map_context));

	_ASSERTE(NULL != dst_path);
	if (NULL == dst_path) return false;
	if (is_file_existsW(dst_path))
	{
		DeleteFileW(dst_path);
	}

	pmap_context dst_ctx = (pmap_context)malloc(sizeof(map_context));
	RtlZeroMemory(dst_ctx, sizeof(map_context));

	bool ret = false;

#pragma warning(disable: 4127)
	do
	{
				src_ctx->handle = CreateFileW(
				(LPCWSTR)src_path,
				GENERIC_READ,
				NULL,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL
				);
			if (INVALID_HANDLE_VALUE == src_ctx->handle)
			{
				print("err ] CreateFile( %ws ) failed. gle = %u", src_path, GetLastError());
				break;
			}

			// check file size
			//
			LARGE_INTEGER fileSize;
			if (TRUE != GetFileSizeEx(src_ctx->handle, &fileSize)) //파일크기 filesize에 저장
			{
				print("err ] GetFileSizeEx( %ws ) failed. gle = %u", src_path, GetLastError());
				break;
			}

			// [ WARN ]
			//
			// 4Gb 이상의 파일의 경우 MapViewOfFile()에서 오류가 나거나
			// 파일 포인터 이동이 문제가 됨
			// FilIoHelperClass 모듈을 이용해야 함
			//
			/*
			_ASSERTE(fileSize.HighPart == 0);
			if (fileSize.HighPart > 0)
			{
				print("err ] file is too large to map. file = %ws, size = %llu", src_path, fileSize.QuadPart);
				break;
			}
			*/
			src_ctx->size = fileSize;
			src_ctx->map = CreateFileMapping(
				src_ctx->handle,
				NULL,
				PAGE_READONLY,
				0,
				0,
				NULL
				);
			if (NULL == src_ctx->map)
			{
				print("err ] CreateFileMapping( %ws ) failed. gle = %u", src_path, GetLastError());
				break;
			}


			//dst
			dst_ctx->handle = CreateFileW(
			(LPCWSTR)dst_path,
			GENERIC_READ | GENERIC_WRITE,
			NULL,
			NULL,
			CREATE_NEW,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);
		if (INVALID_HANDLE_VALUE == dst_ctx->handle)
		{
			print("err ] CreateFile( %ws ) failed. gle = %u", dst_path, GetLastError());
			break;
		}

		dst_ctx->size = fileSize;
		dst_ctx->map = CreateFileMapping(
			dst_ctx->handle,
			NULL,
			PAGE_READWRITE,
			dst_ctx->size.HighPart,
			dst_ctx->size.LowPart,
			NULL
			);
		if (NULL == dst_ctx->map)
		{
			print("err ] CreateFileMapping( %ws ) failed. gle = %u", dst_path, GetLastError());
			break;
		}
		// MapViewOfFile() 함수의 dwFileOffsetLow 파라미터는 
		// SYSTEM_INFO::dwAllocationGranularity 값의 배수이어야 한다.
		// 
		LARGE_INTEGER Offset = { 0 };
		Offset.LowPart = 0;
		LARGE_INTEGER Size = dst_ctx->size;

		static DWORD AllocationGranularity = 0;
		if (0 == AllocationGranularity)
		{
			SYSTEM_INFO si = { 0 };
			GetSystemInfo(&si);
			AllocationGranularity = si.dwAllocationGranularity;
		}

		DWORD AdjustMask = AllocationGranularity - 1;
		LARGE_INTEGER AdjustOffset = { 0 };
		AdjustOffset.HighPart = Offset.HighPart;
		// AllocationGranularity 이하의 값을 버림
		AdjustOffset.LowPart = (Offset.LowPart & ~AdjustMask);
		LONGLONG BytesToMap = (LONGLONG)((Offset.LowPart & AdjustMask) + (LONGLONG)(Size.QuadPart / 20));
		// AllocationGranularity 이하의 값을 버
		// 버려진 값만큼 매핑할 사이즈를 증가

		while (dst_ctx->size.QuadPart > 0){
		
			// 버려진 값만큼 매핑할 사이즈를 증가
			src_ctx->view = (PCHAR)MapViewOfFile(
				src_ctx->map,
				FILE_MAP_ALL_ACCESS,
				AdjustOffset.HighPart,
				AdjustOffset.LowPart,
				BytesToMap
				);
			if (src_ctx->view == NULL)
			{
				//print("err ] MapViewOfFile( %ws ) failed. gle = %u", src_path, GetLastError());
				break;
			}
			dst_ctx->view = (PCHAR)MapViewOfFile(
				dst_ctx->map,
				FILE_MAP_WRITE | FILE_MAP_READ,
				AdjustOffset.HighPart,
				AdjustOffset.LowPart,
				BytesToMap
				);
			if (dst_ctx->view == NULL)
			{
				print("err ] MapViewOfFile( %ws ) failed. gle = %u", dst_path, GetLastError());
				break;
			}

			// copy src to dst by mmio
			for (uint32_t i = 0; i < BytesToMap + AdjustOffset.LowPart; ++i)
			{
				dst_ctx->view[i] = src_ctx->view[i];
			}

			UnmapViewOfFile(src_ctx->map);
			UnmapViewOfFile(dst_ctx->map);

			AdjustOffset.LowPart += BytesToMap;
			dst_ctx->size.QuadPart -= BytesToMap;
		}

		ret = true;
	} while (FALSE);

#pragma warning(default: 4127)

	if (!ret)
	{
		if (NULL != src_ctx->view) UnmapViewOfFile(src_ctx->view);
		if (NULL != src_ctx->map) CloseHandle(src_ctx->map);
		if (INVALID_HANDLE_VALUE != src_ctx->handle) CloseHandle(src_ctx->handle);
		if (NULL != dst_ctx->view) UnmapViewOfFile(dst_ctx->view);
		if (NULL != dst_ctx->map) CloseHandle(dst_ctx->map);
		if (INVALID_HANDLE_VALUE != dst_ctx->handle) CloseHandle(dst_ctx->handle);

		free(src_ctx); src_ctx = NULL;
		free(dst_ctx); dst_ctx = NULL;
		close_map_context(src_ctx);
		close_map_context(dst_ctx);
		return ret;

	}

	close_map_context(src_ctx);
	close_map_context(dst_ctx);

	return ret;
}



/**
* @brief
* @param
* @see
* @remarks
* @code
* @endcode
* @return
**/
bool
file_copy_using_memory_map(
_In_ const wchar_t* src_file,
_In_ const wchar_t* dst_file
)
{
	_ASSERTE(NULL != src_file);
	_ASSERTE(NULL != dst_file);
	if (NULL == src_file || NULL == dst_file) return false;

	if (!is_file_existsW(src_file))
	{
		print("err ] no src file = %ws", src_file);
		return false;
	}

	if (is_file_existsW(dst_file))
	{
		DeleteFileW(dst_file);
	}

	// map src, dst file
	//pmap_context src_ctx = open_map_context(src_file);
	bool ctx = create_map_context(src_file, dst_file);
	if (NULL == ctx)
	{
		print("err ] open_map_context() failed.");
		return false;
	}

	return true;
}

/**
* @brief
* @param
* @see
* @remarks
* @code
* @endcode
* @return
**/
bool
file_copy_using_read_write(
_In_ const wchar_t* src_file,
_In_ const wchar_t* dst_file
)
{
	_ASSERTE(NULL != src_file);
	_ASSERTE(NULL != dst_file);
	if (NULL == src_file || NULL == dst_file) return false;

	if (!is_file_existsW(src_file))
	{
		print("err ] no src file = %ws", src_file);
		return false;
	}

	if (is_file_existsW(dst_file))
	{
		DeleteFileW(dst_file);
	}

	// open src file with READ mode
	HANDLE src_handle = CreateFileW(
		src_file,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
	if (INVALID_HANDLE_VALUE == src_handle)
	{
		print("err ] CreateFile( %ws ) failed. gle = %u", src_file, GetLastError());
		return false;
	}

	// open dst file with WRITE mode
	HANDLE dst_handle = CreateFileW(
		dst_file,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
	if (INVALID_HANDLE_VALUE == dst_handle)
	{
		print("err ] CreateFile( %ws ) failed. gle = %u", dst_file, GetLastError());

		CloseHandle(src_handle);
		return false;
	}

	// file copy
	bool ret = false;
	char buf[4096] = { 0 };
	DWORD bytes_written = 0;
	DWORD bytes_read = 0;

	do
	{
		// read from src
		if (!ReadFile(src_handle, buf, sizeof(buf), &bytes_read, NULL))
		{
			print("err ] ReadFile( src_handle ) failed. gle = %u", GetLastError());
			break;
		}
		else
		{
			// please read
			// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365690(v=vs.85).aspx
			if (0 == bytes_read)
			{
				ret = true;
				break;
			}
		}

		// write to dst
		if (!WriteFile(dst_handle, buf, sizeof(buf), &bytes_written, NULL))
		{
			print("err ] WriteFile( dst_handle ) failed. gle = %u", GetLastError());
			break;
		}
	} while (true);


	CloseHandle(src_handle);
	CloseHandle(dst_handle);
	return ret;
}