//ExportXML.cpp: 定义应用程序的入口点。
//

#include "stdafx.h"
#include "ExportXML.h"
#include "CommDlg.h"
#include "Shlobj.h"

#include <shellapi.h>
#include <stdio.h>

#include <string>
#include <vector>
#include <io.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <fstream>
#include <algorithm>
#include <WindowsX.h>

//输入法相关
#include <imm.h>
#pragma comment (lib ,"imm32.lib")
//---------------------------end

#include "ErlangSocket.h"


#define MAX_LOADSTRING 100

// 全局变量: 
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

HIMC g_hIMC;

HWND g_gmdialog;
HWND g_searchBox;
HANDLE g_hThread;

#define BUFSIZE 10240

enum PreferEnum
{
	PREFER_NONE,
	PREFER_COMPILE,
	PREFER_SVN,
	PREFER_PHP,
	// 最大值
	PREFER_LAST
};

// 要记录的文件
static const char* szSavePath = "xmlexportconfig.txt";

static const int openLog = 0;
static const char* szLogName = "log.txt";

static std::string szProjectPath;
static std::string szLoadPath;
static std::string szCompilePath;
static std::string szPHPPath;
static std::string szProtoPath;
static std::string szCurSelectName;
static std::string szRootPath;
static std::string szGMPath;
static std::string szHost;
static std::string szPort;
static std::string szGMCodePath;

static const int roleIndexCode = 5000;
static const int gmstartCode = 10000;
static const int buttonCodeStart = 40000;
static const int xspace = 5;
static const int yspace = 5;
static const int buttonWidth = 160;
static const int buttonHeight = 40;
const static int max = 1024;
const static int SEARCH_X = 50;
const static int SEARCH_Y = 5;
const static int SEARCH_W = 500;
const static int SEARCH_H = 25;
const static int MAX_GM_COUNT = 50;

static const int horizonNum = 6;
int currentRow = 0;
int curGMPage = 0;
static unsigned long long int curRoleId = 0;

static const bool hasSearch = false;

// 是否按下了特殊键
static bool controlKey;
static bool shiftKey;

// 偏好记录
struct Preference
{
	std::string name;
	int clickCount;
};
std::vector<Preference> allPreferences;

// 所有按钮
std::vector<HWND> allBtns;
// 所有操作按钮
std::vector<HWND> allGMItems;

// 版本
int version = 7;
std::vector<std::string> allFiles;

// 搜索记录
static std::string searchCode;
std::vector<std::string> searchNames;

// 偏好设定
static int preferDefine;

// 与erlang通信
static ErlangSocket erlang;

// 增加的信息
std::vector<std::string> allRoles;
static int curSelectRoleIndex;
static int oldNumberRoles;

// 是否初始化了
static bool INITAPP = false;

// gm命令
struct GMCode
{
	std::string code;
	int args;
};
std::vector<GMCode> gmcodes;

void WriteLog(const char* log, bool add = true)
{
	if (openLog <= 0) return;
	std::string tmpProjectPath(szProjectPath);
	std::string path = tmpProjectPath + "\\" + szLogName;
	if (add)
	{
		std::ofstream out(path.c_str(), std::ios::app);
		out << log << std::endl;
		out.close();
	}
	else
	{
		std::ofstream out(path.c_str());
		out << log << std::endl;
		out.close();
	}
}

std::string GetLast(const std::string& name, const char* split = "\\")
{
	std::string::size_type pos = name.find_last_of(split);
	if (pos != std::string::npos)
	{
		return name.substr(pos + 1);
	}
	else
	{
		return name;
	}
}

std::string GetHead(const std::string& name, const char* split = ".")
{
	std::string::size_type pos = name.find_last_of(split);
	if (pos != std::string::npos)
	{
		return name.substr(0, pos);
	}
	else
	{
		return name;
	}
}

void WChar_tToString(const wchar_t* wchar, std::string& out)
{
	DWORD dwNum = WideCharToMultiByte(CP_OEMCP, NULL, wchar, -1, NULL, 0, NULL, FALSE);
	char *psText = new char[dwNum];
	WideCharToMultiByte(CP_OEMCP, NULL, wchar, -1, psText, dwNum, NULL, FALSE);
	out = psText;
	delete[] psText;
}

// @doc string 转 wchar_t* 外面去删除内存
wchar_t* StringToWideChar(const std::string& code)
{
	const char* pCStrKey = code.c_str();
	//第一次调用返回转换后的字符串长度，用于确认为wchar_t*开辟多大的内存空间
	int pSize = MultiByteToWideChar(CP_OEMCP, 0, pCStrKey, strlen(pCStrKey) + 1, NULL, 0);
	wchar_t *pWCStrKey = new wchar_t[pSize];
	//第二次调用将单字节字符串转换成双字节字符串
	MultiByteToWideChar(CP_OEMCP, 0, pCStrKey, strlen(pCStrKey) + 1, pWCStrKey, pSize);
	return pWCStrKey;
}

std::string UTF8ToGB(const char* str, WCHAR *strSrc, LPSTR szRes)
{
	std::string result;

	//获得临时变量的大小
	int i = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	memset(strSrc, 0, i + 1);
	MultiByteToWideChar(CP_UTF8, 0, str, -1, strSrc, i);

	//获得临时变量的大小
	i = WideCharToMultiByte(CP_ACP, 0, strSrc, -1, NULL, 0, NULL, NULL);
	memset(szRes, 0, i + 1);
	WideCharToMultiByte(CP_ACP, 0, strSrc, -1, szRes, i, NULL, NULL);

	result = szRes;
	return result;
}


std::string StringToUTF8(const std::string & str)
{
	int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);

	wchar_t * pwBuf = new wchar_t[nwLen + 1];//一定要加1，不然会出现尾巴
	ZeroMemory(pwBuf, nwLen * 2 + 2);

	::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), pwBuf, nwLen);

	int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);

	char * pBuf = new char[nLen + 1];
	ZeroMemory(pBuf, nLen + 1);

	::WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);

	std::string retStr(pBuf);

	delete[]pwBuf;
	delete[]pBuf;

	pwBuf = NULL;
	pBuf = NULL;

	return retStr;
}

bool CreateBat(const std::string& cmds, const char* tmpPath)
{
	FILE *f = nullptr;
	fopen_s(&f, tmpPath, "w");
	{
		fprintf(f, "%s", cmds.c_str());
		fclose(f);
		return true;
	}
	return false;
}

bool CreateRunBat(const std::string& cmds, const char* batExt = "tmp.bat")
{
	std::string tmpPath = szProjectPath + "\\";
	tmpPath += batExt;
	FILE *f = nullptr;
	fopen_s(&f, tmpPath.c_str(), "w");
	if (f)
	{
		fprintf(f, "%s", cmds.c_str());
		fclose(f);
		system(tmpPath.c_str());
		remove(tmpPath.c_str());
		return true;
	}
	else
	{
		return false;
	}
}

