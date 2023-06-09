#ifndef INIFILE_H_INCLUDED
#define INIFILE_H_INCLUDED

#include <string>
#include <vector>

using namespace std;

class CLogFileException{
    string m_strError;

public:
    inline CLogFileException(string p_strError){ m_strError = p_strError; }
    inline string OnMessage(){ return this->m_strError; }
};

class CIniFile
{
public:
    CIniFile(const string fileName);
    ~CIniFile(void);

public:
    string GetValue(const string section, const string key);
    int SetValue(const string section, const string key, const string value);
    void Print();
    int OpenIni(const string fileName);
    int OpenIni();
    void CloseIni();
protected:

private:
	string TrimL(string value);
	string TrimR(string value);
	string Trim(string value);
	void DeleteEnter(char *p_str, int size);
private:
	vector<pair<string, pair<string,string> > >  m_szStrItem;
    string m_strFileName;
    FILE*  m_fileHandle;
};


#endif //INIFILE_H_INCLUDED
