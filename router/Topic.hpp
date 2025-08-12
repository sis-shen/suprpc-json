/**
 * @file Topic.hpp
 * @brief 主题的发布和订阅的实现
 */

#pragma once
#include "../common/MuduoTool.hpp"
#include "../common/Message.hpp"

#include <unordered_map>
#include <unordered_set>

namespace suprpc{
    namespace server{
        /**
         * @class TopicManager
         * @brief 话题管理类
         */
        class TopicManager{
            public:
            using ptr = std::shared_ptr<TopicManager>;
            TopicManager(){}
            void onTopicRequest(const BaseConnection::ptr &conn,
                const TopicRequest::ptr& msg){
                    TopicOptype topic_optype = msg->optype();
                    bool ret = true;
                    switch (topic_optype)
                    {
                    case TopicOptype::TOPIC_CRAETE :
                        /* code */
                        break;
                    
                    default:
                        break;
                    }
                }

        
        };
}
}