bool CreateRunTmpBat(const char* bat, const std::string& path, const char* mod = "call")
{
	std::string cmds("@echo off\n");
	cmds += "cd /d " + path;
	//cmds += "\ncall ";
	cmds += "\n";
	cmds += mod;
	cmds += " ";
	cmds += bat;
	return CreateRunBat(cmds);
}

PreferEnum GetPreferEnum(size_t type)
{
	switch (type)
	{
		case ID_PRE_EXP_COMPILE:
			return PreferEnum::PREFER_COMPILE;
		case ID_PRF_EXP_SVN:
			return PreferEnum::PREFER_SVN;
		case ID_PREPHP_START:
			return PreferEnum::PREFER_PHP;
		default:
			return PreferEnum::PREFER_NONE;
	}
}

size_t GetIDC(PreferEnum type)
{
	switch (type)
	{
	case PreferEnum::PREFER_COMPILE:
		return ID_PRE_EXP_COMPILE;
	case PreferEnum::PREFER_SVN:
		return ID_PRF_EXP_SVN;
	case PreferEnum::PREFER_PHP:
		return ID_PREPHP_START;
	default:
		return 0;
	}
}

void SetIDC(HWND hWnd, int Idc, const std::string& code)
{
	HWND host = GetDlgItem(hWnd, Idc);
	wchar_t* buffer = StringToWideChar(code);
	SetWindowText(host, buffer);
	delete buffer;
}

void AddPreferDefine(PreferEnum value)
{
	preferDefine |= value;
}

/**
* 移除Shader宏定义。
* @param value 宏定义。
*/
void RemovePreferDefine(PreferEnum value)
{
	preferDefine &= ~value;
}

bool HasPrefer(PreferEnum type)
{
	return (preferDefine & type) == type;
}

void ChangePreferDefine(HWND hWnd, size_t idc)
{
	PreferEnum type = GetPreferEnum(idc);
	if (type > 0)
	{
		if (HasPrefer(type))
		{
			RemovePreferDefine(type);
			//CheckDlgButton(hWnd, idc, FALSE);
			//((CButton*)GetDlgItem(hWnd, idc))->SetCheck(FALSE);
		}
		else
		{
			AddPreferDefine(type);
			//CheckDlgButton(hWnd, idc, TRUE);
			//GetDlgItem(hWnd, idc)->SetCheck(TRUE);
		}
	}
}

// 启动PHP
void StartPHPStudy()
{
	if (szPHPPath != "")
	{
		//ShellExecute(NULL, L"open", LPCWSTR(szPHPPath.c_str()), NULL, NULL, SW_SHOWDEFAULT);
		WriteLog("open php!!!");
		WriteLog(szPHPPath.c_str());
		WinExec(szPHPPath.c_str(), SW_NORMAL);
		//ShellExecute(NULL, L"open", LPCWSTR(szPHPPath.c_str()), NULL, L"D:\\phpStudy\\", SW_SHOW);
		std::string dir = GetHead(szPHPPath, "\\");
		std::string cmd("TASKKILL /F /IM ");
		cmd += szPHPPath;
		cmd += "\n start";
		CreateRunTmpBat(szPHPPath.c_str(), dir, cmd.c_str());
	}
}

void SetPreferSelect(HWND hWnd, UINT type, BOOL value)
{
	EnableWindow(GetDlgItem(hWnd, type), value);
}

void SetPrefer(HWND hwnd, PreferEnum type, UINT value)
{
	size_t idc = 0;
	switch (type)
	{
	case PreferEnum::PREFER_COMPILE:
		idc = ID_PRE_EXP_COMPILE;
		break;
	case PreferEnum::PREFER_PHP:
		idc = ID_PREPHP_START;
		break;
	case PreferEnum::PREFER_SVN:
		idc = ID_PRF_EXP_SVN;
		break;
	default:
		idc = 0;
	}

	if (idc > 0)
	{
		EnableWindow(GetDlgItem(hwnd, idc), value);
		SetPreferSelect(hwnd, idc, value);
	}
}

// 设置默认启动项
void DefaultPrefer(HWND hwnd)
{
	for (size_t i = 0; i < PreferEnum::PREFER_LAST; ++i)
	{
		PreferEnum type = static_cast<PreferEnum>(i);
		SetPrefer(hwnd, type, HasPrefer(type));
	}
}

