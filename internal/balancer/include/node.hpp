#pragma once
#include <string>

namespace balancer {

struct ReplicationConfig {
    int port;
    std::string host;
    std::string data_dir;
    std::string node_id;
    std::string hosts;
};

struct NodeConfig {
    int http_port;
    ReplicationConfig replication_config;
};

class NodeImpl;

class Node {
  private:
    NodeImpl *_impl;

  public:
    Node(const NodeConfig &config);
    Node(const Node &) = delete;
    Node(Node &&);

    int start();

    ~Node();
};
}