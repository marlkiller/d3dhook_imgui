#include <stdio.h>
#include <string>
#include <stdarg.h>
#include <time.h>

#include "logger.h"
#include <iostream>
#include <stdarg.h>
using namespace std;

#define RUN_LOG_FILENAME "RunLog.log"
#define LOG_FILE_MAX_SIZE (60 * 1024 * 1024)   // ���ļ���С����:60MB


class LogPrinter::LogPrinterDef
{
private:
    static const string m_printLevel[PRINT_LEV_NUM];

    static FILE* mConsoleOut;
    static std::streambuf* mConsoleOutBackup;
    

public:

    
    void prevInfoPrint(PRINT_LEV printLev, const char* fileName, int lineNo) {
        char buf[128];

        time_t now = time(0);
        tm ltm;
        localtime_s(&ltm, &now);

        sprintf_s(buf, "[%d/%02d/%02d %2d:%02d:%02d] [%s:%d] [%s]: ",
            ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday,
            ltm.tm_hour, ltm.tm_min, ltm.tm_sec,
            fileName, lineNo, m_printLevel[printLev].data());

        printf("%s", buf);

        FILE* fp;
        ::fopen_s(&fp, RUN_LOG_FILENAME, "a+");
        if (!fp) {
            return;
        }

        fwrite(buf, sizeof(char), strlen(buf), fp);
        fclose(fp);
    }

    int GetFileSize() {
        struct stat statbuf;
        stat(RUN_LOG_FILENAME, &statbuf);
        return statbuf.st_size;
    }

    void logFileSizeCtl() {
        if (GetFileSize() < LOG_FILE_MAX_SIZE)
            return;

        const char* newname = "RunLogBack.log";

        FILE* fp;
        fopen_s(&fp, newname, "r");
        if (fp != NULL) {
            fclose(fp);
            remove(newname);
        }
        rename(RUN_LOG_FILENAME, newname);
    }
};

LogPrinter::LogPrinterDef* LogPrinter::_PrinterDef = new LogPrinterDef;

const string LogPrinter::LogPrinterDef::m_printLevel[PRINT_LEV_NUM] = {
    "Debug",
    "Info",
    "Event",
    "Error",
    "Unknown",
};


LogPrinter::LogPrinter() {}

LogPrinter::~LogPrinter() { delete _PrinterDef; _PrinterDef = nullptr; }

/* ��¼��־
* @param printLev  [��־��Ϣ�ȼ�]
* @param filename  [��־��Ϣ�ļ���]
* @param lineNo    [��־��Ϣ������]
* @param format    [��־��Ϣ�б��ʽ]
*/
void __cdecl LogPrinter::LogPrint(PRINT_LEV printLev, const char* filename, int lineNo, const char* format, ...)    // __cdecl �ǹؼ��ķ���ֵ����
{
    if (printLev < 0 || printLev >= PRINT_LEV_NUM)
        printLev = PRINT_LEV_UNKNOWN;

    _PrinterDef->logFileSizeCtl();

    string s_filiname;
    const char* temp = strrchr(filename, '\\');
    if (temp)
        s_filiname = temp + 1;
    else
        s_filiname = filename;
    _PrinterDef->prevInfoPrint(printLev, s_filiname.data(), lineNo);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    printf("\n");

    FILE* fp;
    ::fopen_s(&fp, RUN_LOG_FILENAME, "a+");
    if (!fp) {
        return;
    }
    vfprintf(fp, format, args);
    fwrite("\n", 1, 1, fp);
    fclose(fp);

    return;
}


static bool mConsoleOpen = false;
static bool needMenu = true;
static FILE* mConsoleOut;
static std::streambuf* mConsoleOutBackup;

void __cdecl LogPrinter::OpenConsole() {

    if (!mConsoleOpen)
    {
        if (AllocConsole()) {
            mConsoleOutBackup = cout.rdbuf();
            mConsoleOut = freopen("CONOUT$", "w", stdout);
            char nt[100];
            sprintf_s(nt, "��־[%s]-%lld-%d", "Console-Wind", GetTickCount64(), GetCurrentProcessId());
            SetConsoleTitleA(nt);
            Sleep(100);
            HWND find = FindWindowA(NULL, nt);
            if (find) {
                HMENU menu = GetSystemMenu(find, FALSE);
                if (menu) {
                    if (!needMenu) {
                        if (RemoveMenu(menu, 0xF060, 0)) {
                            sprintf_s(nt, "��־[%s]-%lld-%d-�����ιرհ�ť", "Console-Wind", GetTickCount64(), GetCurrentProcessId());
                            SetConsoleTitleA(nt);
                        }
                    }
                }
            }
        }
        wcout.imbue(locale("", LC_CTYPE));
        mConsoleOpen = TRUE;
    }
    
}

void __cdecl LogPrinter::CloseConsole() {
    if (mConsoleOpen) {
        if (mConsoleOut != NULL && mConsoleOutBackup != NULL) {
            cout.rdbuf(mConsoleOutBackup);
            fclose(mConsoleOut);
        }
        FreeConsole();
        mConsoleOpen = FALSE;
    }
}

void __cdecl LogPrinter::OutputDebug(const WCHAR* format, ...) {
    WCHAR strBuffer[4096] = { 0 };
    // VA_LIST ����C�����н����������һ��꣬����ͷ�ļ���#include <stdarg.h>�����ڻ�ȡ��ȷ�������Ĳ�����
    va_list vlArgs;
    // VA_START�꣬��va_start���ʼ�������ն����va_list������ʹ��ָ���һ���ɱ�����ĵ�ַ������strOutputString�ڶ�ջ�еĵ�ַ��
    va_start(vlArgs, format);
    // _vsnwprintf_s��_vsnprintf����ȫ�İ汾��
    _vsnwprintf_s(strBuffer, ARRAYSIZE(strBuffer) - 1, ARRAYSIZE(strBuffer) - 1, format, vlArgs);
    // �ص�ָ��va_list 
    va_end(vlArgs);

    // ����д�ǿ���̨����ʱ���������Ϣ������windows API��
    OutputDebugStringW(strBuffer);
    
}