// 弹出消息窗口自动关闭，需要指出的是，Windows 2000的user32.dll没有导出这个函数。
extern "C"
{
	int WINAPI MessageBoxTimeoutA(IN HWND hWnd, IN LPCSTR lpText, IN LPCSTR lpCaption, IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
	int WINAPI MessageBoxTimeoutW(IN HWND hWnd, IN LPCWSTR lpText, IN LPCWSTR lpCaption, IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
};
#ifdef UNICODE
#define MessageBoxTimeout MessageBoxTimeoutW
#else
#define MessageBoxTimeout MessageBoxTimeoutA
#endif

void GetTChar2Char(const TCHAR* name, std::string& out)
{
	int nLen = WideCharToMultiByte(CP_ACP, 0, name, -1, NULL, 0, NULL, NULL);

	if (nLen <= 0) return;

	char* pszDst = new char[nLen];
	if (NULL == pszDst) return;

	WideCharToMultiByte(CP_ACP, 0, name, -1, pszDst, nLen, NULL, NULL);
	pszDst[nLen - 1] = 0;

	out = pszDst;
	delete[] pszDst;

}

int IncludeChinese(char *str)//返回0：无中文，返回1：有中文
{
	char c;
	while (1)
	{
		c = *str++;
		if (c == 0) break;  //如果到字符串尾则说明该字符串没有中文字符
		if (c & 0x80)        //如果字符高位为1且下一字符高位也是1则有中文字符
			if (*str & 0x80) return 1;
	}
	return 0;
}

void Write(const std::string& s, FILE* hFile)
{
	size_t szStr = s.size();
	fwrite(&szStr, sizeof(int), 1, hFile);
	if (szStr <= 0) return;
	fwrite(&s.front(), sizeof(char), szStr, hFile);
}

void Read(std::string& s, FILE* pFile)
{
	s = "";

	size_t szChar = sizeof(char);
	size_t strip = sizeof(int);

	int szSize = 0;
	fread(&szSize, strip, 1, pFile);
	for (int k = 0; k < szSize; ++k)
	{
		char a;
		fread(&a, szChar, 1, pFile);
		s.push_back(a);
	}
}

void GetFiles(std::string path, std::vector<std::string>& files)
{
	//文件句柄  
	long   hFile = 0;
	//文件信息  
	struct _finddata_t fileinfo;
	std::string p;
	if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			//如果是目录,迭代之  
			//如果不是,加入列表  
			if ((fileinfo.attrib &  _A_SUBDIR))
			{
				if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
					GetFiles(p.assign(path).append("\\").append(fileinfo.name), files);
			}
			else
			{
				if (IncludeChinese(fileinfo.name) == 0)
				{
					files.push_back(fileinfo.name);
				}
			}
		} while (_findnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
}

Preference& GetPrefrence(const std::string& name)
{
	for (size_t i = 0; i < allPreferences.size(); ++i)
	{
		if (allPreferences[i].name == name) return allPreferences[i];
	}
	Preference tmp;
	tmp.name = name;
	tmp.clickCount = 0;
	allPreferences.push_back(tmp);
	return allPreferences[allPreferences.size() - 1];
}

bool ComparePrefrence(const std::string& a, const std::string& b)
{
	Preference& ap = GetPrefrence(a);
	Preference& bp = GetPrefrence(b);
	return ap.clickCount > bp.clickCount;
}

void ReLayout(std::vector<std::string>& files, HWND hwnd, HINSTANCE hinstance, int row = horizonNum)
{
	size_t i = 0;
	for (i = 0; i < allBtns.size(); ++i) DestroyWindow(allBtns[i]);
	allBtns.clear();
	sort(files.begin(), files.end(), ComparePrefrence);
	int curY = 0;
	int curX = -1;
	for (i = 0; i < files.size(); ++i)
	{
		std::string& fileName = files[i];
		if (curX++ >= row - 1)
		{
			curX = 0;
			++curY;
		}
		int x = xspace * (curX + 1) + buttonWidth * curX;
		int y = yspace * (curY + 1) + buttonHeight * curY + SEARCH_Y * 2 + SEARCH_H;

		wchar_t* buffer = StringToWideChar(fileName);

		int code = buttonCodeStart + i;
		HWND btn = CreateWindow(L"Button", buffer, WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
			x, y, buttonWidth, buttonHeight, hwnd, (HMENU)code, hinstance, NULL);
		allBtns.push_back(btn);

		delete buffer;
	}
}

void RelayoutGM(std::vector<GMCode>& codes)
{
	size_t i = 0;
	for (i = 0; i < allGMItems.size(); ++i) DestroyWindow(allGMItems[i]);
	allGMItems.clear();

	wchar_t* buffer = new wchar_t[max];

	const static int w = 100;
	const static int h = 20;
	const static int bw = 60;
	const static int argsw = 100;

	const static int startY = 130;
	const static int startX = 10;

	const static int twoStartX = 500;

	const static int rowSpace = 5;
	const static int colSpace = 5;
	size_t tmpSize = 0;

	int curCol = 0; // 当前第几列
	int curRow = 0; // 当前第几行
	int resetSize = max;

	int x = 0, y = 0, codeStart = 0, nowCode = 0, nowIndex = 0;

	int startIndex = MAX_GM_COUNT * curGMPage;
	int size = codes.size();
	int endIndex = startIndex + MAX_GM_COUNT;
	endIndex = endIndex >= size ? size: endIndex;

	for (int f = startIndex; f < endIndex; ++f)
	{
		GMCode& gm = codes[f];

		i = f - startIndex;
		nowIndex = 0;
		codeStart = gmstartCode + i * 100;
		nowCode = codeStart + nowIndex;

		curRow = static_cast<int>(i / 2);
		curCol = i % 2;

		HWND btn = 0;

		y = startY + curRow * (h + rowSpace);
		x = curCol == 0 ? startX + nowIndex * (w + colSpace) : twoStartX + nowIndex * (w + colSpace);


		memset(buffer, 0, resetSize);

		tmpSize = gm.code.size();
		resetSize = tmpSize + 1;
		MultiByteToWideChar(CP_ACP, 0, gm.code.c_str(), tmpSize, buffer, tmpSize * sizeof(wchar_t));
		buffer[tmpSize + 1] = 0;
		btn = CreateWindow(L"Button", buffer, WS_BORDER | WS_VISIBLE | WS_CHILD,
			x, y, w, h, g_gmdialog, (HMENU)nowCode, hInst, NULL);
		allGMItems.push_back(btn);

		for (int j = 0; j < gm.args; ++j)
		{
			nowCode++;
			nowIndex++;

			y = startY + curRow * (h + rowSpace);
			x = curCol == 0 ? startX + nowIndex * (w + colSpace) : twoStartX + nowIndex * (w + colSpace);

			btn = CreateWindow(L"Edit", _T(""), WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
				x, y, argsw, h, g_gmdialog, (HMENU)nowCode, hInst, NULL);
			allGMItems.push_back(btn);
		}

		/*nowCode++;
		nowIndex++;
		y = startY + curRow * (h + rowSpace);
		x = curCol == 0 ? startX + nowIndex * (w + colSpace) : twoStartX + nowIndex * (w + colSpace);
		btn = CreateWindow(L"Button", _T("确定"), WS_VISIBLE | WS_CHILD,
			x, y, bw, h, g_gmdialog, (HMENU)nowCode, hInst, NULL);
		allGMItems.push_back(btn);*/
	}

	delete buffer;
}

void GetAllFiles(const char* path, HWND hwnd, HINSTANCE hinstance, int row = horizonNum)
{
	if (path && strcmp(path, "") != 0)
	{
		size_t i = 0;

		allFiles.clear();
		GetFiles(path, allFiles);

		ReLayout(allFiles, hwnd, hinstance, row);
	}
}

bool ParseLod(FILE* pFile, const char* ext)
{
	if (strcmp(ext, szSavePath) == 0)
	{
		// 版本号
		int nowVer = 0;
		fread(&nowVer, sizeof(int), 1, pFile);
		// 存储路径
		Read(szLoadPath, pFile);
		// 偏好设定
		int size = 0;
		fread(&size, sizeof(int), 1, pFile);
		allPreferences.clear();
		for (int i = 0; i < size; ++i)
		{
			std::string tmpName("");
			int tmpCount = 0;
			Read(tmpName, pFile);
			fread(&tmpCount, sizeof(int), 1, pFile);
			Preference tmp;
			tmp.name = tmpName;
			tmp.clickCount = tmpCount;
			allPreferences.push_back(tmp);
		}
		if (nowVer > 1) Read(szCompilePath, pFile);
		if (nowVer > 2) Read(szPHPPath, pFile);
		if (nowVer > 3) fread(&preferDefine, sizeof(int), 1, pFile);
		if (nowVer > 4) Read(szProtoPath, pFile);
		if (nowVer > 5) Read(szGMPath, pFile);
		if (nowVer > 6) Read(szGMCodePath, pFile);
		return false;
	}
	else
	{
		Read(szLoadPath, pFile);
		return true;
	}
}

bool CheckLoadCfg(HWND hWnd, const char* ext)
{
	std::string tmpLoadPath = szProjectPath + "\\" + ext;
	FILE* pFile = nullptr;
	fopen_s(&pFile, tmpLoadPath.c_str(), "rb+");
	if (pFile)
	{
		// 解析文件
		ParseLod(pFile, ext);
		//if (del) remove(tmpLoadPath.c_str());

		fclose(pFile);
		WriteLog("open_path_success!!");
		WriteLog(szLoadPath.c_str());
		return true;
	}
	else
	{
		WriteLog("open_path_fail!!");
		return false;
	}
}

std::string GetPreFolder(size_t count = 2)
{
	std::string tmp(szProjectPath);
	for (size_t i = 0; i < count; ++i)
	{
		std::string tmp1 = GetLast(tmp);
		tmp = tmp.substr(0, tmp.size() - tmp1.size() - 1);
	}
	return tmp;
}

void LoadCfg(HWND hWnd)
{
	char cBuf[MAX_PATH];
	LPWSTR pBuf = (LPWSTR)cBuf;
	GetCurrentDirectory(MAX_PATH, pBuf);                   //获取程序的当前目录
	WChar_tToString(pBuf, szProjectPath);

	WriteLog("create_success!!", false);

	szRootPath = GetPreFolder();

	if (!CheckLoadCfg(hWnd, szSavePath))
	{
		if (!CheckLoadCfg(hWnd, "path.txt"))
		{
			szLoadPath = szRootPath;
			szLoadPath += "\\xml";
			WriteLog("set_default_path");
			WriteLog(szLoadPath.c_str());
		}
	}

	if (szCompilePath == "")
	{
		szCompilePath = szRootPath;
		szCompilePath += "\\server\\bin";
	}

	if (szProtoPath == "")
	{
		std::string prePath = GetPreFolder(1);
		szProtoPath = prePath + "\\protocol";
	}

	if (szGMPath == "")
	{
		szGMPath = szRootPath;
		szGMPath += "\\server\\bin\\app.cfg";
		//szGMPath = szProjectPath + "\\app.cfg";
	}

	if (szGMCodePath == "")
	{
		szGMCodePath = szRootPath;
		szGMCodePath += "\\server\\src\\mod\\misc\\gm.erl";
		//szGMCodePath = szProjectPath + "\\gm.erl";
	}
}

int CalcRow(HWND hWnd)
{
	RECT rect;
	//获取客户区域大小     
	GetClientRect(hWnd, &rect);
	double sw = rect.right - rect.left;
	return static_cast<int>(sw / buttonWidth);
}

void Reload(HWND hWnd, bool calcRow = false)
{
	int row = CalcRow(hWnd);
	if (calcRow == false || row != currentRow)
	{
		currentRow = row;
		GetAllFiles(szLoadPath.c_str(), hWnd, hInst, row);
	}
}

void WriteSave(HWND hWnd)
{
	INITAPP = false;

	FILE* hFile = nullptr;
	std::string savePath = szProjectPath + "\\" + szSavePath;
	fopen_s(&hFile, savePath.c_str(), "wb");
	// 版本号
	fwrite(&version, sizeof(int), 1, hFile);
	// 保存路径
	Write(szLoadPath, hFile);
	// 写入偏好
	size_t len = allPreferences.size();
	fwrite(&len, sizeof(int), 1, hFile);
	for (size_t i = 0; i < len; ++i)
	{
		std::string tmpname = allPreferences[i].name;
		int tmpCount = allPreferences[i].clickCount;
		Write(tmpname, hFile);
		fwrite(&tmpCount, sizeof(int), 1, hFile);
	}
	// 写入编译目录
	Write(szCompilePath, hFile);
	// 写入PHP文件
	Write(szPHPPath, hFile);
	// 写入偏好设定
	fwrite(&preferDefine, sizeof(int), 1, hFile);
	// 写入协议目录
	Write(szProtoPath, hFile);
	// 写入GM路径
	Write(szGMPath, hFile);
	// 写入GM命令路径
	Write(szGMCodePath, hFile);
	fclose(hFile);
	if (g_hThread)
	{
		CloseHandle(g_hThread);
		g_hThread = nullptr;
	}
	if(g_searchBox) DestroyWindow(g_searchBox);
	if(g_gmdialog) DestroyWindow(g_gmdialog);
	DestroyWindow(hWnd);
}

void Search(std::vector<std::string>& source, std::vector<std::string>& out)
{
	std::string::size_type idx = std::string::npos;
	for (size_t i = 0; i < source.size(); ++i)
	{
		std::string tmp = source[i];
		idx = tmp.find(searchCode.c_str());
		if (idx != std::string::npos) out.push_back(tmp);
	}
}

void ShowTimeoutBox(const std::string& code, const wchar_t* title, int timeout = 500)
{
	wchar_t* buffer = StringToWideChar(code);
	MessageBoxTimeout(NULL, buffer, title, MB_ICONINFORMATION, GetSystemDefaultLangID(), timeout);
	delete buffer;
}

void Search(HWND hwnd, HINSTANCE hinstance)
{
	if (searchCode != "")
	{
		//ShowTimeoutBox(searchCode, _T("当前搜索"));
		//HWND hStatic = GetDlgItem(g_searchBox, IDC_SHOW_SEARCH_TXT);
		wchar_t* buffer = StringToWideChar(searchCode);
		SetWindowText(g_searchBox, buffer);
		delete buffer;
		
		searchNames.clear();
		Search(allFiles, searchNames);
		ReLayout(searchNames, hwnd, hinstance, currentRow);
	}
	else
	{
		/*ShowWindow(g_searchBox, SW_HIDE);
		DestroyWindow(g_searchBox);
		g_searchBox = 0;*/
		SetWindowText(g_searchBox, L"");
		ReLayout(allFiles, hwnd, hInst, currentRow);
	}
}

void ClearSearch(HWND hwnd, HINSTANCE hinstance)
{
	searchCode = "";
	searchNames.clear();
	Search(hwnd, hinstance);
	ReLayout(allFiles, hwnd, hinstance, currentRow);
}

std::string GetSelectFolder(HWND hWnd, HINSTANCE hinstance, const std::string& defaultStr)
{
	WriteLog("select dir success!!");
	TCHAR szBuffer[MAX_PATH] = { 0 };
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(BROWSEINFO));
	bi.hwndOwner = NULL;
	bi.pszDisplayName = szBuffer;
	bi.lpszTitle = _T("从下面选文件夹目录:");
	bi.ulFlags = BIF_RETURNFSANCESTORS;
	LPITEMIDLIST idl = SHBrowseForFolder(&bi);
	if (NULL != idl)

	{
		SHGetPathFromIDList(idl, szBuffer);
		std::string tmpLoadPath;
		GetTChar2Char(szBuffer, tmpLoadPath);

		WriteLog("dir open ！！！");
		WriteLog(tmpLoadPath.c_str());
		WriteLog("=====================================");
		return tmpLoadPath;
	}
	return defaultStr;
}

std::string GetSelectFile()
{
	OPENFILENAME ofn;      // 公共对话框结构。     
	TCHAR szFile[MAX_PATH]; // 保存获取文件名称的缓冲区。               
							// 初始化选择文件对话框。     
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = _T("All(*.*)");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	//ofn.lpTemplateName =  MAKEINTRESOURCE(ID_TEMP_DIALOG);    
	// 显示打开选择文件对话框。

	std::string name("");
	if (GetOpenFileName(&ofn))
	{
		//显示选择的文件。 
		//std::cout << szFile << std::endl;
		GetTChar2Char(szFile, name);
		OutputDebugString(szFile);    //这一句是显示路径吗？为什么不显示？
		OutputDebugString((LPCWSTR)"\r\n");
	}
	return name;
}

void CreateMakeErlang()
{
	std::string tmpMakeCmds("rem del ebin\\*.beam\n");
	tmpMakeCmds += "erl +K true -smp auto -pa ..\\ebin -s make all -s c q";
	FILE *f = nullptr;
	std::string tmpMakePath(szCompilePath);
	tmpMakePath += "\\tmp_make.bat";
	fopen_s(&f, tmpMakePath.c_str(), "w");
	if (f)
	{
		fprintf(f, "%s", tmpMakeCmds.c_str());
		fclose(f);

		CreateRunTmpBat("tmp_make.bat", szCompilePath);
		remove(tmpMakePath.c_str());
	}
}

// svn 自动操作
void SVNAUTO()
{
	if (szCurSelectName == "") return;

	std::string proj = szRootPath;

	std::string dataPath(proj);
	dataPath += "\\server\\src\\data\\";
	dataPath += szCurSelectName + "_data.erl";

	std::string hrlPath(proj);
	hrlPath += "\\server\\inc\\data\\";
	hrlPath += szCurSelectName + "_data.hrl";

	std::string cliPath(proj);
	cliPath += "\\client\\Game006\\Lua\\src\\EData\\";
	
	std::string fa = szCurSelectName.substr(0, 1);
	std::string fb = szCurSelectName.substr(1, szCurSelectName.size());
	transform(fa.begin(), fa.end(), fa.begin(), ::toupper);

	cliPath += fa + fb + "EData.lua";

	std::string cmds("@echo off");
	cmds += "\nTortoiseProc.exe /command:update /path:";
	cmds += proj + " /closeonend:2";

	// add
	cmds += "\nTortoiseProc.exe /command:add /path:\"";
	cmds += dataPath + "\"";

	cmds += "\nTortoiseProc.exe /command:add /path:\"";
	cmds += hrlPath + "\"";

	cmds += "\nTortoiseProc.exe /command:add /path:\"";
	cmds += cliPath + "\"";

	// commit
	cmds += "\nTortoiseProc.exe /command:commit /path:\"";
	cmds += dataPath + "\" -m /logmsg:\"";
	cmds += szCurSelectName + "数据提交\"";

	cmds += "\nTortoiseProc.exe /command:commit /path:\"";
	cmds += hrlPath + "\" -m /logmsg:\"";
	cmds += szCurSelectName + "数据提交\"";

	cmds += "\nTortoiseProc.exe /command:commit /path:\"";
	cmds += cliPath + "\" -m /logmsg:\"";
	cmds += szCurSelectName + "数据提交\"";
	
	CreateRunBat(cmds);
}

void ProjectCompile()
{
	if(!CreateRunTmpBat("make.bat", szCompilePath)) ShowTimeoutBox("编译出错", L"编译", 3000);
}

void ProjectStart(bool center = false)
{
	if (center)
	{
		if (!CreateRunTmpBat("main2.bat", szCompilePath)) ShowTimeoutBox("启动中央服出错", _T("启动中央服"), 3000);
	}
	else
	{
		if (!CreateRunTmpBat("main.bat", szCompilePath)) ShowTimeoutBox("启动本地服出错", _T("启动本地服"), 3000);
	}
}

void DoAuto(PreferEnum type)
{
	if (HasPrefer(type))
	{
		switch (type)
		{
		case PreferEnum::PREFER_PHP:
			StartPHPStudy();
			break;
		case PreferEnum::PREFER_COMPILE:
			ProjectCompile();
			break;
		case PreferEnum::PREFER_SVN:
			SVNAUTO();
			break;
		}
	}
}

// svn 操作
std::string CreateSVNCommond(const char* command, const char* file)
{
	std::string tmp("TortoiseProc.exe /command:");
	tmp += command;
	tmp += " /path:";
	tmp += file;
	tmp += " /closeonend:2";
	return tmp;
}
void DoSVN(const char* command, const char* file)
{
	std::string tmp = CreateSVNCommond(command, file);
	CreateRunBat(tmp);
}
void DoSVN(const char* command)
{
	DoSVN(command, szRootPath.c_str());
}

// 协议
void ExportProto()
{
	if (!CreateRunTmpBat("gen.bat", szProtoPath)) ShowTimeoutBox("导出协议出错", _T("导出协议"), 3000);
}

// 输入法管理
void ChineseController(HWND wnd, bool in = false)
{
	if (in)
	{
		//focus on		
		g_hIMC = ::ImmGetContext(wnd);
		if (g_hIMC)
		{
			ImmAssociateContext(wnd, nullptr);
			ImmReleaseContext(wnd, g_hIMC);
		}

	}
	else
	{
		//focus out
		if (g_hIMC)
		{
			ImmDestroyContext(g_hIMC);
			g_hIMC = nullptr;
		}

		g_hIMC = ImmCreateContext();
		if (g_hIMC)
		{
			ImmAssociateContext(wnd, g_hIMC);
			ImmReleaseContext(wnd, g_hIMC);
		}
	}
}

// start 某项
void Start(const char* path, const char* file = nullptr)
{
	std::string tmp("start ");
	tmp += path;
	if (file)
	{
		tmp += "\\";
		tmp += file;
	}
	CreateRunBat(tmp);
}

void DoXML(HWND hWnd, size_t wmId)
{
	size_t index = wmId - buttonCodeStart;

	std::vector<std::string>& vec = searchNames;
	if (vec.size() <= 0) vec = allFiles;

	if (index < vec.size())
	{
		std::string& fileName = vec[index];
		
		if (!controlKey)
		{
			Preference& ref = GetPrefrence(fileName);
			ref.clickCount += 1;

			std::string last = GetLast(fileName, ".");
			szCurSelectName = fileName.substr(0, fileName.size() - last.size() - 1);

			std::string tmp("@echo off\necho -------------------------------------------------------\necho 正在生成数据文件...\necho ------------------------------------------------------\n");
			tmp += "php.exe base/make.php ";
			tmp += fileName;
			tmp += "\npause\n";
			CreateRunBat(tmp);

			//ClearSearch(hWnd, hInst);
			ShowTimeoutBox(szCurSelectName, _T("当前导出文件"), 2000);

			if (shiftKey)
			{
				SVNAUTO();
				ProjectCompile();
			}
			else
			{
				DoAuto(PreferEnum::PREFER_SVN);
				DoAuto(PreferEnum::PREFER_COMPILE);
			}

			szCurSelectName = "";
		}
		else
		{
			std::string filepath = szLoadPath + "\\" + fileName;
			std::string tmp = CreateSVNCommond("update", filepath.c_str());
			tmp += "\nstart ";
			tmp += filepath + "";
			CreateRunBat(tmp);
		}
	}
}

void OpenProtoDir()
{
	Start(szProtoPath.c_str());
}

void OpenConPHP()
{
	Start(szProjectPath.c_str(), "conf.php");
}

void OpenProjDir()
{
	Start(szRootPath.c_str());
}

// 强行清除
void ClearSVN()
{
	std::string tmp("@echo off");
	tmp += "\ncd /d ";

	std::string pro = szRootPath;
	pro += "\\.svn";

	tmp += pro;
	tmp += "\nsqlite3.exe wc.db \"delete from work_queue\"";
	CreateRunBat(tmp);

}

void GetSocketCfg(const std::string& szLine, const std::string& val, const char* host, std::string& out)
{
	std::string::size_type pos = szLine.find("{");
	std::string tmp("");
	if (pos != std::string::npos)
	{
		tmp = szLine.substr(pos + 1);
		if (strcmp(tmp.c_str(), host) == 0)
		{
			pos = val.find(',');
			if (pos != std::string::npos)
			{
				tmp = val.substr(pos+1);

				pos = tmp.find('\"');
				if (pos != std::string::npos)
				{
					tmp = tmp.substr(pos + 1);
					pos = tmp.find('\"');
					if (pos != std::string::npos)
					{
						tmp = tmp.substr(0, pos);
					}
				}

				pos = tmp.find("}");
				if (pos != std::string::npos)
				{
					out = tmp.substr(0, pos);
				}
				else
				{
					out = tmp;
				}
			}
		}
	}
}

// 读取socket数据
void LoadTCPSocket()
{
	if (szGMPath != "")
	{
		std::ifstream fin(szGMPath.c_str(), std::ios::in);
		const char* host = "host";
		const char* gm_port = "gm_port";
		std::string szLine("");
		std::string szTmp("");
		std::string tmp("");
		char line[max] = { 0 };
		while (fin.getline(line, sizeof(line)))
		{
			std::stringstream word(line);
			word >> szLine;
			word >> szTmp;
			GetSocketCfg(szLine, szTmp, host, szHost);
			GetSocketCfg(szLine, szTmp, gm_port, szPort);
		}
		fin.clear();
		fin.close();
	}
	else
	{
		szHost = "";
		szPort = "";
	}

	SetIDC(g_gmdialog, IDC_EDIT_HOST, szHost);
	SetIDC(g_gmdialog, IDC_EDIT_PORT, szPort);
}

void SplitString(const std::string& s, std::vector<std::string>& v, const char* c)
{
	std::string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (std::string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + strlen(c);
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));
}


