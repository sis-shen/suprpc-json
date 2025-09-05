/**
 * @file Server.hpp
 * @brief 服务器类封装
 */
#pragma once
 #include "../common/Dispatcher.hpp"
 #include "../client/Client.hpp"
 #include "../client/ClientTopic.hpp"
 #include "./Service.hpp"
 #include "./RpcRouter.hpp"
 #include "./Topic.hpp"

 namespace suprpc{
    namespace server{
        /**
         * @class RegisterServer
         * @brief 注册服务类
         */
        class RegistryServer{
            public:
                using ptr = std::shared_ptr<RegistryServer>;
                RegistryServer(int port):
                    _pd_manager(std::make_shared<PDManager>()),
                    _dispatcher(std::make_shared<Dispatcher>())
                {
                    auto service_cb = std::bind(&PDManager::onServiceRequest,_pd_manager.get(),
                    std::placeholders::_1,std::placeholders::_2);
                    _dispatcher->registerHandler<ServiceRequest>(
                        MType::REQ_SERVICE,service_cb);

                    _server = suprpc::ServerFactory::create(port);
                    auto message_cb = std::bind(
                        &Dispatcher::onMessage,
                        _dispatcher.get(),
                        std::placeholders::_1,std::placeholders::_2
                    );
                    _server->setMessageCallback(message_cb);
                    auto close_cb = std::bind(&RegistryServer::onConnShutdown,this,
                    std::placeholders::_1);
                    _server->setCloseCallback(close_cb);

                }

                void start(){
                    _server->start();
                }
            private:
                void onConnShutdown(const BaseConnection::ptr&conn){
                    _pd_manager->onConnShutdown(conn);
                }
            private:
            PDManager::ptr _pd_manager;
            Dispatcher::ptr _dispatcher;
            BaseServer::ptr _server;
        };

        /**
         * @class RpcServer
         * @brief Rpc服务器类
         */
        class RpcServer{
            public:
            using ptr = std::shared_ptr<RpcServer>;
            RpcServer(const Address&access_addr,
                bool enableRegistry = false,
                const Address &registry_server_addr = Address()):
                _enableRegistry(enableRegistry),
                _access_addr(access_addr),
                _router(std::make_shared<suprpc::server::RpcRouter>()),
                _dispatcher(std::make_shared<suprpc::Dispatcher>())
                {
                    if(enableRegistry) {
                        _reg_client = std::make_shared<client::RegistryClient>(
                            registry_server_addr.first,
                            registry_server_addr.second);
                    }
                    auto rpc_cb = std::bind(&RpcRouter::onRpcRequest,_router.get(),
                        std::placeholders::_1,std::placeholders::_2);

                    _dispatcher->registerHandler<suprpc::RpcRequest>(
                        suprpc::MType::REQ_RPC,rpc_cb
                    );

                    _server = suprpc::ServerFactory::create(access_addr.second);
                    auto message_cb = std::bind(&Dispatcher::onMessage,_dispatcher.get(),
                    std::placeholders::_1,std::placeholders::_2);
                    _server->setMessageCallback(message_cb);
                }

                void registerMethod(const ServiceDescribe::ptr &service) {
                    if(_enableRegistry) {
                        _reg_client->registryMethod(service->method(),_access_addr);
                    }
                    _router->registerMethod(service);
                }

                void start() {
                    _server->start();
                }

            private:
                bool _enableRegistry;
                Address _access_addr;
                RpcRouter::ptr _router;
                Dispatcher::ptr _dispatcher;
                client::RegistryClient::ptr _reg_client;
                BaseServer::ptr _server;
        };
    }
 }