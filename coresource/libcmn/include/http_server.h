#pragma once
#include <iostream>
#include <string>
#include "mongoose.h"
#include "systhread.h"
#include "syslogger.h"

class HttpServer {
public:
    HttpServer(const std::string& address = "127.0.0.1", int port = 60003);
    ~HttpServer();
    static void printVariables();

    ST_VOID run();
    ST_VOID  Stop();
    ST_VOID Start();

    //电表参数x
    static std::string rechargeDate;
    static uint64_t ownerNo;
    static int operateType;
    static int rechargeQuality;
    static int rechargeNumber;
    static int totalQuality;
    static int presetBatteryLevel;

private:
    static void event_handler(mg_connection* connection, int event, void* event_data);
    static void handle_json_data(mg_connection* connection, std::string& json_data);
    static ST_VOID	*RunServer(ST_VOID *param);
    mg_mgr mgr;
    Thread	      server_thread;
};

