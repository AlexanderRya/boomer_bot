#include "bot.hpp"

int main() {
    const std::string daknig_id = "248901263054471168";
    std::ifstream f("../token.txt");
    std::string token{ std::istreambuf_iterator{ f }, {}};
    bot::bot b(std::move(token), '%');
    bot::util::deserialize(b.reminder_list);
    b.push_on_command_event([&b, &daknig_id](const nlohmann::json& j) {
        auto content = bot::util::get_from_json<std::string>(j["d"], "content");
        if (content[0] == b.prefix /*&& channel_id == "526518219549442071"*/) {
            auto author_id = bot::util::get_from_json<std::string>(j["d"]["author"], "id");
            auto author_username = bot::util::get_from_json<std::string>(j["d"]["author"], "username");
            auto channel_id = bot::util::get_from_json<std::string>(j["d"], "channel_id");
            auto guild_id = bot::util::get_from_json<std::string>(j["d"], "guild_id");
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
                if (author_id == daknig_id || author_id == "187927238274973696") {
                    if (content.size() > 5 && content.substr(5) == " subs daily_reminder") {
                        b.ping(channel_id);
                    } else {
                        b.send_message(channel_id, "Invalid syntax, try: `%ping subs daily_reminder`");
                    }
                } else {
                    b.send_message(channel_id, "Only our lord and saviour DaKnig can use this command");
                }
            } else if (content.substr(1, 5) == "unsub") {
                if (content.size() == 6) {
                    auto it = std::find_if(b.reminder_list.begin(), b.reminder_list.end(), [&](const auto& user) {
                        return user.second == author_id;
                    });
                    if (it != b.reminder_list.end()) {
                        b.reminder_list.erase(it);
                        bot::util::serialize(b.reminder_list);
                        b.send_message(channel_id, "Successfully unsubbed to the daily reminder!");
                    } else {
                        b.send_message(channel_id, "You are not present on the reminder list!");
                    }
                } else {
                    b.send_message(channel_id, "Invalid syntax, try: `%unsub`");
                }
            } else if (content.substr(1, 4) == "help") {
                b.send_message(channel_id,
                               "Command list:\n"
                               "%help: basically this command duh\n"
                               "%sub: syntax: `%sub to daily_reminder`, you choose to get pinged to get reminded that: yall dum\n"
                               "%ping: syntax: `%ping subs daily_reminder`, only DaKnig's allowed to use this command, it pings everyone on the list (has a 6 hours cooldown)\n"
                               "%unsub: syntax: `%unsub`, unsubs from this service (:feelsbad:)\n"
                               "%list: syntax: `%list`, shows everyone subscribed to the service\n"
                               "%ask: a glorious command."
                );
            } else if (content.substr(1, 4) == "list") {
                if (b.reminder_list.empty()) {
                    b.send_message(channel_id, "No one is on the list!");
                } else {
                    std::stringstream s;
                    s << "List of the members of the daily reminder:\n";
                    for (const auto& [username, id] : b.reminder_list) {
                        s << username << "\n";
                    }
                    b.send_message(channel_id, s.str());
                }
            } else if (content.substr(1, 3) == "ask") {
                b.send_message(channel_id, "Rules §2.1: **Actually ask your question.** Posting \"Can somebody help me?\" or similar messages is annoying and doesn't help anyone, including you.");
            } else {
                b.send_message(channel_id, "Invalid command");
            }
        }
    });
    b.run();
    return 0;
}
