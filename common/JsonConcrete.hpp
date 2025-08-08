/**
 * @file JsonConcrete
 * @brief 消息抽象的Json实现
 */

#include "Base.hpp"
#include "JsonProto.hpp"
#include "logger.hpp"

namespace suprpc
{
    /**
     * @class JsonMessage
     * @brief 消息类的Json实现
     */
    class JsonMessage : public BaseMessage
    {
    public:
        using ptr = std::shared_ptr<JsonMessage>;
        virtual std::string serialize() override
        {
            std::string body;
            bool ret = JSON::serialize(_body, body);
            if (ret == false)
            {
                return std::string();
            }
            return body;
        }

        virtual bool deserialize(const std::string &msg) override
        {
            return JSON::deserialize(msg, _body);
        }

    protected:
        Json::Value _body;
    };

    /**
     * @class JsonRequest
     * @brief 请求类的Json实现
     */
    class JsonRequest : public JsonMessage
    {
    public:
        using ptr = std::shared_ptr<JsonMessage>;
    };

    /**
     * @class JsonResponse
     * @brief  响应类的Json实现
     */
    class JsonResponse : public JsonMessage
    {
    public:
        using ptr = std::shared_ptr<JsonResponse>;

        virtual bool check() override
        {
            if (_body[KEY_RCODE].isNull() == true)
            {
                SUP_LOG_ERROR("响应中没有状态码！");
                return false;
            }
            if (_body[KEY_RCODE].isIntegral() == false)
            {
                SUP_LOG_ERROR("响应状态码类型错误！");
                return false;
            }
            return true;
        }

        virtual RCode rcode()
        {
            return (RCode)_body[KEY_RCODE].asInt();
        }

        virtual void setRCode(const RCode rcode)
        {
            _body[KEY_RCODE] = (int)rcode;
        }
    };
}