/**
 * @file Service.hpp
 * @brief 服务端的服务注册与发现代码的封装
 */

#pragma once
#include "../common/MuduoTool.hpp"
#include "../common/Message.hpp"
#include "../common/UuidGen.hpp"
#include <set>

namespace suprpc
{
    namespace server
    {
        /**
         * @class ProviderManager
         * @brief 基于连接管理Provider对象
         */
        class ProviderManager
        {
        public:
            using ptr = std::shared_ptr<ProviderManager>;
            /**
             * @class Provider
             * @brief 对方法、连接和主机进行管理和提供服务
             */
            struct Provider
            {
                using ptr = std::shared_ptr<Provider>;
                std::mutex _mutex;
                BaseConnection::ptr _conn;
                Address _host;
                std::vector<std::string> _methods;
                Provider(
                    const BaseConnection::ptr &conn,
                    const Address &host) : _conn(conn), _host(host)
                {
                }
                void appendMethod(const std::string &method)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _methods.emplace_back(method);
                }
            };

            void addProvider(const BaseConnection::ptr &conn, const Address &host, const std::string &method)
            {
                Provider::ptr provider;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _conns.find(conn);
                    if (it != _conns.end())
                    {
                        provider = it->second;
                    }
                    else
                    {
                        provider = std::make_shared<Provider>(conn, host);
                        _conns.insert(std::make_pair(conn, provider));
                    }

                    auto &providers = _providers[method];
                    providers.insert(provider);
                }
                provider->appendMethod(method);
            }

