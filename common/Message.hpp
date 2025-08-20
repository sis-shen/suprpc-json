/**
 * @file Message.hpp
 * @brief 针对不同类型的消息的封装
 */

#include "Base.hpp"
#include "DataTypes.hpp"
#include "JsonConcrete.hpp"
namespace suprpc
{
    /**
     * @class RpcRequest
     * @brief rpc请求的实现
     */
    class RpcRequest : public JsonRequest
    {
    public:
        using ptr = std::shared_ptr<RpcRequest>;
        virtual bool check() override
        {
            // rpc请求中，包含请求方法名称（字符串）,参数字段（对象）
            if (_body[KEY_METHOD].isNull() == true ||
                _body[KEY_METHOD].isString() == false)
            {
                SUP_LOG_ERROR("RPC请求中没有方法名或者方法类型错误！");
                return false;
            }
            if (_body[KEY_PARAMS].isNull() == true ||
                _body[KEY_PARAMS].isObject() == false)
            {
                SUP_LOG_ERROR("RPC请求中没有参数或者参数类型错误！");
                return false;
            }
            return true;
        }
        std::string method()
        {
            return _body[KEY_METHOD].asString();
        }

        void setMethos(const std::string &method)
        {
            _body[KEY_METHOD] = method;
        }
        Json::Value params()
        {
            return _body[KEY_PARAMS];
        }
        void setParams(const Json::Value params)
        {
            _body[KEY_PARAMS] = params;
        }
    };

    /**
     * @class TopicRequest
     * @brief 话题请求的实现
     */
    class TopicRequest : public JsonRequest
    {
    public:
        using ptr = std::shared_ptr<TopicRequest>;

        virtual bool check() override
        {
            if (_body[KEY_TOPIC_KEY].isNull() == true ||
                _body[KEY_TOPIC_KEY].isString() == false)
            {
                SUP_LOG_ERROR("主题请求中没有主题名称或主题名称类型错误！");
                return false;
            }
            if (_body[KEY_OPTYPE].isNull() == true ||
                _body[KEY_OPTYPE].isIntegral() == false)
            {
                SUP_LOG_ERROR("主题请求中没有操作类型或者操作类型的类型错误！");
                return false;
            }
            if (_body[KEY_OPTYPE].asInt() == (int)TopicOptype::TOPIC_PUBLISH &&
                (_body[KEY_TOPIC_MSG].isNull() == true || _body[KEY_TOPIC_MSG].isString() == false))
            {
                SUP_LOG_ERROR("主题消息发布中没有消息内容字段或消息内容类型错误！");
                return false;
            }

            return true;
        }

        std::string topicKey()
        {
            return _body[KEY_TOPIC_KEY].asString();
        }

        void setTopicKey(const std::string &key)
        {
            _body[KEY_TOPIC_KEY] = key;
        }

        TopicOptype optype()
        {
            return (TopicOptype)_body[KEY_OPTYPE].asInt();
        }

        void setOptype(const TopicOptype optype)
        {
            _body[KEY_OPTYPE] = (int)optype;
        }

        std::string topicMsg()
        {
            return _body[KEY_TOPIC_MSG].asString();
        }

        void setTopicMsg(const std::string &msg)
        {
            _body[KEY_TOPIC_MSG] = msg;
        }
    };

    /**
     * @class ServiceRequest
     * @brief 服务请求类的实现
     */
    class ServiceRequest : public JsonRequest
    {
    public:
        virtual bool check() override
        {
            if (_body[KEY_METHOD].isNull() == true ||
                _body[KEY_METHOD].isString() == false)
            {
                SUP_LOG_ERROR("服务请求方法中没有方法字段或者方法类型错误");
                return false;
            }

            if (_body[KEY_OPTYPE].isNull() == true ||
                _body[KEY_OPTYPE].isIntegral() == false)
            {
                SUP_LOG_ERROR("服务请求中没有操作类型或者操作类型的类型错误！");
                return false;
            }
            if (_body[KEY_OPTYPE].asInt() != (int)ServiceOptype::SERVICE_DISCOVERY &&
                (_body[KEY_HOST].isNull() == true || _body[KEY_HOST].isObject() == false ||
                 _body[KEY_HOST][KEY_HOST_IP].isNull() == true ||
                 _body[KEY_HOST][KEY_HOST_IP].isString() == false ||
                 _body[KEY_HOST][KEY_HOST_PORT].isNull() == true ||
                 _body[KEY_HOST][KEY_HOST_PORT].isIntegral() == false))
            {
                SUP_LOG_ERROR("服务请求中没有主机内容字段或主机类型错误！");
                return false;
            }

            return true;
        }

        std::string method()
        {
            return _body[KEY_METHOD].asString();
        }

        void setMethod(const std::string &method)
        {
            _body[KEY_METHOD] = method;
        }

        ServiceOptype optype()
        {
            return (ServiceOptype)_body[KEY_OPTYPE].asInt();
        }

