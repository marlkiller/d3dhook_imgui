#pragma once
#include <wtypes.h>

#define LOG_PRINT(printLev, format, ...) LogPrinter::LogPrint(printLev, __FILE__, __LINE__, format, __VA_ARGS__);
#define LOG_INFO(format, ...) LogPrinter::LogPrint(PRINT_LEV_INFO, __FILE__, __LINE__, format, __VA_ARGS__);
#define LOG_EVENT(format, ...) LogPrinter::LogPrint(PRINT_LEV_EVENT, __FILE__, __LINE__, format, __VA_ARGS__);
#define LOG_ERROR(format, ...) LogPrinter::LogPrint(PRINT_LEV_ERROR, __FILE__, __LINE__, format, __VA_ARGS__);
#define LOG_DEBUG(format, ...) LogPrinter::LogPrint(PRINT_LEV_DEBUG, __FILE__, __LINE__, format, __VA_ARGS__);
#define OPEN_COONSOLE() LogPrinter::OpenConsole();
#define CLOSE_COONSOLE() LogPrinter::CloseConsole();
#define OUTPUT_DEBUG(format,...) LogPrinter::OutputDebug(format,__VA_ARGS__);

enum PRINT_LEV
{
    PRINT_LEV_DEBUG,
    PRINT_LEV_INFO,
    PRINT_LEV_EVENT,
    PRINT_LEV_ERROR,
    PRINT_LEV_UNKNOWN,
    PRINT_LEV_NUM
};

class LogPrinter
{
private:
    explicit LogPrinter();
    ~LogPrinter();

public:
    static void LogPrint(PRINT_LEV printLev, const char* filename, int lineNo, const char* format, ...);


    static void OpenConsole();
    static void CloseConsole();
    static void OutputDebug(const WCHAR* format, ...);

private:
    class LogPrinterDef;
    static LogPrinterDef* _PrinterDef;
};
