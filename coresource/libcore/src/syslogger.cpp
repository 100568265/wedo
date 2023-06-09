#include "syslogger.h"
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include "datetime.h"

LogWriter::LogWriter()
{

}

LogWriter::~LogWriter()
{

}

LocalWriter::LocalWriter() : fp(0)
{
}

LocalWriter::~LocalWriter()
{
    LoggerUnInit();
}

ST_INT LocalWriter::Log(const LogLevel logLevel, const ST_CHAR* fmt,va_list ap)
{
	if(!fp) {
		fprintf(stderr, "Log file handle is NULL\n");
		return -1;
	}
	else {
        fprintf(fp,sGetLogTime());          //LXB 19-10-14
        fprintf(fp,sGetLogPrefix(logLevel));
		vfprintf(fp,fmt,ap);
		fprintf(fp,"\r\n");
		fflush(fp);
		return 0;
	}
}

ST_INT LocalWriter::LogFuncEntry(const ST_CHAR* funcName)
{
	if(!fp)
	{
		fprintf(stderr,"fp==NULL");
		return -1;
	}
	else
	{
		ST_INT bytes_written = 0;
		bytes_written = fprintf(fp,"{ %s \r\n", funcName);
		fflush(fp);
		return bytes_written;
	}

}

ST_INT LocalWriter::LogFuncExit(const ST_CHAR* funcName,const ST_INT lineNumber)
{
	if(!fp)
	{
		fprintf(stderr,"fp==NULL");
		return -1;
	}
	else
	{
		ST_INT bytes_written = 0;
		bytes_written = fprintf(fp,"%s : %d }\r\n", funcName,lineNumber);
		fflush(fp);
		return bytes_written;
	}
}

ST_INT LocalWriter::LoggerUnInit()
{
	if(fp) {
		if( (fp != stdout) && (fp != stderr) )
			fclose(fp);
	}
	fp = NULL;
	return 0;
}

const ST_CHAR* LocalWriter::sGetLogPrefix(const LogLevel logLevel)
{
	switch (logLevel) {
		case Trace:	return "[T]";
		case Debug: return "[D]";
		case Info:  return "[I]";
		case Warn:  return "[W]";
		case Error:	return "[E]";
		case Fatal:	return "[F]";
		default:	return "";
	}
}

const ST_CHAR* LocalWriter::sGetLogTime()
{
    struct tm t_tm;
	DateTime::localtime (time(0), t_tm);
	static ST_CHAR timeinfo[50];    //局部静态变量
	sprintf(timeinfo,"[%d-%02d-%02d %02d:%02d:%02d]",
         (t_tm.tm_year-100),
         (t_tm.tm_mon+1),
         (t_tm.tm_mday),
         t_tm.tm_hour,
         t_tm.tm_min,
         t_tm.tm_sec)
         ;
	return timeinfo;
}


SysLogger::SysLogger(std::string& name)
	: pLogWriter(0), filename(name)
{
}

SysLogger::SysLogger() : filename("log.log")
{
    pLogWriter = NULL;
}

SysLogger::~SysLogger()
{
    if(NULL!=pLogWriter)
    {
        delete pLogWriter;
    }
    pLogWriter=NULL;
}

SysLogger *SysLogger::GetInstance()
{
    static SysLogger instance;
    static ST_BOOLEAN m_Inited = false;
    if(!m_Inited)
    {
        instance.InitLogger(2);
        m_Inited = true;
    }
    return &instance;
}

ST_INT SysLogger::InitLogger(ST_INT logType)
{
	ST_INT retVal = 0;
	if(pLogWriter)
	{
		fprintf(stderr,"\n [liblogger]Deinitializing the current log writer\n");
		UnInitLogger();
	}
	switch(logType)
	{
		case 1:
            pLogWriter = new LocalWriter();
            ((LocalWriter*)pLogWriter)->fp = stdout;
            break;
		case 2:
            pLogWriter = new LocalWriter();
            ((LocalWriter*)pLogWriter)->fp = fopen(filename.c_str(), "a");
			break;
	}
	retVal = 0;
	return retVal;
}


