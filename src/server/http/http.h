#pragma once

#include "httplib.h"
#include "balancer.h"

namespace NSlicer {

class THttpSlicerServer {
public:
    THttpSlicerServer(NSlicer::Balancer* balancer);

    void Start();

private:
    httplib::Server svr;
    NSlicer::Balancer* Balancer_;
    bool Flag_ = true;

    void InitializeNotifyNodes();
    void InitializeGetMapping();
    void InitializeSendMetrics();
};

} // NSlicer