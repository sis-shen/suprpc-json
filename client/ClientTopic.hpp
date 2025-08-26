/**
 * @file ClientTopic.hpp
 * @brief 客户端类代码法封装
 */

 #pragma once
 #include "Requestor.hpp"
 #include "../common/UuidGen.hpp"
 #include <unordered_set>

 namespace suprpc{
    namespace client{
        class TopicManager{
            public:
                using SubCallback = std::function<void (const std::string&,const std::string&)>;
                using ptr = std::shared_ptr<TopicManager>;
                TopicManager(const Requestor::ptr &requestor):_requestor(requestor){}

                bool create(const BaseConnection::ptr &conn,const std::string&key){
                    return commonRequest(conn,key,TopicOptype::TOPIC_CRAETE);
                }

                bool remove(const BaseConnection::ptr &conn,const std::string&key){
                    return commonRequest(conn,key,TopicOptype::TOPIC_REMOVE);
                }

                bool subscribe(const BaseConnection::ptr &conn,
                    const std::string&key,const SubCallback &cb){
                        addSubscribe(key,cb);
                        bool ret = commonRequest(conn,key,TopicOptype::TOPIC_SUBSCRIBE);
                        if(ret == false){
                            delSubscribe(key);
                            return false;
                        }
                        return true;
                    }
                bool cancel(const BaseConnection::ptr &conn,const std::string&key){
                    delSubscribe(key);
                    return commonRequest(conn,key,TopicOptype::TOPIC_CANCEL);
                }
                bool publish(const BaseConnection::ptr &conn,const std::string&key,const std::string&msg){
                    return commonRequest(conn,key,TopicOptype::TOPIC_PUBLISH,msg);
                }
                void onPublish(const BaseConnection::ptr &conn,const TopicRequest::ptr &msg){
                    auto type = msg->optype();
                    if(type != TopicOptype::TOPIC_PUBLISH) {
                        SUP_LOG_ERROR("收到了错误类型的主题操作!");
                        return;
                    }

                    std::string topic_key = msg->topicKey();
                    std::string topic_msg = msg->topicMsg();
                    auto callback = getSubscribe(topic_key);
                    if(!callback){
                        SUP_LOG_ERROR("收到了{}主题消息，但该消息无主题处理回调！",topic_key);
                        return ;
                    }
                    return callback(topic_key,topic_msg );
                }
            private:
                void addSubscribe(const std::string&key,const SubCallback&cb){
                    std::unique_lock<std::mutex>lock(_mutex);
                    _topic_callbacks.insert(std::make_pair(key,cb));
                }
                void delSubscribe(const std::string&key)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _topic_callbacks.erase(key);
                }

                const SubCallback getSubscribe(const std::string&key){
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _topic_callbacks.find(key);
                    if(it == _topic_callbacks.end()){
                        return SubCallback();
                    }
                    return it->second;
                }

                bool commonRequest(const BaseConnection::ptr &conn,
                    const std::string& key,
                    TopicOptype toptype,
                    const std::string&msg = ""
                ){
                    auto msg_req = MessageFactory::create<TopicRequest>();
                    msg_req->setId(uuid());
                    msg_req->setMType(MType::REQ_TOPIC);
                    msg_req->setOptype(toptype);
                    msg_req->setTopicKey(key);
                    if(toptype==TopicOptype::TOPIC_PUBLISH){
                        msg_req->setTopicMsg(msg);
                    }
                    BaseMessage::ptr msg_rsp;
                    bool ret = _requestor->send(conn,msg_req,msg_rsp);
                    if(ret == false){
                        SUP_LOG_ERROR("主题操作请求失败");
                    }

                    auto topic_rsp_msg = std::dynamic_pointer_cast<TopicResponse>(msg_rsp);
                    if(!topic_rsp_msg){
                        SUP_LOG_ERROR("主题操作响应，向下类型转换失败");
                        return false;
                    }

                    if(topic_rsp_msg->rcode() != RCode::RCODE_OK){
                        SUP_LOG_ERROR("主题操作请求出错：{}",errReason(topic_rsp_msg->rcode()));
                        return false;
                    }
                    return true;
                }
            private:
                std::mutex _mutex;
                Requestor::ptr _requestor;
                std::unordered_map<std::string,SubCallback> _topic_callbacks;
        };
    }
 }