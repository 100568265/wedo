#include <iostream>
#include "http_client.h"

void handle_func(std::string rsp)
{
	// do sth according to rsp
	std::cout << "http rsp1: " << rsp << std::endl;
}

int main()
{
	// 拼完整url，带参数
//	std::string url1 = "http://192.168.0.146:8082";
//	HttpClient::SendReq(url1, handle_func);
	
	std::string url2 = "http://47.111.14.175:8088/CloudIot/MesRoot/DeviceIot/Upload";
	HttpClient::SendReq(url2, [](std::string rsp) { 
		std::cout << "http rsp2: " << rsp << std::endl; 
	});
	
	system("pause");

	return 0;
}