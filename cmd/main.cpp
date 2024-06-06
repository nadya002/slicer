#include <cstdlib>
#include <string>

#include <node.hpp>
#include "spdlog/spdlog.h"

int loadIntFromEnv(std::string name) {
    const char* data = std::getenv(name.c_str());
    if (data == nullptr) {
        spdlog::error("Env variable $" + name + " should be set");
        exit(1);
    }
    return std::atoi(data);
}

std::string loadStringFromEnv(std::string name) {
    const char* data = std::getenv(name.c_str());
    if (data == nullptr) {
        spdlog::error("Env variable $" + name + " should be set");
        exit(1);
    }
    return std::string(data);
}

balancer::NodeConfig loadConfig() {
    balancer::NodeConfig cfg;
    cfg.http_port = loadIntFromEnv("HTTP_PORT");
    cfg.replication_config.port = loadIntFromEnv("REPLICATION_PORT");
    cfg.replication_config.node_id = loadStringFromEnv("NODE_ID");
    cfg.replication_config.data_dir = loadStringFromEnv("DATA_DIR");
    cfg.replication_config.hosts = loadStringFromEnv("HOSTS");
    cfg.replication_config.host = loadStringFromEnv("REPLICATION_HOST");
    return cfg;
}

int main() {
    spdlog::set_level(spdlog::level::debug);
    auto cfg = loadConfig();
    balancer::Node node(cfg);
    if (node.start() != 0) {
        spdlog::error("Balance node failed during execttion. Exiting...");
        return 1;
    }
    return 0;
}