void ParseGMCodes(std::string& str, std::vector<GMCode>& codes, std::vector<std::string>& tmpv, WCHAR *strSrc, LPSTR szRes)
{
	std::string tmp("[\"=");
	std::string::size_type pos1 = str.find(tmp);
	if (pos1 != std::string::npos)
	{
		str = str.substr(pos1 + tmp.size(), str.size());
		tmp = "]";
		pos1 = str.find(tmp);
		if (pos1 != std::string::npos)
		{
			std::string code = str.substr(0, pos1);
			tmpv.clear();
			SplitString(code, tmpv, ",");
			size_t count = tmpv.size();
			if (count > 0)
			{
				GMCode gm;
				std::string tmpstr = UTF8ToGB((tmpv[0]).c_str(), strSrc, szRes);
				pos1 = tmpstr.find("\"");
				if(pos1 != std::string::npos) gm.code = tmpstr.substr(0, pos1); // 去掉引号
				else gm.code = tmpstr;
				gm.args = count - 1;
				codes.push_back(gm);
			}
			str = str.substr(pos1 + 1, str.size());
			ParseGMCodes(str, codes, tmpv, strSrc, szRes);
		}
	}
}

void ParseGMCodes()
{
	if (szGMCodePath != "")
	{
		gmcodes.clear();
		std::ifstream fin(szGMCodePath.c_str(), std::ios::in);
		std::string szLine("");
		std::vector<std::string> v;
		WCHAR *strSrc = new WCHAR[max];
		LPSTR szRes = new CHAR[max];
		while (getline(fin, szLine))
		{
			ParseGMCodes(szLine, gmcodes, v, strSrc, szRes);
		}
		delete[]strSrc;
		delete[]szRes;
		fin.clear();
		fin.close();

		RelayoutGM(gmcodes);
	}
}

