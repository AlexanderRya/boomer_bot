#ifndef DUMB_REMINDER_BOT_UTILS_HPP
#define DUMB_REMINDER_BOT_UTILS_HPP

#include "types.hpp"

#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <iostream>
#include <fstream>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

namespace bot::util {
    inline static const std::string user_agent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/76.0.3809.132 Safari/537.36";

    inline std::string get_api_url() {
        return "https://discordapp.com/api/v6";
    }

    inline std::string get_ws_url(const std::string& token) {
        auto json = nlohmann::json::parse(cpr::Get(
            cpr::Url{ get_api_url() + "/gateway/bot" },
            cpr::Header{
                { "Authorization", "Bot " + token },
                { "User-Agent", user_agent }
            }).text);
        std::cout << json << "\n";
        if (!json.contains("url")) {
            throw std::runtime_error("Error, invalid token.\n\n");
        }
        return json["url"].get<std::string>();
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
                }}
            }}
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
        std::string str{};
        while (std::getline(in, str)) {
            auto item = split(str.substr(10), " | id: ");
            v.emplace_back(item[0], item[1]);
        }
        in.close();
    }

    inline void serialize(const std::vector<std::pair<std::string, types::snowflake>>& v) {
        std::ofstream out("../daily_reminder.txt", std::ios::out | std::ios::trunc);
        for (const auto&[username, id] : v) {
            out << "username: " << username << " | id: " << id << "\n";
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

    inline bool verify_subject_alternative_name(const char* hostname, X509* cert) {
        STACK_OF(GENERAL_NAME)* san_names = nullptr;
        san_names = (STACK_OF(GENERAL_NAME)*)X509_get_ext_d2i(cert, NID_subject_alt_name, nullptr, nullptr);

        if (san_names == nullptr) {
            return false;
        }

        int san_names_count = sk_GENERAL_NAME_num(san_names);

        bool result = false;

        for (int i = 0; i < san_names_count; i++) {
            const GENERAL_NAME* current_name = sk_GENERAL_NAME_value(san_names, i);
            if (current_name->type != GEN_DNS) {
                continue;
            }

            char* dns_name = (char*)ASN1_STRING_data(current_name->d.dNSName);
            if (ASN1_STRING_length(current_name->d.dNSName) != strlen(dns_name)) {
                break;
            }

            result = (strcasecmp(hostname, dns_name) == 0);
        }

        sk_GENERAL_NAME_pop_free(san_names, GENERAL_NAME_free);

        return result;
    }

    inline bool verify_common_name(const char* hostname, X509* cert) {
        int common_name_loc = X509_NAME_get_index_by_NID(X509_get_subject_name(cert), NID_commonName, -1);
        if (common_name_loc < 0) {
            return false;
        }

        X509_NAME_ENTRY* common_name_entry = X509_NAME_get_entry(X509_get_subject_name(cert), common_name_loc);
        if (common_name_entry == nullptr) {
            return false;
        }

        ASN1_STRING* common_name_asn1 = X509_NAME_ENTRY_get_data(common_name_entry);
        if (common_name_asn1 == nullptr) {
            return false;
        }

        char* common_name_str = (char*)ASN1_STRING_data(common_name_asn1);
        if (ASN1_STRING_length(common_name_asn1) != strlen(common_name_str)) {
            return false;
        }
        return (strcasecmp(hostname, common_name_str) == 0);
    }

    inline bool verify_certificate(const char* hostname, bool preverified, boost::asio::ssl::verify_context& ctx) {
        int depth = X509_STORE_CTX_get_error_depth(ctx.native_handle());

        if (depth == 0 && preverified) {
            X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());

            if (verify_subject_alternative_name(hostname, cert)) {
                return true;
            } else {
                return verify_common_name(hostname, cert);
            }
        }

        return preverified;
    }
}

#endif //DUMB_REMINDER_BOT_UTILS_HPP
