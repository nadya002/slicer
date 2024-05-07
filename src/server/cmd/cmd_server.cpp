#include <iostream>
#include <vector>

#include "httplib.h"
#include "nlohmann/json.hpp"
#include "balancer.h"

int main() {
    // Создаем сервер
    httplib::Server svr;
    NSlicer::Balancer balanser;
    bool flag = true;
    // Обрабатываем GET запрос на корневой URL (/)
    svr.Get("/", [&](const httplib::Request &req, httplib::Response &res) {
        std::cerr << "Strart" << std::endl;
        nlohmann::json jsonObject;
        jsonObject["name"] = "John";
        std::string jsonString = jsonObject.dump();

        res.set_content(jsonString, "json");
    });

    svr.Post("/api/v1/notify_nodes", [&](const httplib::Request &req, httplib::Response &res) {
        std::cerr << "Strart" << std::endl;
        std::vector<std::string> NodeIds_;
        auto body = req.body;
        std::cerr << "Body " << body << std::endl;

        auto jsonObject = nlohmann::json::parse(body);

         std::cerr << "succsess" << body << std::endl;

        auto newHosts = jsonObject["New"];
        auto hosts = newHosts["Hosts"];
        std::cerr << "NodeIds " << std::endl;
        for (auto host : hosts) {
            NodeIds_.push_back(host["Host"]);
            std::cerr << "NodeId " << NodeIds_.back() << std::endl;
        }
        if (flag) {
            balanser.Initialize(NodeIds_);
        }
        //nlohmann::json jsonObject;
        //jsonObject["name"] = "John";
        //std::string jsonString = jsonObject.dump();

       // res.set_content(jsonString, "json");
    });

    svr.Get("/api/v1/get_mapping", [&](const httplib::Request &req, httplib::Response &res) {
        std::cerr << "Get mapping" << std::endl;

        nlohmann::json jsonObject;
        std::vector<NSlicer::TRangesToNode> rangesToNode = balanser.GetMappingRangesToNodes();
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

        //jsonObject["name"] = "John";
        std::string jsonString = jsonObject.dump();

        res.set_content(jsonString, "application/json");
    });

    // Обрабатываем GET запрос на URL /hello с параметром name
    // svr.Get("/hello", [](const httplib::Request &req, httplib::Response &res) {
    //     auto name = req.get_param_value("name");
    //     std::string greeting = "Hello, " + name + "!";
    //     res.set_content(greeting, "json");
    // });

    // Запускаем сервер на localhost на порту 8080
    std::cout << "Server listening on http://localhost:8080" << std::endl;
    svr.listen("localhost", 8080);

    return 0;
}
