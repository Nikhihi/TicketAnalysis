#pragma once

#include <string>
#include <vector>
#include <optional>

// 工单数据结构，字段与 task5_ticket_fields.md 对齐
struct Ticket {
    std::string ticket_id;        // 工单唯一编号
    std::string created_at;       // 创建时间 "YYYY-MM-DD HH:MM"
    std::string date;             // 派生字段:创建日期 "YYYY-MM-DD"
    std::string category;         // 问题分类标签
    std::string description;      // 问题描述
    std::string priority;         // 优先级 高/中/低
    double resolution_time_hours; // 处理时长(小时)
    int satisfaction;             // 满意度评分 1-5
    std::string channel;          // 来源渠道 在线/电话/邮件
    bool is_resolved;             // 是否已解决
};

// 解析 ticket 数组 JSON,失败返回 nullopt
std::optional<std::vector<Ticket>> load_tickets(const std::string& json_path);
