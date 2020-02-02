#include "bot.hpp"

namespace bot {
    bot::bot(std::string token, const char prefix)
        : token(std::move(token)),
          prefix(prefix) {
        internal_event_map["READY"] = [this](const nlohmann::json& j) {
            heartbeat = std::thread([this]() {
                while (!connection_closed) {
                    nlohmann::json data{
                        { "op", 1 },
                        { "d", nullptr }
                    };
                    if (last_sequence_id != -1) {
                        data["d"] = last_sequence_id;
                    }
                    con->send(data.dump());
                    std::cout << "sent opcode 11\n";
                    acked = false;
                    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                    if (!acked) {
                        std::cout << "\n\n[Error] Reconnecting...\n\n";
                        connection_closed = true;
                    }
                }
            });
            session_id = j["d"]["session_id"].get<std::string>();
            name = j["d"]["user"]["username"].get<std::string>();
            discriminator = std::stoi(j["d"]["user"]["discriminator"].get<std::string>());
            std::cout << fmt::format("READY\nLogged in as: {}#{}\n\n", name, discriminator);
        };
    }

    void bot::init_gateway() {
        static bool is_init = false;

        std::string uri = util::get_ws_url(this->token);

        c.set_access_channels(websocketpp::log::alevel::all);
        c.clear_access_channels(websocketpp::log::alevel::frame_payload);
        c.set_error_channels(websocketpp::log::elevel::all);

        if (!is_init) {
            c.init_asio();
            is_init = true;
        }

        c.start_perpetual();
        perpetual = std::thread(&client::run, &c);

        c.set_message_handler([this](const websocketpp::connection_hdl &, const message_ptr &msg) {
            nlohmann::json j = nlohmann::json::parse(msg->get_payload());
            switch (j["op"].get<int>()) {
                case 10: {
                    //std::cout << "opcode 10 received\n";
                    interval = j["d"]["heartbeat_interval"].get<int>();
                    con->send(util::get_identify_packet(this->token));
                } break;

                case 11: {
                    std::cout << "acked! json: " << j << "\n";
                    acked = true;
                } break;

                default: {
                    last_sequence_id = j.contains("s") ? j["s"].get<int>() : -1;
                    auto event = j["t"].get<std::string>();
                    if (internal_event_map.contains(event)) {
                        internal_event_map[event](j);
                    } else if (event_map.contains(event)) {
                        event_map[event](j);
                    } else {
                        //std::cout << event << " (unhandled)\n";
                    }
                } break;
            }
        });

        c.set_tls_init_handler([](const websocketpp::connection_hdl &) {
            auto ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
            try {
                ctx->set_options(boost::asio::ssl::context::default_workarounds |
                                 boost::asio::ssl::context::no_sslv2 |
                                 boost::asio::ssl::context::no_sslv3 |
                                 boost::asio::ssl::context::single_dh_use);
                ctx->set_verify_mode(boost::asio::ssl::verify_peer);
                ctx->set_verify_callback([](const bool preverified, boost::asio::ssl::verify_context &ictx) {
                    return util::verify_certificate("discord.gg", preverified, ictx);
                });
                ctx->load_verify_file("../ca-chain.cert.pem");
            } catch (std::exception& e) {
                std::cout << e.what() << "\n";
            }
            return ctx;
        });

        ws_pp::lib::error_code ec;
        con = c.get_connection(uri, ec);

        if (ec) {
            throw std::runtime_error("connect initialization error: " + ec.message());
        }

        c.connect(con);
    }

    void bot::run() {
        using namespace std::chrono_literals;

        init_gateway();

        while (true) {
            std::this_thread::sleep_for(10ms);
            if (connection_closed) {
                con->pause_reading();
                con->close(ws_pp::close::status::service_restart, "Success");
                con.reset();

                std::this_thread::sleep_for(20s);

                heartbeat.join();

                ws_pp::lib::error_code ec;
                con = c.get_connection(util::get_ws_url(this->token), ec);

                if (ec) {
                    throw std::runtime_error("connect initialization error: " + ec.message());
                }

                c.connect(con);

                acked = true;
                connection_closed = false;
            }
        }
    }

    void bot::push_on_command_event(const std::function<void(const nlohmann::json&)>& f) {
        event_map["MESSAGE_CREATE"] = f;
    }

    void bot::send_message(const types::snowflake& channel_id, const std::string& content) const {
        auto body = nlohmann::json{
            { "content", content },
            { "tts", false }
        };
        //std::cout << body << "\n\n";
        auto r = cpr::Post(cpr::Url{ util::get_api_url() + "/channels/" + channel_id + "/messages" },
                           cpr::Header{
                               { "Content-Type", "application/json" },
                               { "User-Agent", util::user_agent },
                               { "Authorization", "Bot " + token }
                           },
                           cpr::Body{ body.dump() });
        std::cout << "send_message status code: " << r.status_code << "\n" << "body: " << r.text << "\n";
    }

    void bot::ping(const types::snowflake& channel_id) {
        using namespace std::chrono_literals;
        can_use_ping = check_ping_cooldown();
        if (can_use_ping) {
            std::stringstream s{};
            if (reminder_list.empty()) {
                send_message(channel_id, "No one is on the reminder list!");
            }
            for (const auto& [username, id] : reminder_list) {
                s << fmt::format("<@!{}> ", id);
            }
            send_message(channel_id, s.str());
            can_use_ping = false;
            std::ofstream("../time.txt", std::ios::out | std::ios::trunc) << (std::time(nullptr) + 21600);
        } else {
            send_message(channel_id, "You can't do that right now!");
        }
    }

    bool bot::check_ping_cooldown() const {
        std::time_t next_ping{};
        std::ifstream("../time.txt") >> next_ping;
        return std::time(nullptr) >= next_ping;
    }
}
