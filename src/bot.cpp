#include "bot.hpp"

namespace bot {
	bot::bot(std::string token, const char prefix)
	: token(std::move(token)),
	prefix(prefix) {
		internal_event_map["READY"] = [this](const nlohmann::json& j) {
			heartbeat = std::thread([this]() {
				nlohmann::json data{
					{ "op", 1 },
					{ "d", nullptr }
				};
				if (last_sequence_id != -1) {
					data["d"] = last_sequence_id;
				}
				while (!connection_closed) {
					con->send(data.dump());
					std::cout << "sent opcode 11\n";
					acked = false;
					std::this_thread::sleep_for(std::chrono::milliseconds(interval));
					if (!acked) {
						std::cout << "heartbeat error, trying to reconnect...\n";
						disconnected = true;
						data = nlohmann::json({ { "token", this->token },
												  { "session_id", session_id },
												  { "seq", last_sequence_id } });
						con->send(data.dump());
					}
				}
			});
			session_id = j["d"]["session_id"].get<std::string>();
			name = j["d"]["user"]["username"].get<std::string>();
			discriminator = std::stoi(j["d"]["user"]["discriminator"].get<std::string>());
			std::cout << fmt::format("READY\nLogged in as: {}#{}\n\n", name, discriminator);
		};

		c.set_access_channels(websocketpp::log::alevel::all);
		c.clear_access_channels(websocketpp::log::alevel::frame_payload);
		c.set_error_channels(websocketpp::log::elevel::all);
		c.init_asio();

		c.set_tls_init_handler([](const ws_pp::connection_hdl&) {
			auto ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
			ctx->set_options(boost::asio::ssl::context::default_workarounds |
							 boost::asio::ssl::context::no_sslv2 |
							 boost::asio::ssl::context::no_sslv3 |
							 boost::asio::ssl::context::single_dh_use);
			return ctx;
		});
		c.set_message_handler([this](const websocketpp::connection_hdl& hdl, const message_ptr& msg) {
			nlohmann::json j = nlohmann::json::parse(msg->get_payload());
			switch (j["op"].get<int>()) {
				case 9: {
					if (disconnected) {
						std::this_thread::sleep_for(std::chrono::milliseconds(5000));
						con->send(util::get_identify_packet(this->token));
						std::cout << "successfully reconnected\n";
					}
				}
				case 10: {
					std::cout << "opcode 10 received\n";
					interval = j["d"]["heartbeat_interval"].get<int>();
					con->send(util::get_identify_packet(this->token));
				} break;
				case 11: {
					std::cout << "acked! json: " << j << "\n";
					acked = true;
				} break;
				default: {
					last_sequence_id = j["s"].is_number() && j.contains("s") ? j["s"].get<int>() : -1;
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
		c.start_perpetual();

		ws_pp::lib::error_code ec;
		con = c.get_connection(util::get_ws_url(this->token), ec);

		if (ec) {
			throw std::runtime_error("connect initialization error: " + ec.message());
		}

		background_ws = std::thread(&client::run, &c);

		c.connect(con);
	}

	void bot::run() {
		using namespace std::chrono_literals;
		c.run();
		std::this_thread::sleep_for(1ms);
		heartbeat.join();
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
		can_use_ping = check_ping_delay();
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
			next_ping = std::time(nullptr) + 10;
		} else {
			send_message(channel_id, "You can't do that right now!");
		}
	}

	bool bot::check_ping_delay() {
		return std::time(nullptr) >= next_ping;
	}
}