/**
 * @file RpcCaller.hpp
 * @brief Rpc调用器封装
 */
#pragma once
#include "Requestor.hpp"
#include "../common/UuidGen.hpp"

namespace suprpc{
    namespace client{
        /**
         * @class RpcCaller
         * @brief Rpc调用器类
         */
        class RpcCaller{
            public:
                using ptr = std::shared_ptr<RpcCaller>;
                using JsonAsyncResponse = std::future<Json::Value>;
                using JsonResponseCallback = std::function<void (const Json::Value&)>;

                RpcCaller(const Requestor::ptr &requestor)
                :_requestor(requestor){}

                bool call(
                    const BaseConnection::ptr &conn,
                    const std::string&method,
                    const Json::Value&params,
                    Json::Value &result){
                        SUP_LOG_DEBUG("开始同步rpc调用...");
                        //1.组织请求
                        auto req_msg = MessageFactory::create<RpcRequest>();
                        req_msg->setId(uuid());
                        req_msg->setMType(MType::REQ_RPC);
                        req_msg->setMethod(method);
                        req_msg->setParams(params);
                        BaseMessage::ptr rsp_msg;

                        //2.发送请求
                        bool ret = _requestor->send(conn,std::dynamic_pointer_cast<BaseMessage>(req_msg),rsp_msg);

                        if(ret == false){
                            SUP_LOG_ERROR("同步Rpc请求失败");
                            return false;
                        }

                        SUP_LOG_DEBUG("收到响应，进行解析，获取结果！");
                        //3.等待响应
                        auto rpc_rsp_msg = std::dynamic_pointer_cast<RpcResponse>(rsp_msg);
                        if(!rpc_rsp_msg){
                            SUP_LOG_ERROR("rpc响应向下类型转换失败");
                            return false;
                        }
                        if(rpc_rsp_msg->rcode() != RCode::RCODE_OK)
                        {
                            SUP_LOG_ERROR("rpc请求出错: {}",errReason(rpc_rsp_msg->rcode()));
                            return false;
                        }
                        result = rpc_rsp_msg->result();
                        SUP_LOG_DEBUG("结果设置完毕！");
                        return true;
                    }

                    bool call(const BaseConnection::ptr &conn,
                        const std::string &method,
                        const Json::Value &params,JsonAsyncResponse &result){
                            auto req_msg = MessageFactory::create<RpcRequest>();
                            req_msg->setId(uuid());
                            req_msg->setMType(MType::REQ_RPC);
                            req_msg->setMethod(method);
                            req_msg->setParams(params);

                            auto json_promise = std::make_shared<std::promise<Json::Value>>();
                            result = json_promise->get_future();
                            Requestor::RequestCallback cb = std::bind(
                                &RpcCaller::Callback,this,json_promise,std::placeholders::_1
                            );
                            bool ret = _requestor->send(conn,std::dynamic_pointer_cast<BaseMessage>(req_msg),cb);
                            if(ret == false){
                                SUP_LOG_ERROR("异步Rpc请求失败!");
                                return false;
                            }
                            return true;
                        }
                bool call(const BaseConnection::ptr &conn,const std::string&method,
                    const Json::Value &params,const JsonResponseCallback &cb){
                        
                    }
            private:
            void Callback_(const JsonResponseCallback &cb,
                const BaseMessage::ptr &msg){
                    auto rpc_rsp_msg = std::dynamic_pointer_cast<RpcResponse>(msg);
                    if(!rpc_rsp_msg){
                        SUP_LOG_ERROR("rpc响应，向下类型转换失败");
                        return;
                    }
                    if(rpc_rsp_msg->rcode() != RCode::RCODE_OK){
                        SUP_LOG_ERROR("rpc回调请求出错: %s",errReason(rpc_rsp_msg->rcode()));
                        return;
                    }
                    cb(rpc_rsp_msg->result);
                }

            void Callback(std::shared_ptr<std::promise<Json::Value>> result,
                const BaseMessage::ptr &msg){
                    auto rpc_rsp_msg = std::dynamic_pointer_cast<RpcResponse>(msg);
                    if(!rpc_rsp_msg){
                        SUP_LOG_ERROR("rpc响应，向下类型转换失败！");
                        return;
                    }
                    if(rpc_rsp_msg->rcode() != RCode::RCODE_OK){
                        SUP_LOG_ERROR("rpc异步请求出错:%s",errReason(rpc_rsp_msg->rcode()));
                        return;
                    }
                    result->set_value(rpc_rsp_msg->result());
                }
            private:
                Requestor::ptr _requestor;
        }
    }
}