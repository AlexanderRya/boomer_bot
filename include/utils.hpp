#ifndef DUMB_REMINDER_BOT_UTILS_HPP
#define DUMB_REMINDER_BOT_UTILS_HPP

#include "types.hpp"

#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <iostream>
#include <fstream>

namespace bot::util {
	inline std::string get_api_url() {
		return "https://discordapp.com/api/v6";
	}

	inline std::string get_ws_url(const std::string& token) {
		auto json = nlohmann::json::parse(cpr::Get(
			cpr::Url{ get_api_url() + "/gateway/bot" },
			cpr::Header{{ "Authorization", "Bot " + token }}).text);
		std::cout << json << "\n";
		try {
			return json["url"].get<std::string>();
		} catch (...) {
			std::cout << "Error, invalid token.\n\n";
			std::abort();
		}
	}

	inline std::string get_identify_packet(const std::string& token) {
		return nlohmann::json{
			{ "op", 2 },
			{ "d", {
				{ "token", token },
				{ "properties", {
					{ "$os", "linux" },
					{ "$browser", "chrome" },
					{ "$device", "daknigs_bot" }
				} }
			} }
		}.dump();
	}

	inline auto split(const std::string& s, const std::string& delimiter) {
		size_t pos_start = 0, pos_end, delim_len = delimiter.length();
		std::vector<std::string> res;
		while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
			auto token = s.substr(pos_start, pos_end - pos_start);
			pos_start = pos_end + delim_len;
			res.emplace_back(token);
		}
		res.emplace_back(s.substr(pos_start));
		return res;
	}

	inline void deserialize(std::vector<std::pair<std::string, types::snowflake>>& v) {
		std::ifstream in("../daily_reminder.txt");
		std::string str;
		while (std::getline(in, str)) {
			auto item = split(str, ",");
			v.emplace_back(item[0], item[1]);
		}
		in.close();
	}

	inline void serialize(const std::vector<std::pair<std::string, types::snowflake>>& v) {
		std::ofstream out("../daily_reminder.txt", std::ios::out | std::ios::trunc);
		for (const auto& [username, id] : v) {
			out << username << ',' << id << "\n";
		}
		out.close();
	}

	template <typename Ty>
	inline Ty get_from_json(const nlohmann::json& j, const char* key) {
		if (j.contains(key)) {
			return j[key].get<Ty>();
		}
		return Ty{};
	}

	inline static const std::string user_agent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/76.0.3809.132 Safari/537.36";
}

#endif //DUMB_REMINDER_BOT_UTILS_HPP
