#include <iostream>

using namespace std;
#include "tinyxml2.h"
#include "TcpClientPort.h"
 #include <unistd.h>
#include "Manager.h"
using namespace tinyxml2;
int main()
{
    Manager manager;
    manager.load_configs();
    manager.start();
    return 0;
}
