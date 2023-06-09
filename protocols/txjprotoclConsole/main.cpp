#include <iostream>
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "CIni.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>


#include <iostream>
#include <string>
#include <fstream>

using namespace std;


static bool readConfigFile(const char * cfgfilepath, const string & key, string & value)
{
    fstream cfgFile;
    cfgFile.open(cfgfilepath);//打开文件
    if( ! cfgFile.is_open())
    {
        cout<<"can not open cfg file!"<<endl;
        return false;
    }
    char tmp[1000];
    while(!cfgFile.eof())//循环读取每一行
    {
        cfgFile.getline(tmp,1000);//每行读取前1000个字符，1000个应该足够了
        string line(tmp);
        size_t pos = line.find('=');//找到每行的“=”号位置，之前是key之后是value
        if(pos==string::npos) return false;
        string tmpKey = line.substr(0,pos);//取=号之前
        if(key==tmpKey)
        {
            value = line.substr(pos+1);//取=号之后
            return true;
        }
    }
    return false;
}


//http://47.111.14.175:8088/CloudIot/DeviceIot/KeepAlive
/*{
   "UUID" : "4a9a5ea8-c68c-4690-85b7-bd580f1b4254",
   "data" : {
      "tmChargeTime" : "10.54",
      "tmClpClsTime" : "5.00",
      "tmClpOpnPosi" : "391.20",
      "tmClpOpnTime" : "2.13",
…
      "tmShotCount" : "9266",
      "tmTurnPosi" : "48.90",
      "tmTurnPress" : "0",
      "tmTurnTime" : "2.80"
   },
   "datetime" : "2019-05-06 12:14:48"
}
*/
int main()
{
    string cfgData;
    if(readConfigFile("data.config","age",cfgData))
    {
        cout<<cfgData<<endl;
    }
    else
    {
        cout<<"open file error"<<endl;
    }
}
