#include "PE_Operation.h"

//获取文件在内存中的大小
DWORD GetSizeOfImage(PBYTE pFileBuffer)
{
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pFileBuffer;
	PIMAGE_NT_HEADERS64 pNt = (PIMAGE_NT_HEADERS64)(pFileBuffer + pDos->e_lfanew);
	return pNt->OptionalHeader.SizeOfImage;
}

ULONG64 GetImageBase(PBYTE pFileBuffer)
{
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pFileBuffer;
	PIMAGE_NT_HEADERS64 pNt = (PIMAGE_NT_HEADERS64)(pFileBuffer + pDos->e_lfanew);
	return pNt->OptionalHeader.ImageBase;
}

//把读取的磁盘文件在内存中拉伸
void FileBuffer_To_ImageBuffer(PBYTE pFileBuffer, PBYTE pImageBuffer)
{
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pFileBuffer;
	PIMAGE_NT_HEADERS64 pNt = (PIMAGE_NT_HEADERS64)(pFileBuffer + pDos->e_lfanew);									//64位 和 32位结构不大相同
	PIMAGE_FILE_HEADER pPE = (PIMAGE_FILE_HEADER)((PBYTE)pNt + 4);
	PIMAGE_OPTIONAL_HEADER64 pOp = (PIMAGE_OPTIONAL_HEADER64)((PBYTE)pPE + IMAGE_SIZEOF_FILE_HEADER);				//64位 和 32位结构不大相同
	PIMAGE_SECTION_HEADER pSec = (PIMAGE_SECTION_HEADER)((PBYTE)pOp + pNt->FileHeader.SizeOfOptionalHeader);

	//硬盘 和 内存 整个头部是完全相同的， 不同的地方是节表对应的区域
	memcpy(pImageBuffer, pFileBuffer, pNt->OptionalHeader.SizeOfHeaders);
	for (int i = 0; i < pNt->FileHeader.NumberOfSections; i++)
	{

		DWORD rawSize = pSec[i].SizeOfRawData;
		DWORD virtSize = pSec[i].Misc.VirtualSize;
		// 这里需要注意 SizeOfRawData是文件中对齐后的大小。
		//Misc.VirtualSize不能直接理解为“内存中的最终大小”。它只是“内存中有效数据的逻辑大小”，且不包含对齐填充。
		if (rawSize > 0)
		{
			DWORD bytesToCopy = (rawSize < virtSize) ? rawSize : virtSize;
			if (bytesToCopy > 0) {
				memcpy(pImageBuffer + pSec[i].VirtualAddress,
					pFileBuffer + pSec[i].PointerToRawData,
					bytesToCopy);
			}
		}
		if (virtSize > rawSize)
		{
			memset(pImageBuffer + pSec[i].VirtualAddress + rawSize,
				0,
				virtSize - rawSize);
		}
	}
}

//RVA 转化 FOA
DWORD RVAToFOA(PBYTE pFileBuffer, DWORD RVA)
{
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pFileBuffer;
	PIMAGE_NT_HEADERS64 pNt = (PIMAGE_NT_HEADERS64)(pFileBuffer + pDos->e_lfanew);
	PIMAGE_FILE_HEADER pPE = (PIMAGE_FILE_HEADER)((PBYTE)pNt + 4);
	PIMAGE_OPTIONAL_HEADER64 pOp = (PIMAGE_OPTIONAL_HEADER64)((PBYTE)pPE + IMAGE_SIZEOF_FILE_HEADER);
	PIMAGE_SECTION_HEADER pSec = (PIMAGE_SECTION_HEADER)((PBYTE)pOp + pNt->FileHeader.SizeOfOptionalHeader);

	DWORD FOA = 0;
	//判断RVA位置
	if (RVA <= pOp->SizeOfHeaders)
	{

		return RVA;
	}

	for (int i = 0; i < pPE->NumberOfSections; i++, pSec++)
	{
		if (RVA >= pSec->VirtualAddress && RVA < (pSec->VirtualAddress + pSec->Misc.VirtualSize))
		{
			RVA -= pSec->VirtualAddress;
			FOA = RVA + pSec->PointerToRawData;
		}
	}

	return FOA;
}

