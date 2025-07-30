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
            }

        private:
            const size_t lenFieldsLength = 4;
            const size_t mtypeFieldLength = 4;
            const size_t idlenFieldLength = 4;
    }
}
