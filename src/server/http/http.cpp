#include "http.h"

#include "nlohmann/json.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace NHttp {

THttpSlicerServer::THttpSlicerServer(NSlicer::Balancer* balancer)
    : Balancer_(balancer)
{
    HttpLogger_ = spdlog::basic_logger_mt("http_logger", "logger/http_logger.txt");

    svr.Get("/", [](const httplib::Request &req, httplib::Response &res) {
        res.set_content("Hello, World!", "text/plain");
    });

    InitializeNotifyNodes();
    InitializeGetMapping();
    InitializeSendMetrics();
}

void THttpSlicerServer::Start(int port)
{
    HttpLogger_->info("Server listening on http://localhost:" + std::to_string(port));
    spdlog::info("Server listening on http://localhost:" + std::to_string(port));

    svr.listen("localhost", port);
}

void THttpSlicerServer::InitializeNotifyNodes()
{
    svr.Post("/api/v1/notify_nodes", [&](const httplib::Request &req, httplib::Response &res) {
        std::cerr << "notify_nodes" << std::endl;
        std::vector<std::string> NodeIds_;
        auto body = req.body;
        auto jsonObject = nlohmann::json::parse(body);

        auto newHosts = jsonObject["New"];
        auto hosts = newHosts["Hosts"];

        for (auto host : hosts) {
            NodeIds_.push_back(host["Host"]);
        }
        if (Flag_) {
            Balancer_->Initialize(NodeIds_);
        }
    });
}

void THttpSlicerServer::InitializeGetMapping()
{
    svr.Get("/api/v1/get_mapping", [&](const httplib::Request &req, httplib::Response &res) {
        nlohmann::json jsonObject;
        std::vector<NSlicer::TRangesToNode> rangesToNode = Balancer_->GetMappingRangesToNodes();
        jsonObject["RangeNodePairs"] = nlohmann::json::array();
        for (auto& value : rangesToNode) {
            for (auto& range : value.Ranges) {
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
}

void THttpSlicerServer::InitializeSendMetrics()
{
    svr.Post("/api/v1/send_metric", [&](const httplib::Request &req, httplib::Response &res) {

        std::vector<NSlicer::TMetric> metrics;

        auto body = req.body;
        auto jsonObject = nlohmann::json::parse(body);

        auto jsonMetrics = jsonObject["Metrics"];

        for (auto& metric : jsonMetrics) {
            metrics.push_back(NSlicer::TMetric{
                .Range = NSlicer::TRange{
                    .Start = metric["Range"]["From"],
                    .End = metric["Range"]["To"]
                },
                .Value_ = metric["Value"]
            });
        }
        Balancer_->UpdateMetrics(metrics);
    });
}

int GetPort()
{
    return KDefaultPort;
}

} // NHttp