/**
 * @file Client.hpp
 * @brief 客户端类封装
 */

#pragma once
#include "../common/Dispatcher.hpp"
#include "Requestor.hpp"
#include "RpcCaller.hpp"
#include "ClientTopic.hpp"
#include "ClientService.hpp"

namespace suprpc{
    namespace client{
        /**
         * @class RegistryClient
         * @brief 注册客户端类
         */
        class RegistryClient{
            public:
            using ptr = std::shared_ptr<RegistryClient>;
            RegistryClient(const std::string &ip,int port)
            :_requestor(std::make_shared<Requestor>()),
            _provider(std::make_shared<Provider>(_requestor)),
            _dispatcher(std::make_shared<Dispatcher>())
            {
                auto rsp_cb = std::bind(&client::Requestor::onResponse,
                    _requestor.get(),
                    std::placeholders::_1,std::placeholders::_2);
                _dispatcher->registerHandler<BaseMessage> (MType::RSP_SERVICE,rsp_cb);
                auto message_cb = std::bind(&Dispatcher::onMessage,_dispatcher.get(),
                    std::placeholders::_1,std::placeholders::_2);
                _client = ClientFactory::create(ip,port);
                _client->setMessageCallback(message_cb);
                _client->connect();
            }

            bool registryMethod(const std::string &method,const Address&host) {
                return _provider->registryMethod(_client->connection(),method,host);
            }

            private:
                Requestor::ptr _requestor;
                client::Provider::ptr _provider;
                Dispatcher::ptr _dispatcher;
                BaseClient::ptr _client;
        };

        /**
         * @class DiscoveryClient
         * @brief 发现器客户端
         */
        class DiscoveryClient {
            public:
                using ptr = std::shared_ptr<DiscoveryClient>;
                DiscoveryClient(const std::string&ip,int port,
                    const Discoverer::OfflineCallback &cb):
                    _requestor(std::make_shared<Requestor>()),
                    _discoverer(std::make_shared<client::Discoverer>()),
                    _dispatcher(std::make_shared<Dispatcher>())
                    {
                        auto rsp_cb = std::bind(&client::Requestor::onResponse,
                            _requestor.get(),
                        std::placeholders::_1,std::placeholders::_2);
                        _dispatcher->registerHandler<BaseMessage>(MType::RSP_SERVICE,rsp_cb);
                        auto req_cb = std::bind(&client::Discoverer::onServiceRequest,
                            _discoverer.get(),
                            std::placeholders::_1,std::placeholders::_2);
                            _dispatcher->registerHandler<ServiceRequest>(MType::REQ_SERVICE,req_cb);

                            auto message_cb = std::bind(&Dispatcher::onMessage,_dispatcher.get(),
                            std::placeholders::_1,std::placeholders::_2);
                            _client = ClientFactory::create(ip,port);
                            _client->setMessageCallback(message_cb);
                            _client->connect();
                    }

                bool serviceDiscovery(const std::string&method,Address host) {
                    return _discoverer->serviceDiscovery(_client->connection(),method,host);
                }
            
            private:
                Requestor::ptr _requestor;
                client::Discoverer::ptr _discoverer;
                Dispatcher::ptr _dispatcher;
                BaseClient::ptr _client;
        };

        /**
         * @class RpcClient
         * @brief Rpc客户端
         */
        class RpcClient {
        public:
            RpcClient(bool enableDiscovery,const std::string&ip,int port)
                :_enableDiscovery(enableDiscovery),
                _requestor(std::make_shared<Requestor>()),
                _dispatcher(std::make_shared<Dispatcher>()),
                _caller(std::make_shared<client::RpcCaller>(_requestor))
            {
                auto rsp_cb = std::bind(&client::Requestor::onResponse,
                    _requestor.get(),
                    std::placeholders::_1,std::placeholders::_2) ;
                    _dispatcher->registerHandler<BaseMessage>(MType::RSP_RPC,rsp_cb);
                if(_enableDiscovery) {
                    auto offline_cb = std::bind(&RpcClient::delClient,this,std::placeholders::_1);
                    _discovery_client = std::make_shared<DiscoveryClient>(ip,port,offline_cb);
                }else {
                    auto message_cb = std::bind(&Dispatcher::onMessage,_dispatcher.get(),
                        std::placeholders::_1,std::placeholders::_2);
                        _rpc_client = ClientFactory::create(ip,port);
                        _rpc_client->setMessageCallback(message_cb);
                        _rpc_client->connect();
                }
            }

            bool call(const std::string &method,const Json::Value&params,
                Json::Value& result) {
                    BaseClient::ptr client = getClient(method);
                    if(client.get() == nullptr) {
                        return false;
                    }
                    return _caller->call(client->connection(),method,params,result);
                }

