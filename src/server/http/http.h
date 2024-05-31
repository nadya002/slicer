#pragma once

#include "httplib.h"
#include "balancer.h"

namespace NHttp {

static constexpr int KDefaultPort = 8080;

class THttpSlicerServer {
public:
    THttpSlicerServer(NSlicer::Balancer* balancer);

    void Start(int port);

private:
    httplib::Server svr;
    NSlicer::Balancer* Balancer_;
    bool Flag_ = true;
    std::shared_ptr<spdlog::logger> HttpLogger_;

    void InitializeNotifyNodes();
    void InitializeGetMapping();
    void InitializeSendMetrics();
};

int GetPort();

} // NHttp