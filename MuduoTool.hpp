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
    class MuduoBuffer : public BaseBuffer
    {
    public:
        using ptr = std::shared_ptr<MuduoBuffer>;
        MuduoBuffer(muduo::net::Buffer *buf) : _buf(buf) {}

        virtual size_t readableSize() override
        {
            return _buf->readableBytes();
        }

        virtual int32_t peekInt32() override
        {
            return _buf->peekInt32();
        }

        virtual void retrieveInt32() override
        {
            _buf->retrieveInt32();
        }

        virtual int32_t readInt32() override
        {
            return _buf->readInt32();
        }

        virtual std::string retrieveAsString(size_t len) override
        {
            return _buf->retrieveAsString(len);
        }

    private:
        muduo::net::Buffer *_buf;
    };

    /**
     * @class BufferFactory
     * @brief Buffer工厂
     */
    class BufferFactory
    {
    public:
        template <typename... Args>
        static BaseBuffer::ptr create(Args... args)
        {
            return std::make_shared<MuduoBuffer>(std::forward<Args>(args)...);
        }
    };

    /**
     * @class LVProtocol
     * @brief Lenth-Value协议类，首部字段表示负载的长度
     * @details | Length | Value |
     *          | Lentgh | mtype | idlen | id    | body  |
     */
    class LVProtocol : public BaseProtocol
    {
    public:
        using ptr = std::shared_ptr<LVProtocol>;

        virtual bool canProcessed(const BaseBuffer::ptr &buf) override
        {
            if (buf->readableSize() < lenFieldsLength)
            {
                return false;
            }
            int32_t total_len = buf->peekInt32();
            // 检查总长度是否正确
            if (buf->readableSize() < (total_len + lenFieldsLength))
            {
                return false;
            }
            return true;
        }

        virtual bool onMessage(const BaseBuffer::ptr &buf,
                               BaseMessage::ptr &msg) override
        {
            // 当调用onMessage的时候，默认认为缓冲区的数据足够一条完整的消息
            int32_t total_len = buf->readInt32();
            MType mtype = (MType)buf->readInt32();
            int32_t idlen = buf->readInt32();
            int32_t body_len = total_len - idlen - idlenFieldLength - mtypeFieldLength;
            std::string id = buf->retrieveAsString(idlen);
            std::string body = buf->retrieveAsString(body_len);
            msg = MessageFactory::create(mtype);
            if (msg.get() == nullptr)
            {
                SUP_LOG_ERROR("消息类型错误，构建消息对象失败");
                return false;
            }
            bool ret = msg->deserialize(body);
            if (ret == false)
            {
                SUP_LOG_ERROR("消息正文反序列化失败！");
                return false;
            }
            msg->setId(id);
            msg->setMType(mtype);
            return true;
        }

        virtual std::string serialize(const BaseMessage::ptr &msg) override
        {
            std::string body = msg->serialize();
            std::string id = msg->rid();
            auto mtype = htonl((int32_t)msg->mtype());
            int32_t idlen = htonl(id.size());
            int32_t h_total_len = mtypeFieldLength + idlenFieldLength + id.size() + body.size();
            int32_t n_total_len = htonl(h_total_len);
            std::string result;
            result.reserve(h_total_len);
            result.append((char *)&n_total_len, lenFieldsLength);
            result.append((char *)&mtype, mtypeFieldLength);
            result.append((char *)&idlen, idlenFieldLength);
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
    class MuduoConnection : public BaseConnection
    {
    public:
        using ptr = std::shared_ptr<MuduoConnection>;
        MuduoConnection(const muduo::net::TcpConnectionPtr &conn,
                        const BaseProtocol::ptr &protocol) : _conn(conn), _protocol(protocol) {}

        virtual void send(const BaseMessage::ptr &msg) override
        {
            std::string body = _protocol->serialize(msg);
            _conn->send(body);
        }

        virtual void shutdown() override
        {
            _conn->shutdown();
        }

        virtual bool connected() override
        {
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
    class ConnectionFactory
    {
    public:
        template <typename... Args>
        static BaseConnection::ptr create(Args &&...args)
        {
            return std::make_shared<MuduoConnection>(std::forward<Args>(args)...);
        }
    };

    /**
     * @class MuduoServer
     * @brief Muduo服务器类的封装
     */
    class MuduoServer : public BaseServer
    {
    public:
        using ptr = std::shared_ptr<MuduoServer>;
        MuduoServer(uint16_t port) : _server(&_baseloop, muduo::net::InetAddress("0.0.0.0", port),
                                             "MuduoServer",
                                             muduo::net::TcpServer::kReusePort),
                                     _protocol(ProtocolFactory::create())
        {
        }

        virtual void start() override
        {
            _server.setConnectionCallback(std::bind(&MuduoServer::onConnection, this, std::placeholders::_1));
            _server.setMessageCallback(std::bind(&MuduoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            _server.start();  // 先开始监听
            _baseloop.loop(); // 开启死循环事件监控
        }

    private:
        void onConnection(const muduo::net::TcpConnectionPtr &conn)
        {
            if (conn->connected())
            {
                SUP_LOG_INFO("连接建立");
                auto muduo_conn = ConnectionFactory::create(conn, _protocol);
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _conns.insert(std::make_pair(conn, muduo_conn));
                }
                if (_cb_connection)
                    _cb_connection(muduo_conn);
            }
            else
            {
                SUP_LOG_INFO("连接断开");
                BaseConnection::ptr muduo_conn;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _conns.find(conn);
                    if (it == _conns.end())
                    {
                        return;
                    }
                    muduo_conn = it->second;
                    _conns.erase(conn);
                }
                if (_cb_close)
                    _cb_close(muduo_conn);
            }
        }

        void onMessage(const muduo::net::TcpConnectionPtr &conn,
                       muduo::net::Buffer *buf,
                       muduo::Timestamp)
        {
            SUP_LOG_DEBUG("开始处理数据");
            auto base_buf = BufferFactory::create(buf);
            while (1)
            {
                if (_protocol->canProcessed(base_buf) == false)
                {
                    if (base_buf->readableSize() > max_data_size)
                    {
                        conn->shutdown();
                        SUP_LOG_ERROR("缓冲区中数据过大");
                        return;
                    }

                    break;
                }
                BaseMessage::ptr msg;
                bool ret = _protocol->onMessage(base_buf, msg);
                if (ret == false)
                {
                    conn->shutdown();
                    SUP_LOG_ERROR("缓冲区中数据错误");
                    return;
                }
                BaseConnection::ptr base_conn;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _conns.find(conn);
                    if (it == _conns.end())
                    {
                        conn->shutdown();
                        return;
                    }
                    base_conn = it->second;
                }
                if (_cb_message)
                    _cb_message(base_conn, msg);
            }
        }

        void func() {}

    private:
        const size_t max_data_size = (1 << 16);
        BaseProtocol::ptr _protocol;
        muduo::net::EventLoop _baseloop;
        muduo::net::TcpServer _server;
        std::mutex _mutex;
        std::unordered_map<muduo::net::TcpConnectionPtr, BaseConnection::ptr> _conns;
    };

    /**
     * @class ServerFactory
     * @brief 创建Server对象的工厂类
     */
    class ServerFactory
    {
    public:
        template <typename... Args>
        static BaseServer::ptr create(Args &&...args)
        {
            return std::make_shared<MuduoServer>(std::forward<Args>(args)...);
        }
    };

    /**
     * @class MuduoClient
     * @brief Muduo客户端类的封装
     */
    class MuduoClient : public BaseClient
    {
    public:
        using ptr = std::shared_ptr<MuduoClient>;
        MuduoClient(const std::string &svr_ip, int svr_port)
            : _protocol(ProtocolFactory::create()),
              _baseloop(_loopthread.startLoop()),
              _downlatch(1),
              _client(_baseloop, muduo::net::InetAddress(svr_ip, svr_port), "MuduoClient")
        {
        }

        virtual void connect() override
        {
            SUP_LOG_DEBUG("设置回调函数，连接服务器");

            _client.setConnectionCallback(std::bind(&MuduoClient::onConnection, this,
                                                    std::placeholders::_1));
            _client.setMessageCallback(std::bind(&MuduoClient::onMessage, this,
                                                 std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            _client.connect();
            _downlatch.wait();
            SUP_LOG_INFO("服务器连接成功");
        }

        virtual void shutdown() override
        {
            return _client.disconnect();
        }

        virtual bool send(const BaseMessage::ptr &msg) override
        {
            if (connected() == false)
            {
                SUP_LOG_ERROR("连接已断开！");
                return false;
            }
            _conn->send(msg);
        }

        virtual BaseConnection::ptr connection() override
        {
            return _conn;
        }

        virtual bool connected()
        {
            return (_conn && _conn->connected());
        }

    private:
        void onConnection(const muduo::net::TcpConnectionPtr &conn)
        {
            if (conn->connected())
            {
                SUP_LOG_TRACE("建立连接！");
                _downlatch.countDown();
                _conn = ConnectionFactory::create(conn, _protocol);
            }
            else
            {
                SUP_LOG_TRACE("连接断开！");
                _conn.reset();
            }
        }

        void onMessage(const muduo::net::TcpConnectionPtr &conn,
                       muduo::net::Buffer *buf, muduo::Timestamp)
        {
            SUP_LOG_DEBUG("有数据到来，开始处理！");
            auto base_buf = BufferFactory::create(buf);
            while (1)
            {
                if (_protocol->canProcessed(base_buf) == false)
                {
                    if (base_buf->readableSize() > max_data_size)
                    {
                        conn->shutdown();
                        SUP_LOG_ERROR("缓冲区数据过大！");
                        return;
                    }
                    break;
                }
                BaseMessage::ptr msg;
                bool ret = _protocol->onMessage(base_buf, msg);
                if (ret == false)
                {
                    conn->shutdown();
                    SUP_LOG_ERROR("缓冲区中数据错误！");
                    return;
                }
                if (_cb_message)
                    _cb_message(_conn, msg);
            }
        }

    private:
        const size_t max_data_size = (1 << 16);
        BaseProtocol::ptr _protocol;
        BaseConnection::ptr _conn;
        muduo::CountDownLatch _downlatch;
        muduo::net::EventLoopThread _loopthread;
        muduo::net::EventLoop *_baseloop;
        muduo::net::TcpClient _client;
    };

    /**
     * @class ClientFactory
     * @brief 生成客户端类对象的工厂类
     */
    class ClientFactory
    {
    public:
        template <typename... Args>
        static BaseClient::ptr create(Args &&...args)
        {
            return std::make_shared<MuduoClient>(std::forward<Args>(args)...);
        }
    };
}
