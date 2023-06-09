//#include "StdAfx.h"
#include "dbmysql.h"

DBMySql::DBMySql(void)
{
	m_Data = NULL;
}

DBMySql::~DBMySql(void)
{
	if(NULL != m_Data){
		Close();
	}
}

MYSQL * DBMySql::GetMysql()
{
	return m_Data;
}

int DBMySql::Connect(string host, string user,string passwd, string db,
    unsigned int port,unsigned long client_flag)
{
	if((m_Data = mysql_init(NULL)) &&
		mysql_real_connect( m_Data, host.c_str(),
		user.c_str(), passwd.c_str(),
		db.c_str(),port , NULL,
		client_flag)){
		//选择制定的数据库失败
		if ( mysql_select_db( m_Data, db.c_str () ) < 0 ){
			mysql_close( m_Data) ;
			return -1 ;
		}
        mysql_query(m_Data,"SET NAMES 'UTF8'");
	}
	else{
		//初始化mysql结构失败
		mysql_close( m_Data );
		return -1 ;
	}
	return 0;
}

void DBMySql::Close( )
{
	mysql_close(m_Data) ;
}

int DBMySql::ExecNonQuery(const char *SQL)
{
	if(!mysql_real_query(m_Data,SQL,strlen(SQL))){
		//得到受影响的行数
		return (int)mysql_affected_rows(m_Data) ;
	}
	else{
		//执行查询失败
		return -1;
	}
}

RecordSet *DBMySql::ExecQuery(const char *SQL)
{
	RecordSet *rdset=new RecordSet(m_Data);
	rdset->ExecQuery(SQL);
	return rdset;
}

char *DBMySql::ExecScalar(const char *SQL)
{
	MYSQL_RES * res;
	MYSQL_ROW row ;
	char *p = NULL;
	if(!mysql_real_query( m_Data,SQL,strlen(SQL))){
		//保存查询结果
		res = mysql_store_result( m_Data ) ;
		row = mysql_fetch_row( res ) ;
		//p = ((row[0]==NULL)||(!strlen(row[0])))?"-1":row[0];
		if((row[0]==NULL)||(!strlen(row[0]))){
		    p = "-1";
		}
		else{
		    p=row[0];
		}
		mysql_free_result( res ) ;
	}
	else{
		//执行查询失败

	}
	return p;
}

int DBMySql::BeginTrans()
{
	if(!mysql_real_query(m_Data, "START TRANSACTION" ,
	(unsigned long)strlen("START TRANSACTION") )){
		return 0;
	}
	else{
		//执行查询失败
		return -1;
	}
}

int DBMySql::Commit()
{
	if(!mysql_real_query( m_Data, "COMMIT",
	(unsigned long)strlen("COMMIT") ) ){
		return 0;
	}
	else{
		//执行查询失败
		return -1;
	}
}

int DBMySql::Rollback()
{
	if(!mysql_real_query(m_Data, "ROLLBACK",
	(unsigned long)strlen("ROLLBACK") ) ){
		return 0;
	}
	else{
		//执行查询失败
		return -1;
	}
}

const char *DBMySql::Get_client_info()
{
	return mysql_get_client_info();
}

const unsigned long DBMySql::Get_client_version()
{
	return mysql_get_client_version();
}

const char *DBMySql::Get_host_info()
{
	return mysql_get_host_info(m_Data);
}

const char *DBMySql::Get_server_info()
{
	return mysql_get_server_info( m_Data );
}

const unsigned long   DBMySql::Get_server_version()
{
	return mysql_get_server_version(m_Data);
}

const char *DBMySql::Get_character_set_name()
{
	return mysql_character_set_name(m_Data);
}

const char *DBMySql::GetSysTime()
{
	return ExecScalar("select now()");
}

int DBMySql::CreateDatabase(string dbName)
{
	string temp ;
	temp="CREATE DATABASE ";
	temp+=dbName;
	if(!mysql_real_query( m_Data,temp.c_str () ,
		(unsigned long)temp.length ()) ){
		return 0;
	}
	else{
		//执行查询失败
		return -1;
	}
}

int DBMySql::DropDatabase(string dbName)
{
	string temp ;
	temp="DROP DATABASE ";
	temp+=dbName;
	if(!mysql_real_query( m_Data,temp.c_str () ,
		(unsigned long)temp.length ()) ){
		return 0;
	}
	else{
		//执行查询失败
		return -1;
	}
};


int DBMySql::Ping()
{
	if(!mysql_ping(m_Data)){
		return 0;
	}
	else{
		return -1;
	}
}

int DBMySql::ShutDown()
{
	if(!mysql_shutdown(m_Data,SHUTDOWN_DEFAULT)){
		return 0;
	}
	else{
		return -1;
	}
}

int DBMySql::ReBoot()
{
	if(!mysql_reload(m_Data)){
		return 0;
	}
	else{
		return -1;
	}
}


Field::Field()
{

}

Field::~Field()
{

}

bool Field::IsNum(int num)
{
	if(IS_NUM(m_type[num])){
		return true;
	}
	else{
		return false;
	}
}

bool Field::IsNum(string num)
{
	if(IS_NUM(m_type[GetField_NO(num)])){
		return true;
	}
	else{
		return false;
	}
}

bool Field::IsDate(int num)
{
	if( FIELD_TYPE_DATE == m_type[num] ||
		FIELD_TYPE_DATETIME == m_type[num] ){
		return true;
	}
	else{
		return false;
	}
}

bool Field::IsDate(string num)
{
	int temp;
	temp=GetField_NO(num);
	if(FIELD_TYPE_DATE == m_type[temp] ||
		FIELD_TYPE_DATETIME == m_type[temp] ){
		return true;
	}
	else{
		return false;
	}
}

