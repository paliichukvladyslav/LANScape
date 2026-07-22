#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <string>

class Configuration {
private:
    std::string id;
    std::string display_name;
    std::string device_ip;
    std::string click_action;
    std::string icon_id;
    std::string motd;

public:
    Configuration();

    bool load_from_file(const std::string& file_path);

    std::string get_id() const;
    std::string get_display_name() const;
    std::string get_device_ip() const;
    std::string get_click_action() const;
    std::string get_icon_id() const;
    std::string get_motd() const;
};

#endif
