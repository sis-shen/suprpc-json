#include "../../common/JsonConcrete.hpp"
#include "../../server/Server.hpp"

void Add(const Json::Value&req,Json::Value&rsp) {
    SUP_LOG_DEBUG("触发回调,参数 {} ,{}",req["num1"].asInt(),req["num2"].asInt());
    int num1 = req["num1"].asInt();
    int num2 = req["num2"].asInt();
    rsp = num1 + num2;
}

int main() {
    suprpc::init_logger(false,"",spdlog::level::level_enum::trace);
    std::unique_ptr<suprpc::server::SvrDescbFactory> desc_factory(
        new suprpc::server::SvrDescbFactory()
    );

    desc_factory->setMethodNmae("Add");
    desc_factory->setParamsDesc("num1",suprpc::server::VType::INTEGRAL);
    desc_factory->setParamsDesc("num2",suprpc::server::VType::INTEGRAL);
    desc_factory->setReturnType(suprpc::server::VType::INTEGRAL);
    desc_factory->setCallback(Add);
    suprpc::server::RpcServer server(suprpc::Address("127.0.0.1",9090));
    server.registerMethod(desc_factory->build());
    server.start();
    return 0;
}