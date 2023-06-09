#include "sysinifile.h"
#include <algorithm>
#include <string.h>
#include <stdio.h>

CIniFile::CIniFile(const string fileName):
m_strFileName(fileName),
m_fileHandle(NULL)
{
}

CIniFile::~CIniFile(void)
{
	CloseIni();
}

int CIniFile::OpenIni()
{
	return OpenIni(m_strFileName);
}

int  CIniFile::OpenIni(const string fileName)
{
	this->m_fileHandle = fopen(fileName.c_str(),"rt+");
	if (NULL == this->m_fileHandle) {
		return 0;
	}
	fseek(this->m_fileHandle, 0, SEEK_SET); //将文件指针指向文件开头
	char cTemp[512] = {0};
	vector<string> vecStrItem;

	while(NULL != fgets(cTemp, sizeof(cTemp), this->m_fileHandle)){
		DeleteEnter(cTemp, sizeof(cTemp));//去结尾回车符
		Trim(cTemp); //去前后空格
		string str = cTemp;
		vecStrItem.push_back(str);
	}
	this->CloseIni();
	pair<string, string> pKVItem;
	pair<string, pair<string, string> > pItem;
	string strSection = "";
	string strCurrentSection = "";
	for (unsigned int i = 0; i < vecStrItem.size(); i++) {
		string strLine = ((string)vecStrItem[i]);
		if( '[' == strLine[0] ){
			strSection = strLine;
			strSection = strSection.erase(0,1);
			if(']' == strSection[strSection.size() -1 ])
				strSection = strSection.erase(strSection.size() - 1 );
			if (strCurrentSection != strSection)
				strCurrentSection = strSection;
		}
		else if( string::npos != strLine.find('=') ){
			if ("" != strCurrentSection ) {
				string strKey;
				string strValue;
				strKey = TrimR( strLine.substr(0, strLine.find('=')) );
				strValue = TrimL( strLine.substr(strLine.find('=') + 1, strLine.size() - strLine.find('=')  ) );
				pKVItem.first = strKey;
				pKVItem.second = strValue;
				pItem.first = strCurrentSection;
				pItem.second = pKVItem;
				m_szStrItem.push_back(pItem);
			}
		}
		else{
			if ("" != strCurrentSection ){
				string strKey = "##";
				string strValue = TrimL( strLine );
				pKVItem.first = strKey;
				pKVItem.second = strValue;
				pItem.first = strCurrentSection;
				pItem.second = pKVItem;
				m_szStrItem.push_back(pItem);
			}
			else{
				string strKey = "##";
				string strValue = TrimL( strLine );
				pKVItem.first = strKey;
				pKVItem.second = strValue;
				pItem.first = "###";
				pItem.second = pKVItem;
				m_szStrItem.push_back(pItem);
			}
		}
	}
	return 1;
}

string CIniFile::GetValue(const string p_strSection, const string p_strKey)
{
	string strValue="";
	for (unsigned i = 0; i< m_szStrItem.size(); i++) {
		if( p_strSection == (string) ( ((pair<string, pair<string, string> >)m_szStrItem[i]).first ) ){
			pair<string, string> pKVItem = ((pair<string, pair<string, string> >)m_szStrItem[i]).second;
			if ( p_strKey == (string)(pKVItem.first)) {
				strValue = (string)(pKVItem.second);
				break;
			}
		}
	}
	return strValue;
}


void CIniFile::Print()
{
	string strValue;
	string strKey;
	string strSection;
	for (unsigned i = 0; i< m_szStrItem.size(); i++) {
		strSection = (string) ( ((pair<string, pair<string, string> >)m_szStrItem[i]).first );
		pair<string, string> pKVItem = ((pair<string, pair<string, string> >)m_szStrItem[i]).second;
		strKey = (string)(pKVItem.first);
		strValue = (string)(pKVItem.second);
		printf("[%s]:%s=%s\n", strSection.c_str(), strKey.c_str(), strValue.c_str());
	}
}


void CIniFile::CloseIni()
{
	if(NULL != this->m_fileHandle){
		fclose(this->m_fileHandle);
		this->m_fileHandle = NULL;
	}
}