void SetSelectRole(int i)
{
	if (i >= roleIndexCode && i < gmstartCode)
	{
		i -= roleIndexCode;
		if (allRoles.size() > i)
		{
			curSelectRoleIndex = i;
			std::string& name = allRoles[i];
			SetIDC(g_gmdialog, IDC_EDIT_CUR_ROLE, name);
			std::string::size_type pos1 = name.find(" ");
			std::string tmp = name.substr(0, pos1);
			curRoleId = std::atoll(tmp.c_str());
		}
		else
		{
			std::string name("");
			SetIDC(g_gmdialog, IDC_EDIT_CUR_ROLE, name);
		}
	}
}

void SetLinkEnable(bool linkable)
{
	EnableWindow(GetDlgItem(g_gmdialog, IDC_BUTTON_CON), linkable);
}

void ParseRoles(const std::string& recv)
{
	SetLinkEnable(false);
	if (recv != "linked")
	{
		curRoleId = 0;
		allRoles.clear();

		WCHAR *strSrc = new WCHAR[max];
		LPSTR szRes = new CHAR[max];
		std::string utf8Str = UTF8ToGB(recv.c_str(), strSrc, szRes);
		delete[]strSrc;
		delete[]szRes;

		SplitString(utf8Str, allRoles, "\n");
		SetSelectRole(roleIndexCode);
	}
}

