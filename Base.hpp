#pragma once 
#include <memory>
#include <functional>
#include "DataTypes.hpp"

namespace suprpc{
    /**
     * @class BaseMessage
     * @brief 消息类基类
     */
    class BaseMessage{
        public:
        using ptr = std::shared_ptr<BaseMessage>;
        virtual ~BaseMessage(){}
        virtual void setId(const std::string& id){
            _rid = id;
        }

        virtual std::string rid(){return _rid;}
        virtual void setMType(const MType mtype){
            _mtype = mtype;
        }
        virtual MType mtype () {return _mtype;}
        virtual std::string serialize() = 0;
        virtual bool deserialize(const std::string& msg) = 0;
        virtual bool check() = 0;

        private:
        std::string _rid;
        MType _mtype;
    };

    /**
     * @class BaseBuffer
     * @brief 缓冲类基类
     */
    class BaseBuffer{
        public:
        using ptr = std::shared_ptr<BaseBuffer>;
        virtual size_t readableSize() = 0;
        virtual int32_t peekInt32() = 0;
        virtual void retrieveInt32() = 0;
        virtual int32_t readInt32() = 0;
        virtual std::string retrieveAsString(size_t len) = 0;
    };

    /**
     * @class BaseProtocol
     * @brief 协议类基类
     */
    class BaseProtocol{
        public:
        using ptr = std::shared_ptr<BaseProtocol>;
        virtual bool canProcessed(const BaseBuffer::ptr &buf) = 0;
        virtual bool onMessage(const BaseBuffer::ptr &buf,
            const BaseMessage::ptr &msg) = 0;
        virtual std::string serialize(const BaseMessage::ptr &msg) = 0;
    };

    /**
     * @class BaseConnection
     * @brief 连接类基类
     */
    class BaseConnection
    {
        public:
        using ptr = std::shared_ptr<BaseConnection>;
        virtual void send(const BaseMessage::ptr &msg) = 0;
        virtual void shutdown() = 0;
        virtual void connected() = 0;
    };

    using ConnectionCallback = std::function<void(const BaseConnection::ptr&)>;
    using CloseCallback = std::function<void (const BaseConnection::ptr&)>;
    using MessageCallback = std::function<void (const BaseConnection::ptr&,
            BaseMessage::ptr&)>;
    
    /**
     * @class BaseServer
     * @brief 服务器类基类
     */
    class BaseServer
    {
        public:
        using ptr = std::shared_ptr<BaseServer>;
        virtual void start() = 0;
        virtual void setConnectionCallback(const ConnectionCallback& cb){
            _cb_connection = cb;
        }

        virtual void setCloseCallback(const CloseCallback& cb){
            _cb_close = cb;
        }

        virtual void setMessageCallback(const MessageCallback& cb){
            _cb_message = cb;
        }

        protected:
            ConnectionCallback _cb_connection;
            CloseCallback _cb_close;
            MessageCallback _cb_message;
    };

    /**
     * @class BaseClient
     * @brief 客户端类基类
     */
    class BaseClient
    {
        public:
        using ptr = std::shared_ptr<BaseClient>;
                virtual void setConnectionCallback(const ConnectionCallback& cb){
            _cb_connection = cb;
        }

        virtual void setCloseCallback(const CloseCallback& cb){
            _cb_close = cb;
        }

        virtual void setMessageCallback(const MessageCallback& cb){
            _cb_message = cb;
        }

        virtual void connect() = 0;
        virtual void shutdown() = 0;
        virtual bool send(const BaseMessage::ptr&) = 0;
        virtual BaseClient::ptr connection() = 0;
        virtual bool connected() = 0;

        protected:
            ConnectionCallback _cb_connection;
            CloseCallback _cb_close;
            MessageCallback _cb_message;
    };
    
}