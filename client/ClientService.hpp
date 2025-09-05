/**
 * @file ClientService.hpp
 * @brief 客户端服务注册与发现实现
 */

#pragma once
#include "Requestor.hpp"
#include "../common/UuidGen.hpp"
#include <unordered_map>

namespace suprpc
{
    namespace client
    {
        /**
         * @class Provider
         * @brief 服务提供者类
         */
        class Provider
        {
        public:
            using ptr = std::shared_ptr<Provider>;
            Provider(const Requestor::ptr &requestor) : _requestor(requestor) {}

            bool registryMethod(const BaseConnection::ptr &conn,
                                const std::string &method, const Address &host)
            {
                auto msg_req = MessageFactory::create<ServiceRequest>();
                msg_req->setId(uuid());
                msg_req->setMType(MType::REQ_SERVICE);
                msg_req->setMethod(method);
                msg_req->setHost(host);
                msg_req->setOptype(ServiceOptype::SERVICE_REGISTRY);
                BaseMessage::ptr msg_rsp;
                bool ret = _requestor->send(conn, msg_req, msg_rsp);
                if (ret == false)
                {
                    SUP_LOG_ERROR("{} 服务注册失败！", method);
                    return false;
                }
                auto service_rsp = std::dynamic_pointer_cast<ServiceResponse>(msg_rsp);
                if (service_rsp.get() == nullptr)
                {
                    SUP_LOG_ERROR("响应类型向下转换失败!");
                    return false;
                }
                if (service_rsp->rcode() != RCode::RCODE_OK)
                {
                    SUP_LOG_ERROR("服务注册失败，原因: {}", errReason(service_rsp->rcode()));
                    return false;
                }
                return true;
            }

        private:
            Requestor::ptr _requestor;
        };

        class MethodHost
        {
        public:
            using ptr = std::shared_ptr<MethodHost>;
            MethodHost() : _idx(0) {}
            MethodHost(const std::vector<Address> &hosts) : _hosts(hosts.begin(), hosts.end()), _idx(0) {}

            void appendHost(const Address &host)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _hosts.push_back(host);
            }

            void removeHost(const Address &host)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                for (auto it = _hosts.begin(); it != _hosts.end(); ++it)
                {
                    if (*it == host)
                    {
                        _hosts.erase(it);
                        break;
                    }
                }
            }

            Address chooseHost()
            {
                std::unique_lock<std::mutex> lock(_mutex);
                size_t pos = _idx++ % _hosts.size();
                return _hosts[pos];
            }

            bool empty()
            {
                std::unique_lock<std::mutex> lock(_mutex);
                return _hosts.empty();
            }

        private:
            std::mutex _mutex;
            size_t _idx;
            std::vector<Address> _hosts;
        };

        /**
         * @class Discoverer
         * @brief 服务发现代码封装
         */
        class Discoverer
        {
        public:
            using OfflineCallback = std::function<void(const Address &)>;
            using ptr = std::shared_ptr<Discoverer>;

            Discoverer(const Requestor::ptr &requestor, const OfflineCallback &cb)
                : _requestor(requestor), _offline_callback(cb) {}

            bool serviceDiscovery(const BaseConnection::ptr &conn, const std::string &method, Address &host)
            {
                {std::unique_lock<std::mutex> lock(_mutex);
                auto it = _method_hosts.find(method);
                if (it != _method_hosts.end())
                {
                    if (it->second->empty() == false)
                    {
                        host = it->second->chooseHost();
                        return true;
                    }
                }}

                auto msg_req = MessageFactory::create<ServiceRequest>();
                msg_req->setId(uuid());
                msg_req->setMType(MType::REQ_SERVICE);
                msg_req->setMethod(method);
                msg_req->setOptype(ServiceOptype::SERVICE_DISCOVERY);
                BaseMessage::ptr msg_rsp;
                bool ret = _requestor->send(conn, msg_req, msg_rsp);
                if (ret == false)
                {
                    SUP_LOG_ERROR("服务发现失败!");
                    return false;
                }
                auto service_rsp = std::dynamic_pointer_cast<ServiceResponse>(msg_rsp);
                if (!service_rsp)
                {
                    SUP_LOG_ERROR("服务发现失败！响应类型转换失败！");
                    return false;
                }
                if (service_rsp->rcode() != RCode::RCODE_OK)
                {
                    SUP_LOG_ERROR("服务发现失败！ {}", errReason(service_rsp->rcode()));
                    return false;
                }
                std::unique_lock<std::mutex> lock(_mutex);
                auto method_host = std::make_shared<MethodHost>(service_rsp->hosts());

                if (method_host->empty())
                {
                    return false;
                }
                host = method_host->chooseHost();
                return true;
            }

            // 给Dispatcher用的
            void onServiceRequest(const BaseConnection::ptr &conn, const ServiceRequest::ptr &msg)
            {
                auto optype = msg->optype();
                std::string method = msg->method();
                std::unique_lock<std::mutex> lock(_mutex);
                if (optype == ServiceOptype::SERVICE_ONLINE)
                {
                    auto it = _method_hosts.find(method);
                    if (it == _method_hosts.end())
                    {
                        auto method_host = std::make_shared<MethodHost>();
                        method_host->appendHost(msg->host());
                        _method_hosts[method] = method_host;
                    }
                    else
                    {
                        it->second->appendHost(msg->host());
                    }
                }
                else if (optype == ServiceOptype::SERVICE_OFFLINE)
                {
                    auto it = _method_hosts.find(method);
                    if (it == _method_hosts.end())
                    {
                        return;
                    }
                    it->second->removeHost(msg->host());
                    _offline_callback(msg->host());
                }
            }

        private:
            OfflineCallback _offline_callback;
            std::mutex _mutex;
            std::unordered_map<std::string, MethodHost::ptr> _method_hosts;
            Requestor::ptr _requestor;
        };

    }
}