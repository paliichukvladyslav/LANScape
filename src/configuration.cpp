#include "configuration.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

Configuration::Configuration() = default;

bool Configuration::load_from_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file: " << file_path << "\n";
        return false;
    }

    try {
        json j;
        file >> j;

        id = j.value("id", "unknown_node");
        display_name = j.value("display_name", "Unknown Node");
        device_ip = j.value("device_ip", "127.0.0.1");
        click_action = j.value("click_action", "");
        icon_id = j.value("icon_id", "default_icon");
        motd = j.value("MOTD", "");

    } catch (const json::exception& e) {
        std::cerr << "Error: JSON parsing failed: " << e.what() << "\n";
        return false;
    }

    return true;
}

std::string Configuration::get_id() const { return id; }
std::string Configuration::get_display_name() const { return display_name; }
std::string Configuration::get_device_ip() const { return device_ip; }
std::string Configuration::get_click_action() const { return click_action; }
std::string Configuration::get_icon_id() const { return icon_id; }
std::string Configuration::get_motd() const { return motd; }
