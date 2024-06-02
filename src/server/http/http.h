#pragma once

#include "httplib.h"
#include "balancer.h"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace NHttp {

static constexpr int KDefaultPort = 8080;

class THttpSlicerServer {
public:
    THttpSlicerServer(NSlicer::TBalancer* balancer);

    void Start(int port);

private:
    httplib::Server svr;
    NSlicer::TBalancer* Balancer_;
    bool Flag_ = true;
    std::shared_ptr<spdlog::logger> HttpLogger_;

    void InitializeNotifyNodes();
    void InitializeGetMapping();
    void InitializeSendMetrics();
};

int GetPort();

} // NHttp