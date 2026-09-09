#pragma once
#include <Windows.h>
#include <stdio.h>

//判断文件是否开启ASLR
BOOL ASLRChack(PBYTE pImageBuffer);

//判断文件是否为合规PE文件
BOOL PEChack(PBYTE pFileBuffer);