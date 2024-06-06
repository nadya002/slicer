#pragma once

#include <braft/raft.h>
#include <braft/util.h>
#include <node.hpp>
#include "balancer.hpp"

namespace balancer {

class Replicator : public braft::StateMachine {
private:
  TBalancerSnapshot _snapshot;
  std::string _group_id;
  braft::Node *_node;
  ReplicationConfig _config;
  butil::atomic<int64_t> _leader_term;
  std::unique_ptr<TBalancer> _balancer;

  bool is_leader() const { return _leader_term.load(butil::memory_order_acquire) > 0; }

public:
  Replicator(std::string group_id, ReplicationConfig config) : _config(config), _node(nullptr), _group_id(group_id) {}
  Replicator(const Replicator &) = delete;
  Replicator(Replicator &&) = delete;
  virtual void on_apply(::braft::Iterator &);
  int start();
  void stop();
  void on_leader_start(int64_t);
  void on_leader_stop(const butil::Status &);

  void UpdateMetrics(const std::vector<TMetric> &metrics);
  BalancerDiff GetMappingRangesToNodes();
  void NotifyNodes(
      const std::vector<std::string> &newNodeIds,
      const std::vector<std::string> &deletedNodeIds);

  ~Replicator();
};
}