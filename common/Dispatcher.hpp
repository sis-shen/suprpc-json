/**
 * @file Dispatcher.hpp
 * @brief 封装分发器相关的类
 */

#pragma once

#include "MuduoTool.hpp"
#include "Message.hpp"

namespace suprpc
{
    /**
     * @class Callback
     * @brief 定义回调类基类
     */
    class Callback
    {
    public:
        using ptr = std::shared_ptr<Callback>;
        virtual void onMessage(const BaseConnection::ptr &conn,
                               BaseMessage::ptr &msg);
    };

    /**
     * @class CallbackT
     * @brief 定义回调类基类的泛型编程实现
     */
    template <typename T>
    class CallbackT : public Callback
    {
    public:
        using ptr = std::shared_ptr<CallbackT<T>>;
        using MessageCallback = std::function<void(const BaseConnection::ptr &conn,
                                                 const std::shared_ptr<T> &msg)>;
        CallbackT(const MessageCallback &handler) : _handler(handler) {}
        void onMessage(const BaseConnection::ptr &conn,
                       const BaseMessage::ptr &msg)
        {
            auto typemsg = std::dynamic_pointer_cast<T>(msg);
            _handler(conn, typemsg);
        }

    private:
        MessageCallback _handler;
    };

    /**
     * @class Dispatcher
     * @brief 分发类
     */
    class Dispatcher
    {
    public:
        using ptr = std::shared_ptr<Dispatcher>;
        template <typename T>
        void registerHandler(MType mtype,
                             const typename CallbackT<T>::MessageHandler &handler)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto cb = std::make_shared<CallbackT<T>>(handler);
            _handlers.insert(std::make_pair(mtype, cb));
        }

        void onMessage(const BaseConnection::ptr &conn, BaseMessage::ptr &msg)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _handlers.find(msg->mtype());
            if (it != _handlers.end())
            {
                return it->second->onMessage(conn, msg);
            }
            SUP_LOG_ERROR("收到未知类型消息： {}", (int)msg->mtype());
            conn->shutdown();
        }

    private:
        std::mutex _mutex;
        std::unordered_map<MType, Callback::ptr> _handlers;
    };
}