bool Field::IsChar(int num)
{
	if(m_type[num]==FIELD_TYPE_STRING  ||
		m_type[num]==FIELD_TYPE_VAR_STRING ||
		m_type[num]==FIELD_TYPE_CHAR ){
		return true;
	}
	else{
		return false;
	}
}

bool Field::IsChar(string num)
{
	int temp;
	temp=this->GetField_NO (num);
	if(m_type[temp]==FIELD_TYPE_STRING  ||
		m_type[temp]==FIELD_TYPE_VAR_STRING ||
		m_type[temp]==FIELD_TYPE_CHAR ){
		return true;
	}
	else{
		return false;
	}
}

bool Field::IsBlob(int num)
{
	if(IS_BLOB(m_type[num])){
		return true;
	}
	else{
		return false;
	}
}

bool Field::IsBlob(string num)
{
	if(IS_BLOB(m_type[GetField_NO(num)])){
		return true;
	}
	else{
		return false;
	}
}

int Field::GetField_NO(string field_name)
{
	for(unsigned int i=0;i<m_name.size ();i++){
		if(!m_name[i].compare (field_name)){
			return i;
		}
	}
	return -1;
}

Record::Record(Field *m_f)
{
	m_field =m_f;
}

Record::~Record()
{

}

void Record::SetData(string value)
{
	m_rs.push_back (value);
}

string Record::operator[](string s)
{
	return m_rs[m_field->GetField_NO(s)];
}

string Record::operator[](int num)
{
	return m_rs[num];
}

bool Record::IsNull(int num)
{
	if("" == m_rs[num]){
		return true;
	}
	else{
		return false;
	}
}

bool Record::IsNull(string s)
{
	if("" == m_rs[m_field->GetField_NO(s)]){
		return true;
	}
	else {
		return false;
	}
}

string Record::GetTabText()
{
	string temp;
	for(unsigned int i=0 ;i<m_rs.size();i++){
		temp+=m_rs[i];
		if(i<m_rs.size ()-1)
		temp+="\t";
	}
	return temp;
}

RecordSet::RecordSet()
{
	res = NULL;
	row = NULL;
	pos = 0;
}

RecordSet::RecordSet(MYSQL *hSQL)
{
	res = NULL;
	row = NULL;
	m_Data = hSQL;
	pos = 0;
}

RecordSet::~RecordSet()
{

}

int RecordSet::ExecQuery(const char *SQL)
{
	m_field.m_name.clear();
	m_field.m_type.clear();
	m_recordcount = 0;
	m_field_num = 0;
	m_s.clear();
	pos = 0;
	if ( !mysql_real_query(m_Data,SQL,strlen(SQL))){
		//保存查询结果
		res = mysql_store_result(m_Data );
		//得到记录数量
		m_recordcount = (int)mysql_num_rows(res) ;
		//得到字段数量
		m_field_num = mysql_num_fields(res) ;
		for (int x = 0 ; fd = mysql_fetch_field(res); x++){
			m_field.m_name.push_back(fd->name);
			m_field.m_type.push_back(fd->type);
		}
		//保存所有数据
		while (row = mysql_fetch_row(res)){
			Record temp(&m_field);
			for (int k = 0 ; k < m_field_num ; k++ ){
				if(row[k]==NULL||(!strlen(row[k]))){
					temp.SetData ("");
				}
				else{
					temp.SetData(row[k]);
				}
			}
			//添加新记录
			m_s.push_back (temp);
		}
		mysql_free_result(res ) ;
		return m_s.size();
	}
	return -1;
}

long RecordSet::MoveNext()
{
	return (++pos);
}

long  RecordSet::Move(long length)
{
	unsigned int l = pos + length;
	if(l<0){
		pos = 0;
		return 0;
	}
	else{
		if(l >= m_s.size()){
			pos = m_s.size()-1;
			return pos;
		}
		else{
			pos = l;
			return pos;
		}
	}
}

bool RecordSet::MoveFirst()
{
	pos = 0;
	return true;
}

bool RecordSet::MoveLast()
{
	pos = m_s.size()-1;
	if(pos <0){
		pos = 0;
		return true;
	}
	pos = m_s.size()-1;
	return true;
}

unsigned long RecordSet::GetCurrentPos()const
{
	return pos;
}

bool RecordSet::GetCurrentFieldValue(const string sFieldName,int len,char *sValue)
{
	strcpy(sValue,m_s[pos][sFieldName].c_str());
	return true;
}

bool RecordSet::GetCurrentFieldValue(const int iFieldNum,int len,char *sValue)
{
	strcpy(sValue,m_s[pos][iFieldNum].c_str());
	return true;
}

bool RecordSet::GetFieldValue(long index,const char * sFieldName,int len,char *sValue)
{
	strcpy(sValue,m_s[index][sFieldName].c_str());
	return true;
}

bool RecordSet::GetFieldValue(long index,int iFieldNum,int len,char *sValue)
{
	strcpy(sValue,m_s[index][iFieldNum].c_str());
	return true;
}

bool RecordSet::IsEof()
{
	return (pos == m_s.size())?true:false;
}

int RecordSet::GetRecordCount()
{
	return m_recordcount;
}

int RecordSet::GetFieldNum()
{
	return m_field_num;
}

Field * RecordSet::GetField()
{
	return &m_field;
}

const char * RecordSet::GetFieldName(int iNum)
{
	return m_field.m_name.at(iNum).c_str();
}

const int RecordSet::GetFieldType(char * sName)
{
	int i = m_field.GetField_NO(sName);
	return m_field.m_type.at(i);
}

const int RecordSet::GetFieldType(int iNum)
{
	return m_field.m_type.at(iNum);
}

Record RecordSet::operator[](int num)
{
	return m_s[num];
}

