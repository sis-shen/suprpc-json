/**
 * @file jsonProti.hpp
 * @author supdriver
 * @date 2025-05-26
 * @brief Json序列化与反序列化
 * @version 2.6.0
 */

#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <jsoncpp/json/json.h>

/**
 * @brief 实现json对象的序列化
 * @param val 输出的json对象
 * @param body 报文字符串
 */
namespace suprpc
{
static bool serialize(const Json::Value &val, std::string &body)
{
    std::stringstream ss;
    Json::StreamWriterBuilder swb;
    std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
    int ret = sw->write(val, &ss);
    if (ret != 0)
    {
        std::cout << "json sertialize failed\n";
        return false;
    }
    body = ss.str();
    return true;
}

/**
 * @brief 实现字符串反序列化成json对象
 * @param body 输出报文字符串
 * @param val 输入的json对象
 */
static bool unserialize(const std::string &body, Json::Value &val)
{
    Json::CharReaderBuilder crb;
    std::string errs;
    std::unique_ptr<Json::CharReader> cr(crb.newCharReader());

    bool ret = cr->parse(body.c_str(), body.c_str() + body.size(), &val, &errs);
    if (ret == false)
    {
        std::cout << "json unserialize failed: " << errs << std::endl;
        return false;
    }
    return true;
}
}