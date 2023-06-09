#include "http_server.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"


SysLogger *m_pLogger2;

std::string HttpServer::rechargeDate;
uint64_t HttpServer::ownerNo;
int HttpServer::operateType;
int HttpServer::rechargeQuality;
int HttpServer::rechargeNumber;
int HttpServer::totalQuality;
int HttpServer::presetBatteryLevel;


ST_VOID *HttpServer::RunServer(ST_VOID *param)
{
    if (param == nullptr)
    {
        return nullptr;
    }
    //HistoryStorage *vs = (HistoryStorage*)param;

    HttpServer *ss = static_cast<HttpServer*>(param);
    ss->run();
    //m_pLogger->GetInstance()->LogInfo("RunStorage is already runing");
    return nullptr;
}



ST_VOID HttpServer::Start()
{
    //run = true;
    server_thread.Start(RunServer,this,true);
    m_pLogger2->GetInstance()->LogInfo("Start is already runing");
}

ST_VOID HttpServer::Stop()
{
    //run = false;
    server_thread.Stop();
    m_pLogger2->GetInstance()->LogInfo("Stop is already runing");
}


HttpServer::HttpServer(const std::string& address, int port) {


    mg_mgr_init(&mgr, nullptr);
    mg_connection* connection = mg_bind(&mgr, (address + ":" + std::to_string(port)).c_str(), event_handler);
    if (connection) {
        mg_set_protocol_http_websocket(connection);
        std::cout << "Server started at " << address << ":" << port << std::endl;
    } else {
        std::cerr << "Error starting server." << std::endl;
        exit(1);
    }
}

HttpServer::~HttpServer() {
    mg_mgr_free(&mgr);
}

ST_VOID HttpServer::run() {
    while (true) {
        mg_mgr_poll(&mgr, 1000);
    }
}

void HttpServer::event_handler(mg_connection* connection, int event, void* event_data) {
    if (event == MG_EV_HTTP_REQUEST) {
        http_message* http_message = static_cast<struct http_message*>(event_data);
        std::string json_data(http_message->body.p, http_message->body.len);
        handle_json_data(connection, json_data);
    }
}

void HttpServer::handle_json_data(mg_connection* connection, std::string& json_data) {
    std::cout << "Received JSON data: " << json_data << std::endl;

    // Process JSON data from Java client here
    // ...
    // 解析JSON数据并存储到ElectricMeter对象
    rapidjson::Document received_json;
    if (!received_json.Parse(json_data.c_str()).HasParseError()) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer <rapidjson::StringBuffer> writer(buffer);
        received_json.Accept(writer);

        std::cout << "Received JSON data from Postman: " << buffer.GetString() << std::endl;

        // 解析JSON数据并存储到ElectricMeter对象
        if (received_json.HasMember("rechargeDate") && received_json["rechargeDate"].IsString()) {
            rechargeDate = received_json["rechargeDate"].GetString();
        }
        if (received_json.HasMember("ownerNo") && received_json["ownerNo"].IsString()) {
            ownerNo = std::stoull(received_json["ownerNo"].GetString());
        }
        if (received_json.HasMember("operateType") && received_json["operateType"].IsString()) {
            operateType = std::stoi(received_json["operateType"].GetString());
        }
        if (received_json.HasMember("rechargeQuality") && received_json["rechargeQuality"].IsString()) {
            rechargeQuality = std::stoi(received_json["rechargeQuality"].GetString());
        }
        if (received_json.HasMember("rechargeNumber") && received_json["rechargeNumber"].IsString()) {
            rechargeNumber = std::stoi(received_json["rechargeNumber"].GetString());
        }
        if (received_json.HasMember("totalQuality") && received_json["totalQuality"].IsString()) {
            totalQuality = std::stoi(received_json["totalQuality"].GetString());
        }
        if (received_json.HasMember("presetBatteryLevel") && received_json["presetBatteryLevel"].IsString()) {
            presetBatteryLevel = std::stoi(received_json["presetBatteryLevel"].GetString());
        }
        HttpServer::printVariables();

    }

        // Send response
    mg_printf(connection, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 2\r\n\r\nOK");
    connection->flags |= MG_F_SEND_AND_CLOSE;
}

void HttpServer::printVariables()
{
    std::cout << "rechargeDate = " << rechargeDate << std::endl;
    std::cout << "ownerNo = " << ownerNo << std::endl;
    std::cout << "OperateType = " << operateType << std::endl;
    std::cout << "RechargeQuality = " << rechargeQuality << std::endl;
    std::cout << "RechargeNumber = " << rechargeNumber << std::endl;
    std::cout << "TotalQuality = " << totalQuality << std::endl;
    std::cout << "PresetBatteryLevel = " << presetBatteryLevel << std::endl;
}
