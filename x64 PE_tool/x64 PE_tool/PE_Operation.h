#pragma once
#include <Windows.h>
#include <stdio.h>

//初始化文件路径，判断文件路径是否含空格后，分别处理
void InitFilePath(char* FilePath);

//获取文件在内存中的大小
DWORD GetSizeOfImage(PBYTE pFileBuffer);

//获取文件加载基址
ULONG64 GetImageBase(PBYTE pFileBuffer);

//把读取的磁盘文件在内存中拉伸
void FileBuffer_To_ImageBuffer(PBYTE pFileBuffer, PBYTE pImageBuffer);

//RVA 转化 FOA
DWORD RVAToFOA(PBYTE pFileBuffer, DWORD RVA);

//修复导入表
void Import_Descriptor_Repair(PBYTE pFileBuffer, PBYTE pImageBuffer);

//修复重定位表
void Relocation_Descriptor_Repair(PBYTE pFileBuffer, PBYTE pImageBuffer);

//创建执行线程
void CreateExecuteThread(PBYTE pImageBuffer);

