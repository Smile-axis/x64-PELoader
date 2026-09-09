#include "PE_Operation.h"
#include "Chack.h"


int main()
{
	char FilePath[MAX_PATH];
	LARGE_INTEGER FileSize;
	PBYTE pFileBuffer;
	PBYTE pImageBuffer;

	printf("Input FilePath： ");
	//scanf_s("%s", FilePath, MAX_PATH);
	fgets(FilePath, sizeof(FilePath), stdin);
	InitFilePath(FilePath);

	while (1)
	{
		//获取文件句柄
		HANDLE FileHandle = CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (FileHandle == INVALID_HANDLE_VALUE)
		{
			printf("打开文件失败！\n");
			printf("again Input：");
			fgets(FilePath, sizeof(FilePath), stdin);
			InitFilePath(FilePath);

			continue;
		}
		else printf("打开文件成功！\n");
		
		//获取文件大小 
		GetFileSizeEx(FileHandle, &FileSize);//此处的第二个参数返回的结构是 PLARGE_INTEGER
		size_t Size = (size_t)(((ULONGLONG)FileSize.HighPart << 32) | (FileSize.LowPart));    //高位左移动  + 高位补全 = 64位完整大小
		//printf("%lld\n", ((long long)FileSize.HighPart << 32) | (FileSize.LowPart));
		//printf("%lld\n", Size);
		
		//读取文件
		pFileBuffer = (PBYTE)VirtualAlloc(NULL, Size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);			//在进程中申请一块内存
		memset(pFileBuffer, 0, Size);																				//初始化
		DWORD NumberOfBytesRead;
		ReadFile(FileHandle, pFileBuffer, Size, &NumberOfBytesRead, NULL);//把文件读到申请的内存中

		//检查是否为合法PE文件
		if (!PEChack(pFileBuffer))
		{
			printf("该文件不是合规的 PE文件!\n");
			continue;//重新输入
		}


		//获取文件在内存中的大小
		DWORD SizeOfImage = GetSizeOfImage(pFileBuffer);
		//printf("%x\n", SizeOfImage);
		ULONG64 FileImageBase = GetImageBase(pFileBuffer);
		//printf("%llx\n", FileImageBase);

		if (ASLRChack(pFileBuffer))
		{
			//文件开启ASLR
			pImageBuffer = (PBYTE)VirtualAlloc(NULL, SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (pImageBuffer == NULL)
			{
				printf("申请内存失败!\n");
			}
		}
		else
		{
			//文件未开启ASLR
			pImageBuffer = (PBYTE)VirtualAlloc((LPVOID)FileImageBase, SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (pImageBuffer == NULL)
			{
				printf("申请内存失败!\n");
			}
		}
		//printf("%x\n", pImageBuffer);
		memset(pImageBuffer, 0, SizeOfImage);

		//解析PE文件,分配内存并映射节区
		FileBuffer_To_ImageBuffer(pFileBuffer, pImageBuffer);

		//重定位表修复
		Relocation_Descriptor_Repair(pFileBuffer, pImageBuffer);

		//导入表修复
		Import_Descriptor_Repair(pFileBuffer, pImageBuffer);

		//创建执行线程
		CreateExecuteThread(pImageBuffer);


		break;
	}
	
	system("pause");
	return 0;
}