string CIniFile::TrimL(string p_strValue)
{
	int i = 0 ;
	if ((i=(int)(p_strValue.length())) < 1)
	return "";
	i = 0;
	while(isspace(p_strValue[i])){
		i++ ;
	}
	p_strValue.erase(0,i);
	return p_strValue;
}

string CIniFile::TrimR(string p_strValue)
{
	int i = 0;
	if ( (i = (int)(p_strValue.length()-1)) < 1 )
	return "";
	while( ( p_strValue[i] ) == ' ' ){
		i--;
	}
	p_strValue.erase(i + 1);
	return p_strValue;
}


string CIniFile::Trim(string p_strValue)
{
	return TrimL(TrimR(p_strValue));
}

int CIniFile::SetValue(const string p_strSection, const string p_strKey, const string p_strValue)
{
	pair<string, string> pKVItem;
	pair<string, pair<string, string> > pItem;

	pKVItem.first = p_strKey;
	pKVItem.second = p_strValue;
	pItem.first = p_strSection;
	pItem.second = pKVItem;

	int bFind = 0;

	for(int i = (int)(m_szStrItem.size())-1; i >= 0; i--){ //找到已有的section, 并在末尾添加
		string bb = m_szStrItem[i].second.first;
		if((m_szStrItem[i].first == p_strSection)  && (m_szStrItem[i].second.first != "##")/* && (m_szStrItem[i].second.second != "")*/){
			m_szStrItem.insert( m_szStrItem.begin()+ i+1, pItem );
			bFind = 1;
			break;
		}
	}

	if(bFind != 1) //如果是新增的section
	m_szStrItem.push_back(pItem);

	this->m_fileHandle = fopen(this->m_strFileName.c_str(),"wt+");
	if (NULL == this->m_fileHandle) {
		return 0;
	}

	string strCurSection = "";
	string strTmp = "";

	if((int)(m_szStrItem.size()) > 0){
		strCurSection = m_szStrItem[0].first;
		if(strCurSection != "###"){
			strTmp = "[" + strCurSection + "]" + "\n";
			fputs(strTmp.c_str(), this->m_fileHandle);
		}
		for(int i=0; i<(int)(m_szStrItem.size()); i++){
			if( strCurSection ==  m_szStrItem[i].first ){
				if((m_szStrItem[i].second.first == "##") && (m_szStrItem[i].second.second ==""))
					strTmp = "\n";
				else if((m_szStrItem[i].second.first == "##") && (m_szStrItem[i].second.second !=""))
					strTmp = m_szStrItem[i].second.second+ "\n";
				else
					strTmp = m_szStrItem[i].second.first + " = " + m_szStrItem[i].second.second+ "\n";

				fputs(strTmp.c_str(), this->m_fileHandle);
			}
			else{
				strCurSection = m_szStrItem[i].first;
				if(strCurSection != "###"){
					string strTmp = "[" + strCurSection + "]"+ "\n";
					fputs(strTmp.c_str(), this->m_fileHandle);
			}
			if((m_szStrItem[i].second.first == "##") && (m_szStrItem[i].second.second ==""))
				strTmp = "\n";
			else if((m_szStrItem[i].second.first == "##") && (m_szStrItem[i].second.second !=""))
				strTmp = m_szStrItem[i].second.second+ "\n";
			else
				strTmp = m_szStrItem[i].second.first + " = " + m_szStrItem[i].second.second+ "\n";
			fputs(strTmp.c_str(), this->m_fileHandle);
			}
		}
	}
	this->CloseIni();
	return 1;
}


void CIniFile::DeleteEnter(char *p_str, int size)
{
	int i_len = (int)(strlen(p_str));
	if(p_str[i_len-1] == 0x0d || p_str[i_len-1] == 0x0a)
		p_str[i_len-1] = 0x00;
	i_len =  (int)(strlen(p_str));
	if(p_str[i_len-1] == 0x0d || p_str[i_len-1] == 0x0a)
		p_str[i_len-1] = 0x00;
}
