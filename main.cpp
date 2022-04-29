#include <stdio.h>
#include <Windows.h>

#include <lua.hpp>

struct ARCENTRY {
	unsigned int length;
	unsigned int offset;
	wchar_t filename[];
};

void PrintErrorMessage(LPCWSTR lpMessage)
{
	wprintf(lpMessage);
	system("PAUSE");
}

int main(int argc, char** argv)
{
	HMODULE hModule = LoadLibrary(L"AdvHD.exe");
	if (hModule == NULL) {
		PrintErrorMessage(L"Cannot find AdvHD.exe\n");
		return -1;
	}
	WCHAR advGuid[256];
	int status = LoadString(hModule, 113, advGuid, 256);
	if (status == 0 || advGuid[0] != L'{') {
		PrintErrorMessage(L"Cannot find executable's Guid\n");
		return -1;
	}
	FreeLibrary(hModule);
	wprintf(L"Old Guid: %s\n", advGuid);

	HANDLE hFile = CreateFile(L"Script.arc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		PrintErrorMessage(L"Cannot find Script.arc\n");
		return -1;
	}

	int dwReaded;
	unsigned long long u8pac;
	ReadFile(hFile, &u8pac, sizeof(u8pac), NULL, NULL);
	auto entry_info = new byte[u8pac >> 32];
	ReadFile(hFile, entry_info, u8pac >> 32, (PDWORD)&dwReaded, NULL);

	auto p = entry_info;
	while (p < entry_info + dwReaded) {
		auto entries = (ARCENTRY*)p;
		if (lstrcmpi(entries->filename, L"GameInfo.lua") == 0) {
			auto pBuff = new byte[entries->length];
			SetFilePointer(hFile, entries->offset + dwReaded + 8, NULL, 0);
			ReadFile(hFile, pBuff, entries->length, NULL, NULL);

			HANDLE hFile2 = CreateFile(entries->filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			WriteFile(hFile2, pBuff, entries->length, NULL, NULL);
			CloseHandle(hFile2);

			delete[] pBuff;
		}
		p += (lstrlen(entries->filename) << 1) + 10;
	}
	delete[] entry_info;

	CloseHandle(hFile);

	lua_State *L =  luaL_newstate();
	luaL_openlibs(L);
	luaL_loadfilex(L, "GameInfo.lua", NULL);
	lua_callk(L, 0, 0, NULL, NULL);
	lua_getglobal(L, "getHostInfo");
	lua_callk(L, 0, 4, NULL, NULL);
	const char* s3 = lua_tostring(L, 3);
	const char* s4 = lua_tostring(L, 4);
	DeleteFile(L"GameInfo.lua");

	WCHAR strClsid[256];
	MultiByteToWideChar(CP_ACP, 0, s3, -1, strClsid, 256);

	CLSID clsid;
	CLSIDFromString(strClsid, &clsid);
	for (int i = 0, j = atoi(s4); i < 16; i++)
		((PBYTE)&clsid)[i] ^= j;
	StringFromGUID2(clsid, strClsid, 256);
	wprintf(L"New Guid: %s\n", strClsid);

	hFile = CreateFile(L"AdvHD.exe", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	DWORD dwFileSize = GetFileSize(hFile, NULL);
	auto pBuff = new byte[dwFileSize];
	ReadFile(hFile, pBuff, dwFileSize, NULL, NULL);
	CloseHandle(hFile);

	int advGuidLen = lstrlen(advGuid) * 2;
	for (size_t i = 0; i < dwFileSize; i++) {
		if (memcmp(pBuff + i, advGuid, advGuidLen) == 0) {
			memcpy(pBuff + i, strClsid, advGuidLen);
			break;
		}
		if (i > dwFileSize - advGuidLen) {
			PrintErrorMessage(L"Cannot patch executable's Guid\n");
			return -1;
		}
	}

	MoveFile(L"AdvHD.exe", L"AdvHD.exe.Bak");
	hFile = CreateFile(L"AdvHD.exe", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
	WriteFile(hFile, pBuff, dwFileSize, NULL, NULL);
	CloseHandle(hFile);

	delete[] pBuff;

	return 0;
}