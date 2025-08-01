/**
 * @file MuduoTool.hpp
 * @brief Muduo的二次封装
 */

#pragma once
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/TcpClient.h>

#include "DataTypes.hpp"
#include "JsonConcrete.hpp"
#include "logger.hpp"
#include "Message.hpp"

#include <mutex>
#include <unordered_map>

namespace suprpc
{
    /**
     * @class MuduoBuffer
     * @brief 缓冲的Muduo实现
     */
    class MuduoBuffer:public BaseBuffer
    {
        public:
        using ptr = std::shared_ptr<MuduoBuffer>;
        MuduoBuffer(muduo::net::Buffer*buf):_buf(buf){}

        virtual size_t readableSize() override{
            return _buf->readableBytes();
        }

        virtual int32_t peekInt32()override{
            return _buf->peekInt32();
        }

        virtual void retrieveInt32()override{
            _buf->retrieveInt32();
        }

        virtual int32_t readInt32()override{
            return _buf->readInt32();
        }

        virtual std::string retrieveAsString(size_t len)override{
            return _buf->retrieveAsString(len);
        }

        private:
            muduo::net::Buffer* _buf;
    };

    /**
     * @class BufferFactory
     * @brief Buffer工厂
     */
    class BufferFactory{
        public:
        template<typename ...Args>
        static BaseBuffer::ptr create(Args ...args){
            return std::make_shared<MuduoBuffer>(std::forward<Args>(args)...);
        }
    };

    /**
     * @class LVProtocol
     * @brief Lenth-Value协议类，首部字段表示负载的长度
     * @details | Length | Value |
     *          | Lentgh | mtype | idlen | id    | body  |
     */
    class LVProtocol:public BaseProtocol
    {
        public:
        using ptr = std::shared_ptr<LVProtocol>;
        
        virtual bool canProcessed(const BaseBuffer::ptr& buf) override{
            if(buf->readableSize() < lenFieldsLength)
            {
                return false;
            }
            int32_t total_len = buf->peekInt32();
            //检查总长度是否正确
            if(buf->readableSize() < (total_len + lenFieldsLength))
            {
                return false;
            }
            return true;
        }

        virtual bool onMessage(const BaseBuffer::ptr&buf,
            BaseMessage::ptr&msg)override{
                //当调用onMessage的时候，默认认为缓冲区的数据足够一条完整的消息
                int32_t total_len = buf->readInt32();
                MType mtype = (MType) buf->readInt32();
                int32_t idlen = buf->readInt32();
                int32_t body_len = total_len - idlen - idlenFieldLength - mtypeFieldLength;
                std::string id = buf->retrieveAsString(idlen);
                std::string body = buf->retrieveAsString(body_len);
                msg = MessageFactory::create(mtype);
                if(msg.get() == nullptr){
                    SUP_LOG_ERROR("消息类型错误，构建消息对象失败");
                    return false;
                }
                bool ret = msg->deserialize(body);
                if(ret == false){
                    SUP_LOG_ERROR("消息正文反序列化失败！");
                    return false;
                }
                msg->setId(id);
                msg->setMType(mtype);
                return true;
            }

            virtual std::string serialize(const BaseMessage::ptr &msg)override{
                std::string body = msg->serialize();
                std::string id = msg->rid();
                auto mtype = htonl((int32_t)msg->mtype());
                int32_t idlen = htonl(id.size());
                int32_t h_total_len = mtypeFieldLength + idlenFieldLength + id.size()+body.size();
                int32_t n_total_len = htonl(h_total_len);
                std::string result;
                result.reserve(h_total_len);
                result.append((char*)&n_total_len,lenFieldsLength);
                result.append((char*)&mtype,mtypeFieldLength);
                result.append((char*)&idlen,idlenFieldLength);
                result.append(id);
                result.append(body);
                return result;
            }

        private:
            const size_t lenFieldsLength = 4;
            const size_t mtypeFieldLength = 4;
            const size_t idlenFieldLength = 4;
    };

    /**
     * @class ProtocolFactory
     * @brief 生产协议类对象的工厂
     */
    class ProtocolFactory
    {
    public:
        template <typename... Args>
        static BaseProtocol::ptr create(Args &&...args)
        {
            return std::make_shared<LVProtocol>(std::forward<Args>(args)...);
        }
    };

    /**
     * @class MuduoConnection
     * @brief Muduo连接对象的封装
     */
    class MuduoConnection:public BaseConnection{
        public:
        using ptr = std::shared_ptr<MuduoConnection>;
        MuduoConnection(const muduo::net::TcpConnectionPtr &conn,
            const BaseProtocol::ptr &protocol):
            _conn(conn),_protocol(protocol){}

        virtual void send(const BaseMessage::ptr &msg)override{
            std::string body = _protocol->serialize(msg);
            _conn->send(body);
        }

        virtual void shutdown()override{
            _conn->shutdown();
        }

        virtual bool connected()override{
            return _conn->connected();
        }

        private:
        BaseProtocol::ptr _protocol;
        muduo::net::TcpConnectionPtr _conn;
    };

    /**
     * @class ConnectionFactory
     * @brief 创建连接对象的工厂
     */
    class ConnectionFactory{
        public:
            template<typename ...Args>
            static BaseConnection::ptr create(Args&& ...args){
                return std::make_shared<MuduoConnection>(std::forward<Args>(args)...);
            }
    };

    /**
     * @class MuduoServer
     * @brief Muduo服务器类的封装
     */
    class MuduoServer:public BaseServer{
        public:
        using ptr = std::shared_ptr<MuduoServer>;
        MuduoServer(uint16_t port):
        _server(&_baseloop,muduo::net::InetAddress("0.0.0.0",port),
                "MuduoServer",
                muduo::net::TcpServer::kReusePort),
        _protocol(ProtocolFactory::create())
        {}
        
        virtual void start()override{
            
        }

        void func(){}
        private:
        const size_t max_data_size = (1<<16);
        BaseProtocol::ptr _protocol;
        muduo::net::EventLoop _baseloop;
        muduo::net::TcpServer _server;  
        std::mutex _mutex;
        std::unordered_map<muduo::net::TcpConnectionPtr,BaseConnection::ptr> _conns;
    }

}
