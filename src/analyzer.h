#pragma once

#include <string>
#include <vector>
#include <utility>

#include "ticket.h"

// ---- 各维度统计结果 ----

struct Kpi {
    int total = 0;
    int unresolved = 0;
    double avg_satisfaction = 0.0;
    double avg_resolution_hours = 0.0;
};

struct CategoryStat {
    std::string name;
    int count = 0;
    double avg_resolution = 0.0;
    double avg_satisfaction = 0.0;
    int unresolved = 0;
    std::vector<double> resolution_values;  // 供箱线图
};

struct DayStat {
    std::string date;
    int count = 0;
    int unresolved = 0;
};

struct PriorityStat {
    std::string name;
    int count = 0;
    double avg_resolution = 0.0;
    double avg_satisfaction = 0.0;
};

struct ChannelStat {
    std::string name;
    int count = 0;
    double avg_resolution = 0.0;
    double avg_satisfaction = 0.0;
    int unresolved = 0;
};

struct KeywordStat {
    std::string keyword;
    int count = 0;
};

// 异常信号
struct Anomaly {
    std::string title;                 // 异常名称
    std::string basis;                 // 判断依据
    std::vector<std::string> tickets;  // 关联工单 ID
};

struct AnalysisResult {
    Kpi kpi;
    std::vector<CategoryStat> categories;      // 按数量降序
    std::vector<DayStat> daily;                // 按日期升序
    std::vector<PriorityStat> priorities;      // 高/中/低 固定顺序
    std::vector<ChannelStat> channels;         // 按数量降序
    std::vector<int> satisfaction_dist;        // 下标 0..4 -> 1..5 分
    std::vector<KeywordStat> keywords;         // 高频关键词,按频次降序
    std::vector<std::pair<double, int>> scatter;  // (处理时长, 满意度) 散点
    double corr_resolution_satisfaction = 0.0; // 处理时长↔满意度 Pearson 相关系数
    std::vector<Anomaly> anomalies;
    std::vector<Ticket> tickets;               // 全量明细(供前端表格)
};

// 执行全部分析
AnalysisResult analyze(const std::vector<Ticket>& tickets);
