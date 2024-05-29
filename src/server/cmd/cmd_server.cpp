#include <iostream>
#include <vector>
#include <thread>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "http.h"

// void RebalanceThread()
// {
//     // wait until main() sends data
//     //std::unique_lock lk(m);
//     //cv.wait(lk, []{ return ready; });

//     // after the wait, we own the lock
//     std::cout << "Worker thread is processing data\n";
//     data += " after processing";

//     // send data back to main()
//     processed = true;
//     std::cout << "Worker thread signals data processing completed\n";

//     // manual unlocking is done before notifying, to avoid waking up
//     // the waiting thread only to block again (see notify_one for details)
//     //lk.unlock();
//     //cv.notify_one();
// }

int main() {
    auto main_logger = spdlog::basic_logger_mt("main_logger", "main_logs.txt");
    // Устанавливаем глобальный уровень логгированния
    spdlog::set_level(spdlog::level::info); // Set global log level to debug
    //file_logger->set_level(spdlog::level::info);
    main_logger->flush_on(spdlog::level::info);

    //spdlog::register_logger(main_logger);

    // auto test_logger = spdlog::get("main_logger");
    // test_logger->info("getlogger::helloworld");


    // spdlog::info("Welcome to spdlog!");
    //for (int i = 0; i < 1000; i++) {
    main_logger->info("Start server");
    //}
    // main_logger->info("Start server");
    //spdlog::shutdown();

    NSlicer::Balancer Balancer_;
    std::thread rebalance(NSlicer::RebalancingThread, &Balancer_);
    //spdlog::shutdown();

    NSlicer::THttpSlicerServer server(&Balancer_);
    server.Start();
    rebalance.join();
    return 0;
}