//修复导入表
void Import_Descriptor_Repair(PBYTE pFileBuffer, PBYTE pImageBuffer)
{
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pFileBuffer;
	PIMAGE_NT_HEADERS64 pNt = (PIMAGE_NT_HEADERS64)(pFileBuffer + pDos->e_lfanew);
	PIMAGE_FILE_HEADER pPE = (PIMAGE_FILE_HEADER)((PBYTE)pNt + 4);
	PIMAGE_OPTIONAL_HEADER64 pOp = (PIMAGE_OPTIONAL_HEADER64)((PBYTE)pPE + IMAGE_SIZEOF_FILE_HEADER);
	PIMAGE_SECTION_HEADER pSec = (PIMAGE_SECTION_HEADER)((PBYTE)pOp + pNt->FileHeader.SizeOfOptionalHeader);
	PIMAGE_DATA_DIRECTORY pDir = (PIMAGE_DATA_DIRECTORY)((PBYTE)pOp + 0x70);
	PIMAGE_IMPORT_DESCRIPTOR pImp = (PIMAGE_IMPORT_DESCRIPTOR)(pFileBuffer + RVAToFOA(pFileBuffer, pDir[1].VirtualAddress));
	HMODULE hDll;

	while (1)
	{
		if (pImp->OriginalFirstThunk == 0) break;

		//获取导入dll
		char* DllName = (char*)(pFileBuffer + RVAToFOA(pFileBuffer, pImp->Name));
		//printf("%s\n", DllName);
		hDll = LoadLibraryA(DllName);//使用LoadLibraryA加载dll
		if (!hDll)
		{
			printf("无法加载DLL: %s\n", DllName);
			continue;
		}
		PIMAGE_THUNK_DATA64 pINT = (PIMAGE_THUNK_DATA64)(pFileBuffer + RVAToFOA(pFileBuffer, pImp->OriginalFirstThunk));//INT在文件中
		PIMAGE_THUNK_DATA64 pIAT = (PIMAGE_THUNK_DATA64)(pImageBuffer + pImp->FirstThunk);//修改内存中的IAT

		while (1)
		{
			if (*(PULONGLONG)pINT == 0) break;

			FARPROC pFunc = NULL;
			if ((pINT->u1.Ordinal & IMAGE_ORDINAL_FLAG64) != 0)
			{
				//最高位为1，以序号导入
				WORD ordinal = (WORD)(pINT->u1.Ordinal & 0xFFFF);
				//printf("%d\n", number);
				pFunc = GetProcAddress(hDll, MAKEINTRESOURCEA(ordinal)); //MAKEINTRESOURCEA将整数资源标识符强制转换为字符串指针类型
			}
			else
			{
				//最高位不为1，以名称导入
				PIMAGE_IMPORT_BY_NAME ptr = (PIMAGE_IMPORT_BY_NAME)(pFileBuffer + RVAToFOA(pFileBuffer, pINT->u1.AddressOfData));
				char* Name = ptr->Name;
				pFunc = GetProcAddress(hDll, Name);
			}

			*(PULONGLONG)pIAT = (ULONGLONG)pFunc;
			pIAT++;
			pINT++;
		}
		pImp++;
	}
	printf("导入表修复完成!\n");
}

//修复重定位表
void Relocation_Descriptor_Repair(PBYTE pFileBuffer, PBYTE pImageBuffer)
{
	//判断重定位表是否需要修复
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pFileBuffer;
	PIMAGE_NT_HEADERS64 pNt = (PIMAGE_NT_HEADERS64)(pFileBuffer + pDos->e_lfanew);
	PIMAGE_DATA_DIRECTORY pDir = (PIMAGE_DATA_DIRECTORY)((PBYTE)(pNt->OptionalHeader.DataDirectory));
	//判断重定位表是否存在
	if ((pDir[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress == 0 || pDir[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size == 0) && (ULONG64)pNt->OptionalHeader.ImageBase == (ULONG64)pImageBuffer)
	{
		printf("不存在重定位表!\n");
		return;
	}
	PIMAGE_BASE_RELOCATION pRel = (PIMAGE_BASE_RELOCATION)((PBYTE)pFileBuffer + RVAToFOA(pFileBuffer, pDir[5].VirtualAddress));

	//判断重定位表是否需要修复
	INT64 temp;
	ULONG64 delta = (ULONG64)pImageBuffer - (ULONG64)pNt->OptionalHeader.ImageBase;
	if (delta == 0) return;
	DWORD number = 0;

	while (1)
	{
		if (pRel->SizeOfBlock == 0 && pRel->VirtualAddress == 0) break;
		//printf("%d\n", ++number);
		PWORD pItem = NULL;									//重定向表块中的项指针，2字节不断移动
		int NumberOfItems = (pRel->SizeOfBlock - 8) / 2;	// 重定向表一块中的项数
		int ItemAdd;										//重定向表一项中表示的地址变量，后续会不断变换为Rva FOA
		//printf("NumberOfItems： %lld\n", NumberOfItems);
		//printf("SizeOfBlock： %d\n", pRel->SizeOfBlock);

		pItem = (PWORD)((PBYTE)pRel + 8);
		//printf("%x\n", pItem);
		for (int i = 0; i < NumberOfItems; i++)
		{
			//printf("%d\n", i);
			//printf("%x\n", *pItem);
			if (((*pItem) & 0xF000) == 0xA000)
			{
				DWORD offset = (*pItem & 0x0FFF) + pRel->VirtualAddress;
				PULONG64 pFix = (PULONG64)((PBYTE)pImageBuffer + offset); // 这里定位的是内存镜像，所以不需要RVAToFOA
				*pFix += delta;
			}
			pItem++;
		}
		pRel = (PIMAGE_BASE_RELOCATION)((PBYTE)pRel + pRel->SizeOfBlock);
	}
	printf("重定位表修复完成!\n");
}

//创建执行线程
void CreateExecuteThread(PBYTE pImageBuffer)
{
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pImageBuffer;
	PIMAGE_NT_HEADERS64 pNt = (PIMAGE_NT_HEADERS64)(pImageBuffer + pDos->e_lfanew);
	DWORD64 entryPointRVA = pNt->OptionalHeader.AddressOfEntryPoint;
	BYTE* pEntryPoint = pImageBuffer + entryPointRVA;
	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)pEntryPoint, NULL, 0, NULL);
	if (hThread != NULL) {
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
	}
}

//初始化文件路径，判断文件路径是否含空格后，分别处理
void InitFilePath(char* FilePath)
{
	if (FilePath[0] == 0x22) ////路径中带空格
	{
		for (int i = 0; i < strlen(FilePath); i++)
		{
			FilePath[i] = FilePath[i + 1];
		}
		FilePath[strlen(FilePath) - 2] = '\0';
	}
	else
	{
		FilePath[strlen(FilePath) - 1] = '\0';
	}
}