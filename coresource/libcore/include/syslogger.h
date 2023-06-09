#ifndef SYSLOGGER_H
#define SYSLOGGER_H


#include <string>
#include <stdarg.h>
#include "datatype.h"

enum LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal
};

class LogWriter
{
public:
    LogWriter();
    virtual ~LogWriter();
    virtual ST_INT 		Log(const LogLevel logLevel, const ST_CHAR* fmt, va_list ap) = 0;
    virtual ST_INT 		LogFuncEntry(const ST_CHAR* funcName) = 0;
    virtual ST_INT 		LogFuncExit(const ST_CHAR* funcName, ST_INT lineNum) = 0;
    virtual ST_INT 		LoggerUnInit() = 0;
};

class LocalWriter : public LogWriter
{
    friend class SysLogger;
public:
    LocalWriter();
    ~LocalWriter();
    ST_INT 				Log(const LogLevel logLevel, const ST_CHAR* fmt, va_list ap);
    ST_INT 				LogFuncEntry(const ST_CHAR* funcName);
    ST_INT 				LogFuncExit(const ST_CHAR* funcName, ST_INT lineNum);
    ST_INT 				LoggerUnInit();
protected:
    const ST_CHAR*		sGetLogPrefix(const LogLevel logLevel);
    const ST_CHAR*      sGetLogTime();    //LXB
    FILE                *fp;
};

class SysLogger
{
private:
    SysLogger();
    SysLogger(std::string&);
     ~SysLogger();
 public:
    static SysLogger	*GetInstance();
    ST_INT				InitLogger(ST_INT logType);
    ST_VOID				UnInitLogger();
    ST_INT 				LogTrace (const ST_CHAR* fmt,...);
    ST_INT 				LogDebug (const ST_CHAR* fmt,...);
    ST_INT 				LogInfo  (const ST_CHAR* fmt,...);
    ST_INT 				LogWarn  (const ST_CHAR* fmt,...);
    ST_INT 				LogError (const ST_CHAR* fmt,...);
    ST_INT 				LogFatal (const ST_CHAR* fmt,...);
    ST_INT 				FuncLogEntry(const ST_CHAR* funcName);
    ST_INT 				FuncLogExit(const ST_CHAR* funcName, const ST_INT lineNumber);
protected:
    ST_INT				vsLogStub(LogLevel logLevel, const ST_CHAR* fmt, va_list ap);
    void                checkFileExist();
    void                checkFileSize();  //LXB
private:
    LogWriter			*pLogWriter;
    std::string			filename;
};

#endif // SYSLOGGER_H