            bool call(const std::string &method,const Json::Value&params,
                RpcCaller::JsonAsyncResponse &result) {
                    BaseClient::ptr client = getClient(method);
                    if(client.get() == nullptr) {
                        return false;
                    }
                    return _caller->call(client->connection(),method,params,result);
                }

            bool call(const std::string &method,const Json::Value&params,
                RpcCaller::JsonResponseCallback &cb) {
                    BaseClient::ptr client = getClient(method);
                    if(client.get() == nullptr) {
                        return false;
                    }
                    return _caller->call(client->connection(),method,params,cb);
                }

            private:
                BaseClient::ptr newClient(const Address&host) {
                    auto message_cb = std::bind(&Dispatcher::onMessage,_dispatcher.get(),
                        std::placeholders::_1,std::placeholders::_2);
                        auto client = ClientFactory::create(host.first,host.second);
                        client->setMessageCallback(message_cb);
                        client->connect();
                        putClient(host,client);
                        return client;
                }

                BaseClient::ptr getClient(const Address&host){
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _rpc_clients.find(host);
                    if(it == _rpc_clients.end()){
                        return BaseClient::ptr();
                    }
                    return it->second;
                }

                BaseClient::ptr getClient(const std::string method) {
                    BaseClient::ptr client;
                    if(_enableDiscovery){
                        Address host;
                        bool ret = _discovery_client->serviceDiscovery(method,host);
                        if(ret == false) {
                            SUP_LOG_ERROR("当前{}服务没有找到服务提供者！",method);
                            return BaseClient::ptr();
                        }
                        client = getClient(host);
                        if(client.get() == nullptr) {
                            client = newClient(host);
                        }
                    }else {
                        client = _rpc_client;
                    }
                    return client;
                }

                void putClient(const Address&host,BaseClient::ptr &client) {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _rpc_clients.insert(std::make_pair(host,client));
                }

                void delClient(const Address&host) {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _rpc_clients.erase(host);
                }

            private:
            struct AddressHash {
                size_t operator()(const Address &host) const {
                    std::string addr = host.first + std::to_string(host.second);
                    return std::hash<std::string> {}(addr);
                }
            };
            bool _enableDiscovery;
            DiscoveryClient::ptr _discovery_client;
            Requestor::ptr _requestor;
            RpcCaller::ptr _caller;
            Dispatcher::ptr _dispatcher;
            BaseClient::ptr _rpc_client;
            std::mutex _mutex;
            std::unordered_map<Address,BaseClient::ptr,AddressHash> _rpc_clients;
        };

        class TopicClient { 
            public:
                using ptr = std::shared_ptr<TopicClient>;
                TopicClient(const std::string&ip,int port) :
                _requestor(std::make_shared<Requestor>()),
                _dispatcher(std::make_shared<Dispatcher>()),
                _topic_manager(std::make_shared<TopicManager>())
                {
                    auto rsp_cb = std::bind(&Requestor::onResponse,_requestor.get(),
                        std::placeholders::_1,std::placeholders::_2);
                    _dispatcher->registerHandler<BaseMessage>(MType::RSP_TOPIC,rsp_cb);

                    auto msg_cb = std::bind(&TopicManager::onPublish,
                        _topic_manager.get(),std::placeholders::_1,std::placeholders::_2);

                    _dispatcher->registerHandler<TopicRequest>(MType::REQ_TOPIC,msg_cb);
                    
                    auto message_cb = std::bind(&Dispatcher::onMessage,_dispatcher.get(),
                        std::placeholders::_1,std::placeholders::_2);
                    _rpc_client = ClientFactory::create(ip,port);
                    _rpc_client->setMessageCallback(message_cb);
                    _rpc_client->connect();
                }
                
                bool create(const std::string&key) {
                    return _topic_manager->create(_rpc_client->connection(),key);
                }

                bool remove(const std::string& key){
                    return _topic_manager->remove(_rpc_client->connection(),key);
                }

                bool subscribe(const std::string&key,const TopicManager::SubCallback &cb) {
                    return _topic_manager->cancel(_rpc_client->connection(),key );
                }

                bool cancel(const  std::string&key) {
                    return _topic_manager->cancel(_rpc_client->connection(),key);
                }

                bool publish(const std::string&key,const std::string&msg) {
                    return _topic_manager->publish(_rpc_client->connection(),key,msg);
                }

                void shutdown() {
                    _rpc_client->shutdown();
                }
            private:
                Requestor::ptr _requestor;
                TopicManager::ptr _topic_manager;
                Dispatcher::ptr _dispatcher;
                BaseClient::ptr _rpc_client;
        };
    }
}