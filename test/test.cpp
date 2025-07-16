/**
 * @file jsonProti.hpp
 * @author supdriver
 * @date 2025-05-26
 * @brief Json序列化与反序列化
 * @version 2.6.0
 */
#include "../UuidGen.hpp"

int main()
{
    for(int i = 0;i< 10;++i)
    {
        std::cout<<uuid()<<std::endl;
    }
    return 0;
}