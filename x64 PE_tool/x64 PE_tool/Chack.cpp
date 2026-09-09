#include "Chack.h"

BOOL ASLRChack(PBYTE pImageBuffer)
{
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pImageBuffer;
	PIMAGE_NT_HEADERS64 pNt = (PIMAGE_NT_HEADERS64)(pImageBuffer + pDos->e_lfanew);

	if ((pNt->OptionalHeader.DllCharacteristics & 0x0040) != 0) return TRUE;
	else
	{
		return FALSE;
	}
}
BOOL PEChack(PBYTE pFileBuffer)
{
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pFileBuffer;
	PIMAGE_NT_HEADERS64 pNt = (PIMAGE_NT_HEADERS64)(pFileBuffer + pDos->e_lfanew);
	if (pDos->e_magic == 0x5A4D && pNt->Signature == 0x4550) return TRUE;
	return FALSE;
}