        void setOptype(const ServiceOptype optype)
        {
            _body[KEY_OPTYPE] = (int)optype;
        }

        Address host()
        {
            Address addr;
            addr.first = _body[KEY_HOST][KEY_HOST_IP].asString();
            addr.second = _body[KEY_HOST][KEY_HOST_PORT].asInt();
        }

        void setHost(const Address &addr)
        {
            Json::Value val;
            val[KEY_HOST_IP] = addr.first;
            val[KEY_HOST_PORT] = addr.second;
            _body[KEY_HOST] = val;
        }
    };

    /**
     * @class RpcResponse
     * @brief RPC响应类的实现
     */
    class RpcResponse : public JsonResponse
    {
        public:
        using ptr = std::shared_ptr<RpcResponse>;
        virtual bool check() override
        {
            if (_body[KEY_RCODE].isNull() == true ||
                _body[KEY_RCODE].isIntegral() == false)
            {
                SUP_LOG_ERROR("RPC响应中无状态码或者状态码类型错误！");
                return false;
            }
            if (_body[KEY_RESULT].isNull() == true ||
                _body[KEY_RESULT].isObject() == false)
            {
                SUP_LOG_ERROR("RPC响应中无Rpc调用结果，或者结果类型错误！");
                return false;
            }
        }

        Json::Value result()
        {
            return _body[KEY_RESULT];
        }

        void setResult(const Json::Value &result)
        {
            _body[KEY_RESULT] = result;
        }
    };

    /**
     * @class TopicResponse
     * @brief 话题响应类
     */
    class TopicResponse : public JsonResponse
    {
    public:
        using ptr = std::shared_ptr<TopicResponse>;
    };

    /**
     * @class ServiceResponse
     * @brief 服务响应类
     */
    class ServiceResponse : public JsonResponse
    {
    public:
        using ptr = std::shared_ptr<ServiceResponse>;
        virtual bool check() override
        {
            if (_body[KEY_RCODE].isNull() == true ||
                _body[KEY_RCODE].isIntegral() == false)
            {
                SUP_LOG_ERROR("服务响应中无状态码或者状态码类型错误！");
                return false;
            }
            if (_body[KEY_OPTYPE].isNull() == true ||
                _body[KEY_OPTYPE].isIntegral() == false)
            {
                SUP_LOG_ERROR("服务响应中没有操作类型，或者操作类型的类型错误！");
                return false;
            }
            if (_body[KEY_OPTYPE].asInt() == (int)ServiceOptype::SERVICE_DISCOVERY &&
                (_body[KEY_METHOD].isNull() == true ||
                 _body[KEY_METHOD].isString() == false ||
                 _body[KEY_HOST].isNull() == true ||
                 _body[KEY_HOST].isArray() == false))
            {
                SUP_LOG_ERROR("服务发现响应中响应信息字段错误！");
                return false;
            }

            return true;
        }
        ServiceOptype optype()
        {
            return (ServiceOptype)_body[KEY_OPTYPE].asInt();
        }
        void setOptype(const ServiceOptype optype)
        {
            _body[KEY_OPTYPE] = (int)optype;
        }
        std::string method()
        {
            return _body[KEY_METHOD].asString();
        }

        void setMethod(const std::string &method)
        {
            _body[KEY_METHOD] = method;
        }

        std::vector<Address> hosts()
        {
            std::vector<Address> addrs;
            int sz = _body[KEY_HOST].size();
            for (int i = 0; i < sz; ++i)
            {
                Address addr;
                addr.first = _body[KEY_HOST][KEY_HOST_IP].asString();
                addr.second = _body[KEY_HOST][KEY_HOST_PORT].asInt();
                addrs.push_back(addr);
            }
            return addrs;
        }

        void setHosts(const std::vector<Address> &addrs)
        {
            for (auto &addr : addrs)
            {
                Json::Value val;
                val[KEY_HOST_IP] = addr.first;
                val[KEY_HOST_PORT] = addr.second;
                _body[KEY_HOST].append(val);
            }
        }
    };

    /**
     * @class MessageFactory
     * @brief 生成消息对象的工厂
     */
    class MessageFactory
    {
    public:
        static BaseMessage::ptr create(const MType mtype)
        {
            switch (mtype)
            {
            case MType::REQ_RPC:
                return std::make_shared<RpcRequest>();
            case MType::RSP_RPC:
                return std::make_shared<RpcResponse>();
            case MType::REQ_TOPIC:
                return std::make_shared<TopicRequest>();
            case MType::RSP_TOPIC:
                return std::make_shared<TopicResponse>();
            case MType::REQ_SERVICE:
                return std::make_shared<ServiceRequest>();
            case MType::RSP_SERVICE:
                return std::make_shared<ServiceResponse>();
            }
            return std::make_shared<BaseMessage>();
        }

        template <class T, class... Args>
        static std::shared_ptr<T> create(Args &&...args)
        {
            return std::make_shared<T>(std::forward(args)...);
        }
    };
}