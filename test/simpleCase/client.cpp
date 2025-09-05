#include "../../common/JsonConcrete.hpp"
#include "../../client/RpcCaller.hpp"
#include "../../client/Client.hpp"

void callback(const Json::Value &result) {
    SUP_LOG_INFO("callback result: {}",result.asInt());
}

int main() {
    printf("666\n");
    suprpc::init_logger(false,"",spdlog::level::level_enum::trace);
    // SUP_LOG_TRACE("客户端main函数开始");
    suprpc::client::RpcClient client(false,"127.0.0.1",9090);
    // SUP_LOG_INFO("客户端声明成功");
    Json::Value params,result;
    params["num1"] = 33;
    params["num2"] = 44;
    bool ret = client.call("Add",params,result);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if(ret == true) {
        SUP_LOG_DEBUG("result is {}",result.asInt());
    }

    suprpc::client::RpcCaller::JsonAsyncResponse res_future;
    params["num1"] = 99;
    params["num2"] = 1;
    ret = client.call("Add",params,res_future);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if(ret == true) {
        result = res_future.get();
        SUP_LOG_INFO("result is {}",result.asInt());
    }

    params["num1"] = 55;
    params["num2"] = 44;
    // ret = client.call("Add",params,(suprpc::client::RpcCaller::JsonResponseCallback)(callback));
    SUP_LOG_DEBUG("-------------client done ------------------");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}