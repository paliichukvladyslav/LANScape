#define ZMQ_BUILD_DRAFT_API 1

#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>
#include <unordered_map>
#include <chrono>

using json = nlohmann::json;
using time_point = std::chrono::steady_clock::time_point;

int main() {
    std::cout << "Starting LANScape client...\n";

    zmq::context_t ctx;
    zmq::socket_t dish(ctx, zmq::socket_type::dish);
    dish.bind("udp://239.0.0.1:62626");
    dish.join("lanscape");

    zmq::socket_t rep(ctx, zmq::socket_type::rep);
    rep.bind("tcp://127.0.0.1:62627");

    std::unordered_map<std::string, std::pair<json, time_point>> node_cache;

    zmq::pollitem_t items[] = {
        { dish.handle(), 0, ZMQ_POLLIN, 0 },
        { rep.handle(),  0, ZMQ_POLLIN, 0 }
    };
    std::cout << "Waiting for broadcasts...\n\n";

    const int OFFLINE_TIMEOUT_SEC = 5;

    while (true) {
        zmq::poll(items, 2, std::chrono::milliseconds(1000));

        auto now = std::chrono::steady_clock::now();

        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t msg;
            while(dish.recv(msg, zmq::recv_flags::dontwait)) {
                std::string payload_str(static_cast<char*>(msg.data()), msg.size());

                try {
                    json payload = json::parse(payload_str);
                    std::string id = payload.value("id", "");

                    if (!id.empty()) {
                        payload["status"] = "online";

                        bool was_offline = (node_cache.find(id) == node_cache.end() ||
                            node_cache[id].first.value("status", "") == "offline");

                        node_cache[id] = {payload, now};

                        if (was_offline) {
                            std::cout << "Node online: " << payload.value("display_name", id) << "\n";
                        }
                    }
                } catch (...) {
                    /* do nothing */
                }
            }
        }

        for (auto &pair : node_cache) {
            auto &node_data = pair.second.first;
            auto last_seen = pair.second.second;

            if (node_data.value("status", "") == "online") {
                auto seconds_passed = std::chrono::duration_cast<std::chrono::seconds>(now - last_seen).count();

                if (seconds_passed > OFFLINE_TIMEOUT_SEC) {
                    node_data["status"] = "offline";
                    std::cout << "Node offline: " << node_data.value("display_name", pair.first) << "\n";
                }
            }
        }

        if (items[1].revents & ZMQ_POLLIN) {
            zmq::message_t req;
            if (rep.recv(req, zmq::recv_flags::dontwait)) {
                std::cout << "Shell Extension requested node list.\n";

                json response_array = json::array();
                for (const auto &pair : node_cache) {
                    response_array.push_back(pair.second.first);
                }

                std::string response_str = response_array.dump();
                zmq::message_t reply(response_str.begin(), response_str.end());
                rep.send(reply, zmq::send_flags::none);
            }
        }
    }

    return 0;
}