            Provider::ptr getProvider(const BaseConnection::ptr &conn)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _conns.find(conn);
                if (it != _conns.end())
                {
                    return it->second;
                }
                return Provider::ptr();
            }

            void delProvider(const BaseConnection::ptr &conn)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _conns.find(conn);
                if (it == _conns.end())
                {
                    return;
                }

                for (auto &method : it->second->_methods)
                {
                    auto &providers = _providers[method];
                    providers.erase(it->second);
                }

                _conns.erase(it);
            }

            std::vector<Address> methodHosts(const std::string &method)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _providers.find(method);
                if (it == _providers.end())
                {
                    return std::vector<Address>();
                }
                std::vector<Address> result;
                for (auto &provider : it->second)
                {
                    result.push_back(provider->_host);
                }
                return result;
            }

        private:
            std::mutex _mutex;

            std::unordered_map<std::string, std::set<Provider::ptr>> _providers;
            std::unordered_map<BaseConnection::ptr, Provider::ptr> _conns;
        };

        /**
         * @class DiscovererManager
         * @brief 发现者的管理类
         */
        class DiscovererManager
        {
        public:
            using ptr = std::shared_ptr<DiscovererManager>;
            /**
             * @class Discoverer
             * @brief 发现者类
             */
            struct Discoverer
            {
                using ptr = std::shared_ptr<Discoverer>;
                std::mutex _mutex;
                BaseConnection::ptr _conn;
                std::vector<std::string> _methods;
                Discoverer(const BaseConnection::ptr &conn) : _conn(conn) {}
                void appendMethod(const std::string &method)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _methods.push_back(method);
                }
            };

            Discoverer::ptr addDiscoverer(
                const BaseConnection::ptr &conn,
                const std::string &method)
            {
                Discoverer::ptr discoverer;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _conns.find(conn);
                    if (it != _conns.end())
                    {
                        discoverer = it->second;
                    }
                    else
                    {
                        discoverer = std::make_shared<Discoverer>(conn);
                        _conns.insert(std::make_pair(conn, discoverer));
                    }
                    auto &discoverers = _discoverers[method];
                    discoverers.insert(discoverer);
                }
                discoverer->appendMethod(method);
                return discoverer;
            }

            // 删除该连接相关的Discoverer
            void delDiscoverer(const BaseConnection::ptr &conn)
            {
                std::unique_lock<std::mutex> _mutex;
                auto it = _conns.find(conn);
                if (it == _conns.end())
                {
                    return;
                }
                for (auto &method : it->second->_methods)
                {
                    auto discoverers = _discoverers[method];
                    discoverers.erase(it->second);
                }
            }

            void onlineNotify(const std::string &method, const Address &host)
            {
                notify(method, host, ServiceOptype::SERVICE_ONLINE);
            }

            void offlineNotify(const std::string &method, const Address &host)
            {
                notify(method, host, ServiceOptype::SERVICE_OFFLINE);
            }

        private:
            void notify(const std::string &method, const Address &host, ServiceOptype optype)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _discoverers.find(method);
                if (it == _discoverers.end())
                {
                    return;
                }
                ServiceRequest::ptr msg_req(MessageFactory::create<ServiceRequest>());
                msg_req->setId(uuid());
                msg_req->setMType(MType::REQ_SERVICE);
                msg_req->setMethod(method);
                msg_req->setHost(host);
                msg_req->setOptype(optype);
                for (auto &discoverer : it->second)
                {
                    discoverer->_conn->send(msg_req);
                }
            }

        private:
            std::mutex _mutex;
            std::unordered_map<std::string, std::set<Discoverer::ptr>> _discoverers;
            std::unordered_map<BaseConnection::ptr, Discoverer::ptr> _conns;
        };

        /**
         * @class PDManager
         * @brief Provider和Discoverer的管理类
         */
        class PDManager
        {
        public:
            using ptr = std::shared_ptr<PDManager>;
            PDManager() : _providers(std::make_shared<ProviderManager>()),
                          _discoverers(std::make_shared<DiscovererManager>())
            {
            }

            void onServiceRequest(const BaseConnection::ptr &conn, const ServiceRequest::ptr &msg)
            {
                ServiceOptype optype = msg->optype();
                if (optype == ServiceOptype::SERVICE_REGISTRY)
                {
                    SUP_LOG_INFO("{}:{} 注册服务 {}", msg->host().first, msg->host().second, msg->method());
                    _providers->addProvider(conn, msg->host(), msg->method());
                    _discoverers->onlineNotify(msg->method(), msg->host());
                    return registerResponse(conn, msg);
                }
                else if (optype == ServiceOptype::SERVICE_DISCOVERY)
                {
                    SUP_LOG_INFO("客户端要进行 {} 服务发现！", msg->method());
                    _discoverers->addDiscoverer(conn, msg->method());
                    return discoveryResponse(conn, msg);
                }
                else
                {
                    SUP_LOG_ERROR("收到服务操作请求，但操作类型错误！");
                    return errorResponse(conn, msg);
                }
            }

            void onConnShutdown(const BaseConnection::ptr &conn)
            {
                auto provider = _providers->getProvider(conn);
                if (provider.get() != nullptr)
                {
                    SUP_LOG_INFO("{}:{} 服务下线 ", provider->_host.first, provider->_host.second);
                    for (auto &method : provider->_methods)
                    {
                        _discoverers->offlineNotify(method, provider->_host);
                    }
                    _providers->delProvider(conn);
                }
                _discoverers->delDiscoverer(conn);
            }

        private:
            void errorResponse(const BaseConnection::ptr &conn, const ServiceRequest::ptr &msg)
            {
                auto msg_rsp = MessageFactory::create<ServiceResponse>();
                msg_rsp->setId(msg->rid());
                msg_rsp->setMType(MType::RSP_SERVICE);
                msg_rsp->setRCode(RCode::RCODE_INVALID_OPTYPE);
                msg_rsp->setOptype(ServiceOptype::SERVICE_UNKOWN);
                conn->send(msg_rsp);
            }
            void registerResponse(const BaseConnection::ptr &conn, const ServiceRequest::ptr &msg)
            {
                auto msg_rsp = MessageFactory::create<ServiceResponse>();
                msg_rsp->setId(msg->rid());
                msg_rsp->setMType(MType::RSP_SERVICE);
                msg_rsp->setRCode(RCode::RCODE_OK);
                msg_rsp->setOptype(ServiceOptype::SERVICE_REGISTRY);
                conn->send(msg_rsp);
            }
            void discoveryResponse(const BaseConnection::ptr &conn, const ServiceRequest::ptr &msg)
            {
                auto msg_rsp = MessageFactory::create<ServiceResponse>();
                msg_rsp->setId(msg->rid());
                msg_rsp->setMType(MType::RSP_SERVICE);
                msg_rsp->setOptype(ServiceOptype::SERVICE_DISCOVERY);
                std::vector<Address> hosts = _providers->methodHosts(msg->method());
                if (hosts.empty())
                {
                    msg_rsp->setRCode(RCode::RCODE_NOT_FOUND_SERVICE);
                    return conn->send(msg_rsp);
                }

                msg_rsp->setRCode(RCode::RCODE_OK);
                msg_rsp->setMethod(msg->method());
                msg_rsp->setHosts(hosts);
                conn->send(msg_rsp);
            }

        private:
            ProviderManager::ptr _providers;
            DiscovererManager::ptr _discoverers;
        };
    }
}