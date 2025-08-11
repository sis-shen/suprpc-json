/**
 * @file RpcRouter.hpp
 * @brief Rpc路由相关类的实现
 */

#pragma once

#include "../common/MuduoTool.hpp"
#include "../common/Message.hpp"

namespace suprpc
{
    namespace server
    {
        enum class VType
        {
            BOOL = 0,
            INTEGRAL,
            NUMERIC,
            STRING,
            ARRARY,
            OBJECT,
        };

        class ServiceDescribe
        {
        public:
            using ptr = std::shared_ptr<ServiceDescribe>;
            using ServiceCallback = std::function<void(const Json::Value &, Json::Value &)>;
            using ParamDescribe = std::pair<std::string, VType>;
            ServiceDescribe(std::string &&mthod_name,
                            std::vector<ParamDescribe> &&desc,
                            VType vtype,
                            ServiceCallback &&handler) : _method_name(mthod_name),
                                                         _callback(std::move(handler)),
                                                         _params_desc(std::move(desc)),
                                                         _return_type(vtype)
            {
            }

            const std::string &method() { return _method_name; }

            bool paramCheck(const Json::Value &params)
            {
                for (auto &desc : _params_desc)
                {
                    if (params.isMember(desc.first) == false)
                    {
                        SUP_LOG_ERROR("参数字段完整性校验失败！{}字段缺失", desc.first.c_str());
                        return false;
                    }
                    if (check(desc.second, params[desc.first] == false))
                    {
                        SUP_LOG_ERROR("{}参数类型校验失败", desc.first);
                        return false;
                    }
                }
                return true;
            }

            bool call(const Json::Value &params, Json::Value &result)
            {
                _callback(params, result);
                if (rtypeCheck(result) == false)
                {
                    SUP_LOG_ERROR("回调处理函数中的响应信息校验失败！");
                    return false;
                }
                return true;
            }

        private:
            bool rtypeCheck(const Json::Value &val)
            {
                return check(_return_type, val);
            }

            bool check(VType type, const Json::Value &val)
            {
                switch (type)
                {
                case VType::BOOL:
                    return val.isBool();
                    break;
                case VType::INTEGRAL:
                    return val.isInt();
                    break;
                case VType::NUMERIC:
                    return val.isNumeric();
                    break;
                case VType::STRING:
                    return val.isString();
                    break;
                case VType::ARRARY:
                    return val.isArray();
                    break;
                case VType::OBJECT:
                    return val.isObject();
                    break;
                default:
                    return false;
                }
            }

        private:
            std::string _method_name;                // 方法名称
            ServiceCallback _callback;               // 实际的业务回调函数
            std::vector<ParamDescribe> _params_desc; // 参数字段格式描述
            VType _return_type;                      // 结果作为返回值的描述
        };

        /**
         * @class SvrDescFactory
         * @brief 服务描述工厂
         */
        class SvrDescbFactory
        {
        public:
            void setMethodNmae(const std::string &method_name)
            {
                _method_name = method_name;
            }

            void setCallback(const ServiceDescribe::ServiceCallback &cb)
            {
                _callback = cb;
            }

            void setParamsDesc(const std::vector<ServiceDescribe::ParamDescribe> &params_desc)
            {
                _params_desc = params_desc;
            }

            ServiceDescribe::ptr build()
            {
                return std::make_shared<ServiceDescribe>(
                    std::move(_method_name),
                    std::move(_params_desc),
                    _return_type,
                    std::move(_callback));
            }

        private:
            std::string _method_name;
            ServiceDescribe::ServiceCallback _callback;
            std::vector<ServiceDescribe::ParamDescribe> _params_desc;
            VType _return_type;
        };

        /**
         * @class ServiceManager
         * @brief 管理Service类的类
         */
        class ServiceManager
        {
        public:
            using ptr = std::shared_ptr<ServiceManager>;
            void insert(const ServiceDescribe::ptr &desc)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _services.insert(std::make_pair(desc->method(), desc));
            }

            ServiceDescribe::ptr select(const std::string &method_name)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _services.find(method_name);
                if (it == _services.end())
                {
                    return ServiceDescribe::ptr();
                }
                return it->second;
            }

            void remove(const std::string &method_name)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _services.erase(method_name);
            }

        private:
            std::mutex _mutex;
            std::unordered_map<std::string, ServiceDescribe::ptr> _services;
        };

        /**
         * @class RpcRouter
         * @brief Rpc路由类封装
         */
        class RpcRouter
        {
        public:
            using ptr = std::shared_ptr<RpcRouter>;
            void onRpcRouter(const BaseConnection::ptr &conn,
                             RpcRequest::ptr &request)
            {
                auto service = _svr_manager->select(request->method());
                if (service.get() == nullptr)
                {
                    SUP_LOG_ERROR("{} 服务未找到", request->method());
                    return response(conn, request, Json::Value(), RCode::RCODE_NOT_FOUND_SERVICE);
                }

                if (service->paramCheck(request->params()) == false)
                {
                    SUP_LOG_ERROR("{} 服务器参数校验失败", request->method());
                    return response(conn, request, Json::Value(), RCode::RCODE_INVALID_PARAMS);
                }

                Json::Value result;
                bool ret = service->call(request->params(), result);
                if (ret == false)
                {
                    SUP_LOG_ERROR("{} 服务器出现内部错误", request->method());
                    return response(conn, request, Json::Value(), RCode::RCODE_INTERNAL_ERROR);
                }
                return response(conn, request, Json::Value(), RCode::RCODE_OK);
            }

            void registerMethod(const ServiceDescribe::ptr &service)
            {
                _svr_manager->insert(service);
            }

        private:
            void response(const BaseConnection::ptr &conn,
                          const RpcRequest::ptr &req,
                          const Json::Value &res,
                          RCode code)
            {
                auto msg = MessageFactory::create<RpcResponse>();
                msg->setId(req->rid());
                msg->setMType(suprpc::MType::RSP_RPC);
                msg->setRCode(code);
                msg->setResult(res);
                conn->send(msg);
            }

        private:
            ServiceManager::ptr _svr_manager;
        };
    }
}