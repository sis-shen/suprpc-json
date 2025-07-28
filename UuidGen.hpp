/**
 * @file UuidGen.hpp
 * @author supdriver
 * @date 2025-05-26
 * @brief uuid的生成
 * @version 2.6.0
 */
#pragma once
#include <iostream>
#include <chrono>
#include <random>
#include <string>
#include <sstream>
#include <atomic>
#include <iomanip>


namespace suprpc{
/**
 * @brief 生成uuid
 */
static std::string uuid()
{
   std::stringstream ss;
   // 构造一个机器随机数对象
   std::random_device rd;
   std::mt19937 generator(rd());
   std::uniform_int_distribution<int> distribution(0, 255);
   for (int i = 0; i < 8; i++)
   {
      if (i == 4 || i == 6)
         ss << "-";
      ss << std::setw(2) << std::setfill('0') << std::hex << distribution(generator);
   }
   ss << "-";
   static std::atomic<size_t> seq(1);
   size_t cur = seq.fetch_add(1);
   for (int i = 7; i > 0; i--)
   {
      if (i == 5)
         ss << "-";
      ss << std::setw(2) << std::setfill('0') << std::hex << ((cur >> (i * 8)) & 0xFF);
   }
   return ss.str();
}
}