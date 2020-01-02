#include "bot.hpp"

int main() {
	std::ifstream f("../token.txt");
	std::string token{ std::istreambuf_iterator{ f }, {} };
	bot::bot b(std::move(token), '%');
	bot::util::deserialize(b.reminder_list);
	b.push_on_command_event([&b](const nlohmann::json& j) {
		auto author_id = j["d"]["author"]["id"].get<std::string>();
		auto author_username = j["d"]["author"]["username"].get<std::string>();
		auto channel_id = j["d"]["channel_id"].get<std::string>();
		auto guild_id = j["d"]["guild_id"].get<std::string>();
		auto content = j["d"]["content"].get<std::string>();
		if (content[0] == b.prefix /*&& channel_id == "526518219549442071"*/) {
			if (content.substr(1, 3) == "sub") {
				if (content.size() > 4 && content.substr(4) == " to daily_reminder") {
					if (std::find_if(b.reminder_list.begin(), b.reminder_list.end(), [&](const auto& user) {
						return user.second == author_id;
					}) != b.reminder_list.end()) {
						b.send_message(channel_id, "You already are on the reminder list!");
					} else {
						b.send_message(channel_id, "Successfully subbed to the daily reminder!");
						b.reminder_list.emplace_back(author_username, author_id);
						bot::util::serialize(b.reminder_list);
					}
				} else {
					b.send_message(channel_id, "Invalid syntax, try: `%sub to daily_reminder`");
				}
			} else if (content.substr(1, 4) == "ping") {
				if (content.size() > 5 && content.substr(5) == " subs daily_reminder") {
					if (author_id == "248901263054471168") {
						b.ping(channel_id);
					} else {
						b.send_message(channel_id, "Only our lord and saviour DaKnig can use this command");
					}
				} else {
					b.send_message(channel_id, "Invalid syntax, try: `%ping subs daily_reminder`");
				}
			} else if (content.substr(1, 5) == "unsub") {
				auto it = std::find_if(b.reminder_list.begin(), b.reminder_list.end(), [&](const auto& user) {
					return user.second == author_id;
				});
				if (it != b.reminder_list.end()) {
					b.reminder_list.erase(it);
					b.send_message(channel_id, "Successfully unsubbed to the daily reminder!");
				} else {
					b.send_message(channel_id, "You are not present on the reminder list!");
				}
			} else if (content.substr(1, 4) == "help") {
				b.send_message(channel_id,
							   "Command list:\n"
							   "%help: basically this command duh\n"
							   "%sub: syntax: `%sub to daily_reminder`, you choose to get pinged to get reminded that: yall dum\n"
							   "%ping: syntax: `%ping subs daily_reminder`, only DaKnig's allowed to use this command, it pings everyone on the list (has a 6 hours cooldown)\n"
							   "%unsub: syntax: `%unsub`, unsubs from this service (:feelsbad:)\n"
							   "%list: syntax: `%list`, shows everyone subscribed to the service\n"
				);
			} else if (content.substr(1, 4) == "list") {
				if (b.reminder_list.empty()) {
					b.send_message(channel_id, "No one is on the list!");
				} else {
					std::stringstream s;
					s << "List of the members of the daily reminder:\n";
					for (const auto&[username, id] : b.reminder_list) {
						s << username << "\n";
					}
					b.send_message(channel_id, s.str());
				}
			} else {
				b.send_message(channel_id, "Invalid command");
			}
		}
	});
	b.run();
	return 0;
}
