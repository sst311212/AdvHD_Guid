#include <stdio.h>
#include <Windows.h>

#include "lua.h"

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

int GetAdvHDGuid(LPWSTR advGuid)
{
	HMODULE hModule = LoadLibrary(L"AdvHD.exe");
	if (hModule == NULL) {
		PrintErrorMessage(L"Cannot find AdvHD.exe\n");
		return -1;
	}
	int status = LoadString(hModule, 113, advGuid, 256);
	if (status == 0 || advGuid[0] != L'{') {
		PrintErrorMessage(L"Cannot find executable's Guid\n");
		return -1;
	}
	FreeLibrary(hModule);
	return 0;
}

int GetLuaScript(PBYTE& pBuff, DWORD& nSize)
{
	DWORD dwCache;
	HANDLE hFile = CreateFile(L"Script.arc", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		PrintErrorMessage(L"Cannot find Script.arc\n");
		return -1;
	}

	unsigned long long u8pac;
	ReadFile(hFile, &u8pac, sizeof(u8pac), &dwCache, NULL);
	auto entry_info = new byte[u8pac >> 32];
	ReadFile(hFile, entry_info, u8pac >> 32, &dwCache, NULL);

	auto p = entry_info;
	while (p < entry_info + dwCache) {
		auto entries = (ARCENTRY*)p;
		if (lstrcmpi(entries->filename, L"GameInfo.lua") == 0) {
			pBuff = new byte[entries->length];
			SetFilePointer(hFile, entries->offset + dwCache + 8, NULL, 0);
			ReadFile(hFile, pBuff, entries->length, &nSize, NULL);
			break;
		}
		p += (lstrlen(entries->filename) << 1) + 10;
	}
	delete[] entry_info;

	CloseHandle(hFile);
	return 0;
}

int GetLuaClsid(LPWSTR strClsid)
{
	PBYTE luaBuff;
	DWORD luaSize;
	if (GetLuaScript(luaBuff, luaSize) == -1)
		return -1;

	HMODULE hLuaMod = NULL;
	int luaVer = *(luaBuff + 4);
	if (luaVer == 0x51)
		hLuaMod = LoadLibrary(L"lua5.1.dll");
	if (luaVer == 0x53)
		hLuaMod = LoadLibrary(L"lua5.3.dll");
	if (hLuaMod == NULL) {
		PrintErrorMessage(L"Cannot load Lua library\n");
		return -1;
	}
	luaL_newstate = (_luaL_newstate)GetProcAddress(hLuaMod, "luaL_newstate");
	luaL_openlibs = (_luaL_openlibs)GetProcAddress(hLuaMod, "luaL_openlibs");
	luaL_loadbuffer = (_luaL_loadbuffer)GetProcAddress(hLuaMod, "luaL_loadbuffer");
	luaL_loadbufferx = (_luaL_loadbufferx)GetProcAddress(hLuaMod, "luaL_loadbufferx");
	lua_pcall = (_lua_pcall)GetProcAddress(hLuaMod, "lua_pcall");
	lua_pcallk = (_lua_pcallk)GetProcAddress(hLuaMod, "lua_pcallk");
	lua_getfield = (_lua_getfield)GetProcAddress(hLuaMod, "lua_getfield");
	lua_getglobal = (_lua_getglobal)GetProcAddress(hLuaMod, "lua_getglobal");
	lua_pushnil = (_lua_pushnil)GetProcAddress(hLuaMod, "lua_pushnil");
	lua_tolstring = (_lua_tolstring)GetProcAddress(hLuaMod, "lua_tolstring");

	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	if (luaVer == 0x53) {
		luaL_loadbufferx(L, (char*)luaBuff, luaSize, "luaHelper", NULL);
		lua_pcallk(L, 0, 1, 3, NULL, NULL);
		lua_getglobal(L, "getHostInfo");
		lua_pushnil(L);
		lua_pcallk(L, 1, 4, 7, NULL, NULL);
	}
	else {
		luaL_loadbuffer(L, (char*)luaBuff, luaSize, "luaHelper");
		lua_pcall(L, 0, 1, 3);
		lua_getfield(L, -10002, "getHostInfo");
		lua_pushnil(L);
		lua_pcall(L, 1, 4, 7);
	}
	const char* sXor = lua_tostring(L, -1);
	const char* sGuid = lua_tostring(L, -2);

	if (sXor == 0) {
		MultiByteToWideChar(CP_ACP, 0, sGuid, -1, strClsid, 256);
	}
	if (sXor != 0 && sGuid[0] == '{') {
		CLSID clsid;
		MultiByteToWideChar(CP_ACP, 0, sGuid, -1, strClsid, 256);
		CLSIDFromString(strClsid, &clsid);
		for (int i = 0, j = atoi(sXor); i < 16; i++)
			((PBYTE)&clsid)[i] ^= j;
		StringFromGUID2(clsid, strClsid, 256);
	}

	return 0;
}

int PatchAdvHDGuid(LPWSTR advGuid, LPWSTR strClsid)
{
	HANDLE hFile = CreateFile(L"AdvHD.exe", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
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

int main(int argc, char** argv)
{
	WCHAR advGuid[256];
	if (GetAdvHDGuid(advGuid) == -1)
		return -1;
	wprintf(L"Old Guid: %s\n", advGuid);

	WCHAR strClsid[256];
	if (GetLuaClsid(strClsid) == -1)
		return -1;
	wprintf(L"New Guid: %s\n", strClsid);

	if (PatchAdvHDGuid(advGuid, strClsid) == -1)
		return -1;

	return 0;
}