/**
 * @file Message.hpp
 * @brief 针对不同类型的消息的封装
 */

 #include "Base.hpp"
 #include "DataTypes.hpp"
 #include "JsonConcrete.hpp"
 namespace suprpc{
    /**
     * @class RpcRequest
     * @brief rpc请求的实现
     */
    class RpcRequest: public JsonRequest{
        public:
        using ptr = std::shared_ptr<RpcRequest>;
        virtual bool check()override{
            //rpc请求中，包含请求方法名称（字符串）,参数字段（对象）
            if(_body[KEY_METHOD].isNull() == true ||
                _body[KEY_METHOD].isString() == false)
                {
                    SUP_LOG_ERROR("RPC请求中没有方法名或者方法类型错误！");
                    return false;
                }
            if(_body[KEY_PARAMS].isNull() == true || 
            _body[KEY_PARAMS].isObject() == false)
            {
                SUP_LOG_ERROR("RPC请求中没有参数或者参数类型错误！");
                return false;
            }
            return true;
        }
        std::string method(){
            return _body[KEY_METHOD].asString();
        }

        void setMethos(const std::string& method){
            _body[KEY_METHOD] = method;
        }
        Json::Value params(){
            return _body[KEY_PARAMS];
        }
        void setParams(const Json::Value params){
            _body[KEY_PARAMS] = params;
        }
    }; 

    /**
     * @class TopicRequest
     * @brief 话题请求的实现
     */
    class TopicRequest:public JsonRequest{
        public:
        using ptr = std::shared_ptr<TopicRequest>;

        virtual bool check()override{
            if(_body[KEY_TOPIC_KEY].isNull() == true ||
                _body[KEY_TOPIC_KEY].isString() == false){
                    SUP_LOG_ERROR("主题请求中没有主题名称或主题名称类型错误！");
                    return false;
                }
            if(_body[KEY_OPTYPE].isNull() == true || 
                _body[KEY_OPTYPE].isIntegral() == false)
                {
                    SUP_LOG_ERROR("主题请求中没有操作类型或者操作类型的类型错误！");
                    return false;
                }
            if(_body[KEY_OPTYPE].asInt() == (int)TopicOptype::TOPIC_PUBLISH && 
                (_body[KEY_TOPIC_MSG].isNull() == true || _body[KEY_TOPIC_MSG].isString() ==false)){
                    SUP_LOG_ERROR("主题消息发布中没有消息内容字段或消息内容类型错误！");
                    return false;
                }

            return true;
        }

        std::string topicKey(){
            return _body[KEY_TOPIC_KEY].asString();
        }

        void setTopicKey(const std::string &key){
            _body[KEY_TOPIC_KEY] = key;
        }

        TopicOptype optype(){
            return (TopicOptype)_body[KEY_OPTYPE].asInt();
        }

        void setOptype(const TopicOptype optype){
            _body[KEY_OPTYPE] = (int)optype;
        }

        std::string topicMsg(){
            return _body[KEY_TOPIC_MSG].asString();
        }

        void setTopicMsg(const std::string&msg){
            _body[KEY_TOPIC_MSG] = msg;
        }

    };

    /**
     * @class ServiceRequest
     * @brief 服务请求类的实现
     */
    class ServiceRequest:public  JsonRequest{
        virtual bool check()override{
            if(_body[KEY_METHOD].isNull() == true || 
                _body[KEY_METHOD].isString() == false){
                    SUP_LOG_ERROR("服务请求方法中没有方法字段或者方法类型错误");
                    return false;
                }

            if(_body[KEY_OPTYPE].isNull() == true || 
                _body[KEY_OPTYPE].isIntegral() == false)
                {
                    SUP_LOG_ERROR("服务请求中没有操作类型或者操作类型的类型错误！");
                    return false;
                }
            if(_body[KEY_OPTYPE].asInt() != (int)ServiceOptype::SERVICE_DISCOVERY && 
                (_body[KEY_HOST].isNull() == true || _body[KEY_HOST].isObject() ==false ||
                _body[KEY_HOST][KEY_HOST_IP].isNull() ==true ||
                _body[KEY_HOST][KEY_HOST_IP].isString() == false ||
                _body[KEY_HOST][KEY_HOST_PORT].isNull() == true ||
                _body[KEY_HOST][KEY_HOST_PORT].isIntegral() == false)){
                    SUP_LOG_ERROR("服务请求中没有主机内容字段或主机类型错误！");
                    return false;
                }

            return true;
        }

        std::string method(){
            return _body[KEY_METHOD].asString();
        }

        void setMethod(const std::string& method){
            _body[KEY_METHOD] = method;
        }

        ServiceOptype optype(){
            return (ServiceOptype)_body[KEY_OPTYPE].asInt();
        }

        void setOptype(const ServiceOptype optype){
            _body[KEY_OPTYPE] = (int)optype;
        }

        Address host(){
            Address addr;
            addr.first = _body[KEY_HOST][KEY_HOST_IP].asString();
            addr.second = _body[KEY_HOST][KEY_HOST_PORT].asInt();
        }

        void setHost(const Address& addr){
            Json::Value val;
            val[KEY_HOST_IP] = addr.first;
            val[KEY_HOST_PORT] = addr.second;
            _body[KEY_HOST] = val;
        }
    }
 }