/**
* 在一个新的线程里面接收数据
*/
DWORD WINAPI Fun(LPVOID lpParamter)
{
	SOCKET* sockConn0 = (SOCKET*)lpParamter;
	SOCKET sockConn = *sockConn0;
	char recvBuf[max];
	bool GetData = true;
	while (GetData && INITAPP) {
		memset(recvBuf, 0, sizeof(recvBuf));
		try
		{
			int recvResult = recv(sockConn, recvBuf, sizeof(recvBuf), 0);
			if (recvResult > 0) ParseRoles(recvBuf);
			else GetData = false;
		}
		catch (std::exception&)
		{
			WriteLog("recive recive error");
		}
	}
	closesocket(sockConn);
	SetLinkEnable(true);
	if (g_hThread)
	{
		CloseHandle(g_hThread);
		g_hThread = nullptr;
	}
	return 0;
}

// gm面板操作
void DoGM(int code)
{
	if (code >= gmstartCode)
	{
		if (curRoleId > 0)
		{
			int gmIndex = code - gmstartCode;
			int gmMod = gmIndex % 100;
			gmIndex /= 100;
			int startIndex = curGMPage * MAX_GM_COUNT;
			if (gmMod == 0 && gmIndex < gmcodes.size())
			{
				GMCode& gm = gmcodes[startIndex + gmIndex];
				std::string gmCode = std::to_string(curRoleId);
				gmCode += " =";
				gmCode += gm.code;
				wchar_t *buffer = new wchar_t[max];
				char* pCStrKey = new char[max];
				size_t preBuffSize = max;
				size_t preKeySize = max;
				for (int i = 0; i < gm.args; ++i)
				{
					memset(buffer, 0, preBuffSize);
					memset(pCStrKey, 0, preKeySize);
					gmCode += " ";
					GetDlgItemText(g_gmdialog, code + i + 1, buffer, max);
					/// 第一次调用确认转换后单字节字符串的长度，用于开辟空间
					preBuffSize = wcslen(buffer);
					int pSize = WideCharToMultiByte(CP_OEMCP, 0, buffer, preBuffSize, NULL, 0, NULL, NULL);
					//第二次调用将双字节字符串转换成单字节字符串
					if (pSize == 0)
					{
						pSize = 1;
						pCStrKey[0] = '0';
					}
					preKeySize = pSize + 1;
					WideCharToMultiByte(CP_OEMCP, 0, buffer, wcslen(buffer), pCStrKey, pSize, NULL, NULL);
					pCStrKey[pSize] = '\0';

					gmCode += pCStrKey;
				}
				delete buffer;
				delete[] pCStrKey;

				gmCode = StringToUTF8(gmCode);
				erlang.SendGM(gmCode.c_str());
			}
		}
	}
	else
	{
		SetSelectRole(code);
	}
}

