#ifndef DBMYSQL_H
#define DBMYSQL_H

///在windows下编绎时需要去掉以下注释********************
/*
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"lib\\libmysql.lib")
#pragma comment(lib,"lib\\mysqlclient.lib")
#pragma comment(lib,"lib\\mysys-max.lib")
#pragma comment(lib,"lib\\mysys-nt.lib")
#pragma comment(lib,"lib\\strings.lib")*/
///在windows下编绎时需要去掉以下注释********************

#include "mysql.h"
#include <iostream>
#include <vector>
#include <string>
#include <string.h>

using namespace std;

class RecordSet;

class DBMySql
{
public:
	DBMySql(void);
	~DBMySql(void);
public :
	MYSQL *GetMysql();
	int Connect(string host, string user,string passwd, string db,unsigned int port,unsigned long client_flag);
	void Close();
	RecordSet *ExecQuery(const char *SQL);
	int ExecNonQuery(const char *SQL);
	char *ExecScalar(const char *SQL);
	int BeginTrans();
	int Commit();
	int Rollback();
	const char * Get_client_info();
	const unsigned long  Get_client_version();
	const char *Get_host_info();
	const char *Get_server_info();
	const unsigned long  Get_server_version();
	const char *Get_character_set_name();
	const char *GetSysTime();
	int CreateDatabase(string dbName);
	int DropDatabase(string dbName);
	int Ping();
	int ShutDown();
	int ReBoot();
private:
	MYSQL* m_Data;
public:
	RecordSet *m_rdset;
};

class Field
{
 public :
	Field();
	~Field();
	bool IsNum(int num);
	bool IsNum(string num);
	bool IsDate(int num);
	bool IsDate(string num);
	bool IsChar(int num);
	bool IsChar(string num);
	bool IsBlob(int num);
	bool IsBlob(string num);
	int GetField_NO(string field_name);
public :
  vector<string> m_name;
  vector<enum_field_types> m_type;
};

class Record
{
public :
	Record();
	Record(Field* m_f);
	~Record();
	void SetData(string value);
	string operator[](string s);
	string operator[](int num);
	bool IsNull(int num);
	bool IsNull(string s);
	string GetTabText();
public:
	vector<string> m_rs;
	Field *m_field;
};

class RecordSet
{
public :
	RecordSet();
	RecordSet(MYSQL *hSQL);
	~RecordSet();
	int ExecQuery(const char *SQL);
	int GetRecordCount();
	int GetFieldNum();
	long MoveNext();
	long Move(long length);
	bool MoveFirst();
	bool MoveLast();
	unsigned long GetCurrentPos()const;
	bool GetCurrentFieldValue(const string sFieldName,int len,char *sValue);
	bool GetCurrentFieldValue(const int iFieldNum,int len,char *sValue);
	bool GetFieldValue(long index,const char * sFieldName,int len,char *sValue);
	bool GetFieldValue(long index,int iFieldNum,int len,char *sValue);
	bool IsEof();
	Field* GetField();
	const char * GetFieldName(int iNum);
	const int GetFieldType(char * sName);
	const int GetFieldType(int iNum);
	Record operator[](int num);
private :
	vector<Record> m_s;
	unsigned long pos;
	int m_recordcount;
	int m_field_num;
	Field  m_field;
	MYSQL_RES * res ;
	MYSQL_FIELD * fd ;
	MYSQL_ROW row;
	MYSQL* m_Data ;
};

#endif // DBMYSQL_H