ST_VOID  SysLogger::UnInitLogger()
{
	pLogWriter->LoggerUnInit();
	pLogWriter = 0;
}

void SysLogger::checkFileExist ()
{
	if (((LocalWriter*)pLogWriter)->fp == NULL)
		return ;
	if (((LocalWriter*)pLogWriter)->fp == stdout)
		return ;
	if (((LocalWriter*)pLogWriter)->fp == stderr)
		return ;
	if (access (filename.c_str(), F_OK) < 0) {
		fclose (((LocalWriter*)pLogWriter)->fp);
		((LocalWriter*)pLogWriter)->fp = fopen (filename.c_str(), "a");
	}
}

void SysLogger::checkFileSize()
{
    if (((LocalWriter*)pLogWriter)->fp == NULL)
		return ;
	if (((LocalWriter*)pLogWriter)->fp == stdout)
		return ;
	if (((LocalWriter*)pLogWriter)->fp == stderr)
		return ;
    unsigned long fileSize =0;
	fseek(((LocalWriter*)pLogWriter)->fp, 0L, SEEK_END);
    fileSize = ftell(((LocalWriter*)pLogWriter)->fp);
    if(fileSize<512000) //500*1024
        return ;
    else
        remove(filename.c_str());
}

ST_INT SysLogger::vsLogStub(LogLevel logLevel, const ST_CHAR* fmt,va_list ap)
{
	ST_INT retVal = 0;
	checkFileSize();  //LXB
	checkFileExist ();
	retVal = pLogWriter->Log(logLevel,fmt,ap);
	return retVal;
}

ST_INT SysLogger::LogTrace(const ST_CHAR* fmt,...)
{
	ST_INT retVal = 0;
	va_list ap;
	va_start(ap,fmt);
	retVal = vsLogStub(Trace,fmt,ap);
	va_end(ap);
	return retVal;
}

ST_INT SysLogger::LogDebug(const ST_CHAR* fmt,...)
{
	ST_INT retVal = 0;
	va_list ap;
	va_start(ap,fmt);
	retVal = vsLogStub(Debug,fmt,ap);
	va_end(ap);
	return retVal;
}

ST_INT SysLogger::LogInfo(const ST_CHAR* fmt,...)
{
	ST_INT retVal = 0;
	va_list ap;
	va_start(ap,fmt);
	retVal = vsLogStub(Info,fmt,ap);
	va_end(ap);
	return retVal;
}

ST_INT SysLogger::LogWarn(const ST_CHAR* fmt,...)
{
	ST_INT retVal = 0;
	va_list ap;
	va_start(ap,fmt);
	retVal = vsLogStub(Warn,fmt,ap);
	va_end(ap);
	return retVal;
}

ST_INT SysLogger::LogError(const ST_CHAR* fmt,...)
{
	ST_INT retVal = 0;
	va_list ap;
	va_start(ap,fmt);
	retVal = vsLogStub(Error,fmt,ap);
	va_end(ap);
	return retVal;
}

ST_INT SysLogger::LogFatal (const ST_CHAR* fmt,...)
{
	ST_INT retVal = 0;
	va_list ap;
	va_start(ap,fmt);
	retVal = vsLogStub(Fatal,fmt,ap);
	va_end(ap);
	return retVal;
}

ST_INT SysLogger::FuncLogEntry(const ST_CHAR* funcName)
{
	ST_INT retVal = 0;
	retVal = pLogWriter->LogFuncEntry(funcName);
	return retVal;
}

ST_INT SysLogger::FuncLogExit(const ST_CHAR* funcName, const ST_INT lineNumber)
{
	ST_INT retVal = 0;
	retVal = pLogWriter->LogFuncExit(funcName, lineNumber);
	return retVal;
}





