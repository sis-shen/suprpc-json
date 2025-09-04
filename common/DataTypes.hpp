/**
 * @file DataTypes.hpp
 * @author supdriver
 * @date 2025-05-26
 * @brief 消息数据对象定义
 * @version 1.6.0
 */
#pragma once
#include <string>
#include <unordered_map>

#define KEY_METHOD "method"
#define KEY_PARAMS "parameters"
#define KEY_TOPIC_KEY "topic_key"
#define KEY_TOPIC_MSG "topic_msg"
#define KEY_OPTYPE "optype"
#define KEY_HOST "host"
#define KEY_HOST_IP "ip"
#define KEY_HOST_PORT "port"
#define KEY_RCODE "rcode"
#define KEY_RESULT "result"

namespace suprpc
{
    /**
     * @class MType
     * @brief 消息类型定义
     */
    enum class MType
    {
        REQ_RPC = 0,
        RSP_RPC,
        REQ_TOPIC,
        RSP_TOPIC,
        REQ_SERVICE,
        RSP_SERVICE
    };
    /**
     * @class RCode
     * @brief 错误码定义
     */
    enum class RCode
    {
        RCODE_OK = 0,
        RCODE_PARSE_FAILED,
        RCODE_ERROR_MSGTYPE,
        RCODE_INVALID_MSG,
        RCODE_DISCONNECTED,
        RCODE_INVALID_PARAMS,
        RCODE_NOT_FOUND_SERVICE,
        RCODE_INVALID_OPTYPE,
        RCODE_NOT_FOUND_TOPIC,
        RCODE_INTERNAL_ERROR
    };

    /**
     * @brief 错误码转错误原因
     * @param code 错误码
     * @return 错误字符串
     */
    static std::string errReason(RCode code)
    {
        static std::unordered_map<RCode, std::string> err_map = {
            {RCode::RCODE_OK, "处理成功！"},
            {RCode::RCODE_PARSE_FAILED, "消息处理失败！"},
            {RCode::RCODE_ERROR_MSGTYPE, "消息类型错误！"},
            {RCode::RCODE_INVALID_MSG, "非法消息！"},
            {RCode::RCODE_DISCONNECTED, "联机已断开！"},
            {RCode::RCODE_INVALID_PARAMS, "非法参数！"},
            {RCode::RCODE_NOT_FOUND_SERVICE, "找不到对应的服务！"},
            {RCode::RCODE_INVALID_OPTYPE, "无效的操作类型！"},
            {RCode::RCODE_NOT_FOUND_TOPIC, "找不到对应的主题！"},
            {RCode::RCODE_INTERNAL_ERROR, "内部错误！"}};
        auto iter = err_map.find(code);
        if (iter == err_map.end())
        {
            return "未知错误！";
        }
        else
        {
            return iter->second;
        }
    };

    /**
     * @class RType
     * @brief RPC请求类型定义
     */
    enum class RType
    {
        REQ_ASYNC = 0,
        REQ_SYNC,
        REQ_CALLBACK
    };

    /**
     * @class TopicOptype
     * @brief 主题相关操作定义
     */
    enum class TopicOptype
    {
        TOPIC_CRAETE = 0,
        TOPIC_REMOVE,
        TOPIC_SUBSCRIBE,
        TOPIC_CANCEL,
        TOPIC_PUBLISH
    };

    /**
     * @class ServiceOptype
     * @brief 服务操作类型定义
     */
    enum class ServiceOptype
    {
        SERVICE_REGISTRY = 0,
        SERVICE_DISCOVERY,
        SERVICE_ONLINE,
        SERVICE_OFFLINE,
        SERVICE_UNKOWN
    };

    /**
     * @class Address
     * @brief 地址类的封装
     */
    class Address
    {
    public:
        Address(const std::string &host, int port) : first(host), second(port)
        {
        }
        std::string first;
        int second;

        bool operator==(const Address &addr)
        {
            return addr.first == first && addr.second == second;
        }
    };

} // namespace suprpc
