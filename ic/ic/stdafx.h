// stdafx.h : ���� ��������� ���� ��������� �ʴ�
// ǥ�� �ý��� ���� ���� �� ������Ʈ ���� ���� ������
// ��� �ִ� ���� �����Դϴ�.
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#include <Windows.h>
#include <stdint.h>
#include <crtdbg.h>
#include <strsafe.h> //secure�� string ���� �Լ��� ��Ƴ���.

void print(_In_ const char* fmt, _In_ ...);
bool is_file_existsW(_In_ const wchar_t* file_path);



// TODO: ���α׷��� �ʿ��� �߰� ����� ���⿡�� �����մϴ�.