// ================================= ///

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_EXPORTXML, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EXPORTXML));

    MSG msg;

    // 主消息循环: 
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EXPORTXML));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_EXPORTXML);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   // windows 禁用中文输入法
   ImmDisableIME(GetCurrentThreadId());
   ChineseController(hWnd);

   return TRUE;
}


//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
	{
		// 自已初始化操作
		INITAPP = true;
		LoadCfg(hWnd);
		DoAuto(PreferEnum::PREFER_PHP);
		DefaultPrefer(hWnd);
		// 自已初始化操作
		g_gmdialog = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DIALOG_GM), NULL, About);
		//g_searchBox = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DIALOG_SEARCH), NULL, NULL);
		g_searchBox = CreateWindow(L"Edit", _T(""), WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_DISABLED,
			SEARCH_X, SEARCH_Y, SEARCH_W, SEARCH_H, hWnd, (HMENU)-1, hInst, NULL);
		LoadTCPSocket();
		g_hThread = nullptr;
		// 解析GM命令
		ParseGMCodes();

		//ShowWindow(g_gmdialog, SW_SHOW);
		return 0;
	}
	case WM_KEYUP:
	{
		switch (wParam)
		{
		case VK_CONTROL:
			controlKey = false;
			ChineseController(hWnd);
			break;
		case VK_SHIFT:
			shiftKey = false;
			break;
		default:
			break;
		}
		break;
	}
	case EN_KILLFOCUS:
		controlKey = false;
		shiftKey = false;
		break;
	case WM_KEYDOWN:
	{
		if(wParam == 229) MessageBox(NULL, L"请关闭输入法", L"提示", MB_OK);
		if (!controlKey && (wParam >= 'a' && wParam <= 'z') || (wParam >= 'A' && wParam <= 'Z'))
		{
			size_t len = searchCode.size();
			searchCode.push_back(char(wParam));
			transform(searchCode.begin(), searchCode.end(), searchCode.begin(), ::tolower);
			size_t nowlen = searchCode.size();
			//if (len != nowlen && nowlen > 0) ShowWindow(g_searchBox, SW_SHOWNOACTIVATE);
			Search(hWnd, hInst);
		}
		else
		{
			switch (wParam)
			{
			case VK_ESCAPE:
			{
				if (searchCode != "") 
				{
					ClearSearch(hWnd, hInst);
					//ShowWindow(g_searchBox, SW_HIDE);
				}
				else
				{
					WriteSave(hWnd);
				}
				break;
			}
			case VK_CONTROL:
			{
				controlKey = true;
				ChineseController(hWnd, true);
				break;
			}
			case VK_SHIFT:
				shiftKey = true;
				break;
			case VK_F5:
			{
				Reload(hWnd);
				break;
			}
			case VK_F1:
				ProjectCompile();
				break;
			case VK_F2:
				ProjectStart();
				break;
			case VK_F3:
				ProjectStart(true);
				break;
			case VK_F4:
				DoSVN("update");
				break;
			case VK_DELETE:
			case VK_BACK:
			{
				if (searchCode != "")
				{
					searchCode.pop_back();
					Search(hWnd, hInst);
				}
				break;
			}
			default:
				if (wParam == 'u' || wParam == 'U') DoSVN("update");
				break;
			}
		}
		return 0;
	}
	case WM_SIZE:
	{
		Reload(hWnd, true);
		break;
	}
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
		if (wmId >= buttonCodeStart)
		{
			DoXML(hWnd, wmId);
			return 0;
		}
		else
		{
			// 分析菜单选择: 
			switch (wmId)
			{
			case IDM_ABOUT:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;
			case IDM_EXIT:
				WriteSave(hWnd);
				break;
			case ID_SELECT_XML_DIR:
			{
				std::string tmpLoadPath = GetSelectFolder(hWnd, hInst, szLoadPath);
				if (tmpLoadPath != szLoadPath)
				{
					szLoadPath = tmpLoadPath;
					GetAllFiles(szLoadPath.c_str(), hWnd, hInst);
				}
				break;
			}
			case ID_COMPILE:
			{
				ProjectCompile();
				break;
			}
			case ID_START:
				ProjectStart();
				break;
			case ID_START2:
				ProjectStart(true);
				break;
			case ID_SELECT_PRJ_DIR:
			{
				szCompilePath = GetSelectFolder(hWnd, hInst, szCompilePath);
				break;
			}
			case ID_PHP_DIR:
			{
				szPHPPath = GetSelectFile();
				break;
			}
			case ID_GM_CODE_PATH:
			{
				szGMCodePath = GetSelectFile();
				break;
			}
			case ID_PHP_START:
				StartPHPStudy();
				break;
			case ID_SVN_UPDATE:
				DoSVN("update");
				break;
			case ID_SVN_COMMIT:
				DoSVN("commit");
				break;
			case ID_SVN_REVERT:
				DoSVN("revert");
				break;
			case ID_SVN_ADD:
				DoSVN("add");
				break;
			case ID_SVN_CLEANUP:
				DoSVN("cleanup");
				break;
			case ID_SQL_MAIN:
			{
				std::string tmpProjectPath(szProjectPath);
				std::string path = tmpProjectPath + "\\db\\";
				CreateRunTmpBat("make.bat", path);
				break;
			}
			case ID_PREPHP_START:
			case ID_PRE_EXP_COMPILE:
			case ID_PRF_EXP_SVN:
				ChangePreferDefine(hWnd, wmId);
				break;
			case ID_PRO_DIR:
				if(!controlKey) szProtoPath = GetSelectFolder(hWnd, hInst, szProtoPath);
				else OpenProtoDir();
				break;
			case ID_PRO_EXP:
				if(!controlKey) ExportProto();
				else OpenProtoDir();
				break;
			case ID_OPEN_CON_PHP:
				OpenConPHP();
				break;
			case ID_XML_REFRESH:
				Reload(hWnd);
				break;
			case ID_OPEN_PROJ_DIR:
				OpenProjDir();
				break;
			case ID_SVN_CLEAN_ALL:
				ClearSVN();
				break;
			case ID_XML_OPEN:
				Start(szLoadPath.c_str());
				break;
			case ID_PROTO_OPEN:
				OpenProtoDir();
				break;
			case ID_GM_CONFIG:
				szGMPath = GetSelectFile();
				break;
			case ID_GM_OPEN:
				ShowWindow(g_gmdialog, SW_SHOW);
				break;
			case ID_GM_TCP_CONFIG:
				LoadTCPSocket();
				break;
			default:
				WriteLog("select other command!!");
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;
    }
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
		WriteSave(hWnd);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
	case WM_CONTEXTMENU:
	{
		RECT rect;
		POINT pt;
		// 获取鼠标右击是的坐标     
		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);
		//获取客户区域大小     
		GetClientRect((HWND)wParam, &rect);
		//把屏幕坐标转为客户区坐标     
		ScreenToClient((HWND)wParam, &pt);
		//判断点是否位于客户区域内     
		if (PtInRect(&rect, pt))
		{
			HMENU hroot = LoadMenu((HINSTANCE)GetWindowLongPtr(hDlg, GWLP_HINSTANCE), MAKEINTRESOURCE(IDR_MENU_ROLES));
			if (hroot)
			{
				curSelectRoleIndex = 0;		// 数据变了，重新设置
				HMENU hpop = hpop = GetSubMenu(hroot, 0);
				for (int tmp = 0; tmp < oldNumberRoles; ++tmp)
				{
					RemoveMenu(hpop, (UINT_PTR)tmp, MF_STRING);
					DestroyMenu((HMENU)tmp);
				}

				std::vector<std::string>::const_iterator it;
				int index = roleIndexCode;

				wchar_t *buffer = new wchar_t[max];
				int preSize = max;
				for (it = allRoles.begin(); it != allRoles.end(); ++it)
				{
					memset(buffer, 0, preSize);

					const std::string code = *it;
					const char* pCStrKey = code.c_str();
					//第一次调用返回转换后的字符串长度，用于确认为wchar_t*开辟多大的内存空间
					int pSize = MultiByteToWideChar(CP_OEMCP, 0, pCStrKey, strlen(pCStrKey) + 1, NULL, 0);
					//第二次调用将单字节字符串转换成双字节字符串
					MultiByteToWideChar(CP_OEMCP, 0, pCStrKey, strlen(pCStrKey) + 1, buffer, pSize);

					preSize = pSize;
					AppendMenu(hpop, MF_STRING, (UINT_PTR)index++, buffer);
				}
				delete buffer;
				oldNumberRoles = allRoles.size();

				// 把客户区坐标还原为屏幕坐标     
				ClientToScreen((HWND)wParam, &pt);
				//显示快捷菜单     
				TrackPopupMenu(hpop,
					TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
					pt.x,
					pt.y,
					0,
					(HWND)wParam,
					nullptr);
				// 用完后要销毁菜单资源     
				DestroyMenu(hroot);
				return (INT_PTR)TRUE;
			}
			return (INT_PTR)TRUE;
		}
	}
    case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
			break;
		case IDC_BUTTON_CON:
		{
			if (szHost == "" || szPort == "")
			{
				MessageBox(NULL, L"请在app.cfg里配置 gm_port 和 host", L"GM TCP 启动配置项出错", MB_OK);
			}
			else
			{
				SOCKET socket = erlang.Connect(szHost.c_str(), std::stoi(szPort));
				if (socket)
				{
					if (g_hThread)
					{
						CloseHandle(g_hThread);
						g_hThread = nullptr;
					}
					//接收数据 
					g_hThread = CreateThread(NULL, 0, Fun, &socket, 0, NULL);
					erlang.SendGM("roles");
				}
			}
			return (INT_PTR)TRUE;
		}
		case IDC_BUTTON_COMPILE:
			CreateMakeErlang();
			return (INT_PTR)TRUE;
		case IDC_BUTTON_COM_UP:
			CreateMakeErlang();
			erlang.SendGM("u");
			return (INT_PTR)TRUE;
		case IDC_BUTTON_RESTART:
			erlang.SendGM("stop");
			Sleep(1000);
			CreateMakeErlang();
			return (INT_PTR)TRUE;
		case IDC_BUTTON_SEND_GM:
		{
			wchar_t *buffer = new wchar_t[max];
			GetDlgItemText(g_gmdialog, IDC_EDIT_GM, buffer, max);
			std::string gmCode("");
			WChar_tToString(buffer, gmCode);
			erlang.SendGM(gmCode.c_str());
			delete buffer;
			return (INT_PTR)TRUE;
		}
		case ID_GM_RELOAD:
			ParseGMCodes();
			break;
		case ID_ROLE_REFRESH:
			erlang.SendGM("roles");
			break;
		case IDC_BUTTON_FIRST:
		{
			if (curGMPage != 0)
			{
				curGMPage = 0;
				RelayoutGM(gmcodes);
			}
			break;
		}
		case IDC_BUTTON_LAST:
		{
			size_t maxCount = gmcodes.size();
			int max = maxCount / MAX_GM_COUNT;
			//if ((maxCount % MAX_GM_COUNT) != 0) max += 1;
			if (curGMPage != max)
			{
				curGMPage = max;
				RelayoutGM(gmcodes);
			}
			break;
		}
		case IDC_BUTTON_NEXT:
		{
			size_t maxCount = gmcodes.size();
			int max = maxCount / MAX_GM_COUNT;
			if ((maxCount % MAX_GM_COUNT) != 0) max += 1;
			if (curGMPage + 1 < max)
			{
				curGMPage++;
				RelayoutGM(gmcodes);
			}
			break;
		}
		case IDC_BUTTON_PRE:
		{
			if (curGMPage - 1 >= 0)
			{
				curGMPage--;
				RelayoutGM(gmcodes);
			}
			break;
		}
		default:
			DoGM(LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
    }
    return (INT_PTR)FALSE;
}
