/**
 * @file Server.hpp
 * @brief 服务器类封装
 */

 #include "../common/Dispatcher.hpp"
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


            private:
                bool _enableRegistry;
                Address _access_addr;
                
        }
    }
 }