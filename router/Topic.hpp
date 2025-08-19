/**
 * @file Topic.hpp
 * @brief 主题的发布和订阅的实现
 */

#pragma once
#include "../common/MuduoTool.hpp"
#include "../common/Message.hpp"

#include <unordered_map>
#include <unordered_set>

namespace suprpc
{
    namespace server
    {
        /**
         * @class TopicManager
         * @brief 话题管理类
         */
        class TopicManager
        {
        public:
            using ptr = std::shared_ptr<TopicManager>;
            TopicManager() {}
            void onTopicRequest(const BaseConnection::ptr &conn,
                                const TopicRequest::ptr &msg)
            {
                TopicOptype topic_optype = msg->optype();
                bool ret = true;
                switch (topic_optype)
                {
                case TopicOptype::TOPIC_CRAETE:
                    /* code */
                    break;

                default:
                    break;
                }
            }

            void onShutDown(const BaseConnection::ptr &conn)
            {
                // std::vector<Topic::ptr>
            }

        private:
            void errorResponse(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg, RCode code)
            {
                auto msg_rsp = MessageFactory::create<TopicResponse>();
                msg_rsp->setId(msg->rid());
                msg_rsp->setMType(MType::RSP_TOPIC);
                msg_rsp->setRCode(code);
                conn->send(msg_rsp);
            }

            void topicResponse(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg)
            {
                auto msg_rsp = MessageFactory::create<TopicResponse>();
                msg_rsp->setId(msg->rid());
                msg_rsp->setMType(MType::RSP_TOPIC);
                msg_rsp->setRCode(RCode::RCODE_OK);
                conn->send(msg_rsp);
            }

            void topicRemove(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg)
            {
                std::string topic_name = msg->topicKey();
                std::unordered_set<Subscriber::ptr> subscribers;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _topics.find(topic_name);
                    if (it == _topics.end())
                    {
                        return;
                    }

                    subscribers = it->second->_subscribers;
                    _topics.erase(it); // 删除当前主题的映射关系
                }
                for (auto &subscriber : subscribers)
                {
                    subscriber->removeTopic(topic_name);
                }
            }

            bool topicSubscriber(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg)
            {
                Topic::ptr topic;
                Subscriber::ptr subscriber;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto topic_it = _topics.find(msg->topicKey());
                    if (topic_it == _topics.end())
                    {
                        return false;
                    }
                    topic = topic_it->second;
                    auto sub_it = _subscribers.find(conn);
                    if (sub_it != _subscribers.end())
                    {
                        subscriber = sub_it->second;
                    }
                    else
                    {
                        subscriber = std::make_shared<Subscriber>(conn);
                        _subscribers.insert(std::make_pair(conn, subscriber));
                    }
                }

                topic->appendSubscriber(subscriber);
                subscriber->appendTopic(msg->topicKey());
            }

            void topicCcancel(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg)
            {
                Topic::ptr topic;
                Subscriber::ptr subscriber;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto topic_it = _topics.find(msg->topicKey());
                    if (topic_it != _topics.end())
                    {
                        topic = topic_it->second;
                    }
                    auto sub_it = _subscribers.find(conn);
                    if (sub_it != _subscribers.end())
                    {
                        subscriber = sub_it->second;
                    }
                }
                if (subscriber)
                    subscriber->removeTopic(msg->topicKey());
                if (topic && subscriber)
                    topic->removeSubscriber(subscriber);
            }

            bool topicPublish(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg)
            {
                Topic::ptr topic;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto topic_it = _topics.find(msg->topicKey());
                    if (topic_it == _topics.end())
                    {
                        return false;
                    }
                    topic = topic_it->second;
                }
                topic->pushMessage(msg);
                return true;
            }

        private:
            /**
             * @class Subscriber
             * @brief 订阅者类
             */
            struct Subscriber
            {
                using ptr = std::shared_ptr<Subscriber>;
                std::mutex _mutex;
                BaseConnection::ptr _conn;
                std::unordered_set<std::string> _topics;

                Subscriber(const BaseConnection::ptr &conn)
                    : _conn(conn)
                {
                }
                void appendTopic(const std::string &topic_name)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _topics.insert(topic_name);
                }

                void removeTopic(const std::string &topic_name)
                {
                    std::unique_lock(topic_name);
                    _topics.erase(topic_name);
                }
            };
            /**
             * @class Topic
             * @brief 话题类
             */
            struct Topic
            {
                using ptr = std::shared_ptr<Topic>;
                std::mutex _mutex;
                std::string _topic_name;

                std::unordered_set<Subscriber::ptr> _subscribers;
                Topic(const std::string &name) : _topic_name(name) {}
                void appendSubscriber(const Subscriber::ptr &subscriber)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _subscribers.insert(subscriber);
                }
                void removeSubscriber(const Subscriber::ptr &subscriber)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _subscribers.erase(subscriber);
                }
                void pushMessage(const BaseMessage::ptr &msg)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    for (auto &subscriber : _subscribers)
                    {
                        subscriber->_conn->send(msg);
                    }
                }
            };

        private:
            std::mutex _mutex;
            std::unordered_map<std::string, Topic::ptr> _topics;
            std::unordered_map<BaseConnection::ptr, Subscriber::ptr> _subscribers;
        };
    }
}
