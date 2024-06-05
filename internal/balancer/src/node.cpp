#include "node.hpp"
#include <braft/raft.h>
#include <brpc/server.h>
#include <thread>

#include "balancer.hpp"
#include "replicator.hpp"
#include "spdlog/spdlog.h"
#include "httplib.h"
#include "nlohmann/json.hpp"

namespace balancer {

class NodeImpl {
private:
  NodeConfig _config;

public:
  NodeImpl(NodeConfig config) : _config(config) {}

  int start() {

    brpc::Server server;

    if (braft::add_service(&server, _config.replication_config.port) != 0) {
      spdlog::error("Failed to add BRPC service to Relication Handler");
      return 1;
    }

    if (server.Start(_config.replication_config.port, NULL) != 0) {
      spdlog::error("Failed to start BRPC Relication Handler");
      return 1;
    }
    spdlog::info("BRPC Relication Handler successfully started");

    Replicator replicator("main", _config.replication_config);
    if (replicator.start() != 0) {
      spdlog::error("Failed to start replicator");
      return 1;
    }

    httplib::Server http;

    http.Post("/api/v1/notify_nodes", [&](const httplib::Request &req, httplib::Response &res) {
      spdlog::debug("notify_nodes");
      std::vector<std::string> newNodeIds, deletedNodeIds;
      auto body = req.body;
      auto jsonObject = nlohmann::json::parse(body);
      auto newHosts = jsonObject["New"];
      for (auto host : newHosts["Hosts"]) {
        newNodeIds.push_back(host["Host"]);
      }
      auto deletedHosts = jsonObject["Deleted"];
      for (auto host : deletedHosts["Hosts"]) {
        deletedNodeIds.push_back(host["Host"]);
      }
      replicator.NotifyNodes(newNodeIds, deletedNodeIds);
    });

    http.Get("/api/v1/get_mapping", [&](const httplib::Request &req, httplib::Response &res) {
      spdlog::debug("get_mapping");
      nlohmann::json jsonObject;
      std::vector<TRangesToNode> rangesToNode = replicator.GetMappingRangesToNodes();
      jsonObject["RangeNodePairs"] = nlohmann::json::array();
      for (auto &value : rangesToNode) {
        for (auto &range : value.Ranges) {
          nlohmann::json addrangeToNode;
          addrangeToNode["Host"] = value.NodeId;
          addrangeToNode["Range"]["From"] = range.Start;
          addrangeToNode["Range"]["To"] = range.End;
          jsonObject["RangeNodePairs"].push_back(addrangeToNode);
        }
      }
      std::string jsonString = jsonObject.dump();
      res.set_content(jsonString, "application/json");
    });

    http.Post("/api/v1/send_metric", [&](const httplib::Request &req, httplib::Response &res) {
      std::vector<TMetric> metrics;
      auto body = req.body;
      auto jsonObject = nlohmann::json::parse(body);
      auto jsonMetrics = jsonObject["Metrics"];
      for (auto &metric : jsonMetrics) {
        metrics.push_back(TMetric{
            .Range = TRange{
                .Start = metric["Range"]["From"],
                .End = metric["Range"]["To"]},
            .Value_ = metric["Value"]});
      }
      replicator.UpdateMetrics(metrics);
    });

    std::thread httpThread([&]() {
      spdlog::info("Listenning http on 0.0.0.0:" + std::to_string(_config.http_port));
      http.listen("0.0.0.0", _config.http_port);
    });

    spdlog::info("Node is up. Waiting for connections...");

    while (!brpc::IsAskedToQuit()) {
      sleep(1);
    }

    http.stop();
    httpThread.join();
    replicator.stop();
    server.Stop(0);
    server.Join();

    return 0;
  }
};

Node::Node(const NodeConfig &config) : _impl(new NodeImpl(config)) {}

Node::Node(Node &&other) : _impl(other._impl) {
  other._impl = nullptr;
}

int Node::start() {
  return _impl->start();
}

Node::~Node() {
  if (_impl != nullptr) {
    delete _impl;
  }
}
}