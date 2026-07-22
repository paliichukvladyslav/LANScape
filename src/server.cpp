#define ZMQ_BUILD_DRAFT_API 1

#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>


#include "configuration.hpp"
using json = nlohmann::json;

int main () {
	std::cout << "Starting LANScape server...\n";

	Configuration config;
	if (!config.load_from_file("config.json")) {
		std::cerr << "Failed to load config, exiting...\n";
		return 1;
	}

	std::cout << "Loaded config for: " << config.get_display_name() << "\n";

	zmq::context_t ctx;
	zmq::socket_t radio(ctx, zmq::socket_type::radio);
	radio.connect("udp://239.0.0.1:62626");

	std::string group_name = "lanscape";

	while (true) {
		json payload = {
			{"id", config.get_id()},
			{"display_name", config.get_display_name()},
			{"device_ip", config.get_device_ip()},
			{"click_action", config.get_click_action()},
			{"icon_id", config.get_icon_id()},
			{"motd", config.get_motd()},
			{"status", "online"}
		};
		std::string payload_str = payload.dump();

		zmq::message_t msg(payload_str.begin(), payload_str.end());
		msg.set_group(group_name.c_str());

		radio.send(msg, zmq::send_flags::none);

		std::this_thread::sleep_for(std::chrono::seconds(2));
	}

	return 0;
}
