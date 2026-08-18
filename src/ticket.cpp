#include "ticket.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "../third_party/json.hpp"

using json = nlohmann::json;

std::optional<std::vector<Ticket>> load_tickets(const std::string& json_path) {
    std::ifstream in(json_path);
    if (!in.is_open()) {
        std::cerr << "[错误] 无法打开数据文件: " << json_path << std::endl;
        return std::nullopt;
    }

    json data;
    try {
        in >> data;
    } catch (const std::exception& e) {
        std::cerr << "[错误] JSON 解析失败: " << e.what() << std::endl;
        return std::nullopt;
    }

    if (!data.is_array()) {
        std::cerr << "[错误] 根节点应为数组" << std::endl;
        return std::nullopt;
    }

    std::vector<Ticket> tickets;
    tickets.reserve(data.size());

    int skipped = 0;
    for (const auto& item : data) {
        try {
            Ticket t;
            t.ticket_id = item.at("ticket_id").get<std::string>();
            t.created_at = item.at("created_at").get<std::string>();
            t.category = item.at("category").get<std::string>();
            t.description = item.at("description").get<std::string>();
            t.priority = item.at("priority").get<std::string>();
            t.resolution_time_hours = item.at("resolution_time_hours").get<double>();
            t.satisfaction = item.at("satisfaction").get<int>();
            t.channel = item.at("channel").get<std::string>();
            t.is_resolved = item.at("is_resolved").get<bool>();

            // 派生日期字段,取 "YYYY-MM-DD" 部分
            size_t sp = t.created_at.find(' ');
            t.date = (sp == std::string::npos) ? t.created_at
                                               : t.created_at.substr(0, sp);

            tickets.push_back(std::move(t));
        } catch (const std::exception& e) {
            ++skipped;
            std::cerr << "[警告] 跳过字段缺失的工单: " << e.what() << std::endl;
        }
    }

    std::cout << "成功加载 " << tickets.size() << " 条工单"
              << (skipped ? "(跳过 " + std::to_string(skipped) + " 条)" : "")
              << std::endl;
    return tickets;
}
