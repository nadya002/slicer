#include "replicator.hpp"
#include "balancer.hpp"

#include <braft/raft.h>
#include <braft/util.h>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

namespace balancer {

int Replicator::start() {

    butil::EndPoint addr;
    if (butil::str2endpoint(_config.host.c_str(), _config.port, &addr) != 0) {
        spdlog::error("Failed to parse host and port");
        return -1;
    }

    braft::NodeOptions node_options;

    if (node_options.initial_conf.parse_from(_config.hosts) != 0) {
        spdlog::error("Failed to parse replicator hosts configuration");
        return -1;
    }

    node_options.election_timeout_ms = 5000;
    node_options.fsm = this;
    node_options.node_owns_fsm = false;
    node_options.disable_cli = false;
    node_options.disable_cli = false;

    std::string prefix = "local://" + _config.data_dir + "/" + _config.node_id;
    node_options.log_uri = prefix + "/log";
    node_options.raft_meta_uri = prefix + "/raft_meta";
    node_options.snapshot_uri = prefix + "/snapshot";

    braft::Node *node = new braft::Node(_group_id, braft::PeerId(addr));
    if (node->init(node_options) != 0) {
        spdlog::error("Failed to init replicator");
        delete node;
        return -1;
    }

    _node = node;

    spdlog::info("Replicator successfully configured and started");

    return 0;
}

void Replicator::on_apply(braft::Iterator &iter) {
    for (; iter.valid(); iter.next()) {
        spdlog::info("Received new event for replication");
        auto data = iter.data();
        auto jsonObject = nlohmann::json::parse(data.to_string());
        BalancerDiff diff;
        for (auto host : jsonObject["RangeNodePairs"]) {
            TDiffs tdiff;
            tdiff.NodeId = host["NodeId"];
            for (auto range : host["Ranges"]) {
                TRange rng;
                rng.Start = range["From"];
                rng.End = range["To"];
                tdiff.Ranges.push_back(rng);
            }
            diff.push_back(tdiff);
        }
        _snapshot.Apply(diff);
        
        if (iter.done()) {
            iter.done()->Run();
        }
    }
}

void Replicator::stop() {
    if (_node != nullptr) {
        _node->shutdown(NULL);
    }
    spdlog::info("Replicator went down");
}

void Replicator::UpdateMetrics(const std::vector<TMetric> &metrics) {
    if (is_leader()) {
        _balancer->UpdateMetrics(metrics);
    }
}

void Replicator::NotifyNodes(
    const std::vector<std::string> &newNodeIds,
    const std::vector<std::string> &deletedNodeIds) {
    if (is_leader()) {
        _balancer->NotifyNodes(newNodeIds, deletedNodeIds);
    }
}

BalancerDiff Replicator::GetMappingRangesToNodes() {
    if (is_leader()) {
        return _balancer->GetMappingRangesToNodes();
    }
    return _snapshot.GetMapping();
}

Replicator::~Replicator() {
    if (_node != nullptr) {
        delete _node;
    }
}

void Replicator::on_leader_start(int64_t term) {
    spdlog::info("node became leader");
    _leader_term.store(term, butil::memory_order_release);
    _balancer.reset(new TBalancer(_snapshot.GetMapping(), [&](const auto &diff, const auto &callback) {
        spdlog::info("Generated new event for replication");
        const int64_t term = _leader_term.load(butil::memory_order_relaxed);
        butil::IOBuf log;

        nlohmann::json jsonObject;
        jsonObject["RangeNodePairs"] = nlohmann::json::array();
        for (auto &val : diff) {
            nlohmann::json tdiff;
            tdiff["NodeId"] = val.NodeId;
            tdiff["Ranges"] =  nlohmann::json::array();
            for (auto &range : val.Ranges) {
                nlohmann::json trange;
                trange["From"] = range.Start;
                trange["To"] = range.End;
                tdiff["Ranges"].push_back(trange);
            }
            jsonObject["RangeNodePairs"].push_back(tdiff);
        }
        log.append(jsonObject.dump());

        braft::SynchronizedClosure synchronizedClosure;
        braft::Task task;
        task.data = &log;
        task.expected_term = term;
        task.done = &synchronizedClosure;
        _node->apply(task);
        synchronizedClosure.wait();
        callback(true);
    }));
}

void Replicator::on_leader_stop(const butil::Status &status) {
    _leader_term.store(-1, butil::memory_order_release);
    spdlog::info("node is not leader yet");
    _balancer.release();
}
}