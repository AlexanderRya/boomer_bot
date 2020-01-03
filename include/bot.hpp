#ifndef DUMB_REMINDER_BOT_BOT_HPP
#define DUMB_REMINDER_BOT_BOT_HPP

#include "types.hpp"
#include "utils.hpp"

#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <fmt/format.h>

#include <functional>
#include <string>
#include <future>
#include <fstream>
#include <ctime>
#include <mutex>
#include <vector>
#include <chrono>

namespace bot {
    namespace ws_pp = websocketpp;

    class bot {
        using client = ws_pp::client<ws_pp::config::asio_tls_client>;
        using message_ptr = ws_pp::config::asio_client::message_type::ptr;
        using context_ptr = std::shared_ptr<boost::asio::ssl::context>;

        client c;
        client::connection_ptr con;

        bool connection_closed = false;
        bool acked = true;
        bool disconnected = false;
        bool can_use_ping = true;

        const std::string token;

        std::string name;
        types::i32 discriminator;
        std::string session_id;
        types::i32 last_sequence_id;

        types::i32 interval;

        std::thread heartbeat;
        std::thread gateway_thread;

        std::unordered_map<std::string, std::function<void(const nlohmann::json&)>> internal_event_map;
        std::unordered_map<std::string, std::function<void(const nlohmann::json&)>> event_map;
    public:
        const char prefix;

        std::vector<std::pair<std::string, types::snowflake>> reminder_list;

        bot(std::string token, const char prefix);
        void handle_gateway();
        void run();
        void push_on_command_event(const std::function<void(const nlohmann::json&)>&);
        void send_message(const types::snowflake&, const std::string&) const;
        void ping(const types::snowflake& channel_id);
        bool check_ping_cooldown() const;
    };
}

#endif //DUMB_REMINDER_BOT_BOT_HPP
