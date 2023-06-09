#include <iostream>

using namespace std;
#include "tinyxml2.h"
#include "TcpClientPort.h"
 #include <unistd.h>
#include "Manager.h"
using namespace tinyxml2;

int main(int argc, char *argv[])
{
    Manager manager;
    int res = manager.load_configs();
    if(res){
        manager.start();
    }
    return 0;
}
