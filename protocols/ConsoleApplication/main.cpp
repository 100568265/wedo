#include <iostream>
#include <sstream>
#include "tinyxml2.h"
#include <time.h>
using namespace std;
using namespace tinyxml2;

string int2str(const int int_temp)
{
    stringstream stream;
    stream<<int_temp;
    return stream.str();   //此处也可以用 stream>>string_temp
}
string getTodayString()
{
    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);
    string date = int2str(t->tm_year+1900)+int2str(t->tm_mon+1)+int2str(t->tm_mday);
    return date;
}
int main()
{
    string deviceAddr = "1";
    string today = getTodayString();
    string filename = "storage/"+today+".xml";

    const char* declaration = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	XMLDocument doc;
	doc.Parse(declaration);

	XMLElement* root = doc.NewElement("History");
	root->SetAttribute("Day",today.c_str());
	doc.InsertEndChild(root);

	XMLElement* area = doc.NewElement("Area");
	area->SetAttribute("deviceId","1");
	root->InsertEndChild(area);

	XMLElement* data = doc.NewElement("Data");
	data->SetAttribute("id","1");
	data->SetAttribute("name","Ia");
	area->InsertEndChild(data);

    XMLElement* value = doc.NewElement("Value");
    value->SetAttribute("time","00:05");
    XMLText* vText = doc.NewText("100");
    value->InsertEndChild(vText);
	data->InsertEndChild(value);
	doc.SaveFile(filename.c_str());
	return 0;
}






















