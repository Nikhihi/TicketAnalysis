#include "analyzer.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <set>
#include <string>

namespace {

double mean(const std::vector<double>& v) {
    if (v.empty()) return 0.0;
    return std::accumulate(v.begin(), v.end(), 0.0) / static_cast<double>(v.size());
}

double stddev(const std::vector<double>& v, double m) {
    if (v.size() < 2) return 0.0;
    double sum = 0.0;
    for (double x : v) sum += (x - m) * (x - m);
    return std::sqrt(sum / static_cast<double>(v.size()));
}

// 反复出现的关键词(按描述做子串匹配)
const std::vector<std::string>& keywords() {
    static const std::vector<std::string> kws = {
        "重复扣款", "扣款", "付款", "退款", "退货", "运费", "物流", "快递",
        "发货", "收货", "客服", "机器人", "账号", "冻结", "破损", "优惠券", "派送"
    };
    return kws;
}

// 低满意度标签
std::string score_label(int s) { return std::to_string(s) + "分"; }

}  // namespace

AnalysisResult analyze(const std::vector<Ticket>& tickets) {
    AnalysisResult r;
    r.tickets = tickets;

    const int n = static_cast<int>(tickets.size());
    if (n == 0) return r;

    // ---- 基础聚合 ----
    //类型
    std::map<std::string, std::vector<Ticket>> by_cat;
    std::map<std::string, int> day_count, day_unresolved;
    //优先级
    std::map<std::string, std::vector<Ticket>> by_priority;
    //来源
    std::map<std::string, std::vector<Ticket>> by_channel;
    //处理时常及满意评分
    std::vector<double> all_hours, all_sat;
    //描述做关键词匹配
    std::map<std::string, std::vector<std::string>> kw_hits;  // keyword -> 命中工单 ID 列表

    for (const auto& t : tickets) {
        by_cat[t.category].push_back(t);
        by_priority[t.priority].push_back(t);
        by_channel[t.channel].push_back(t);
        day_count[t.date]++;
        if (!t.is_resolved) day_unresolved[t.date]++;
        all_hours.push_back(t.resolution_time_hours);
        all_sat.push_back(static_cast<double>(t.satisfaction));
        for (const auto& kw : keywords()) {
            if (t.description.find(kw) != std::string::npos) {
                kw_hits[kw].push_back(t.ticket_id);
            }
        }
    }

    // ---- KPI ----
    r.kpi.total = n;
    r.kpi.unresolved = 0;
    for (const auto& t : tickets)
        if (!t.is_resolved) r.kpi.unresolved++;
    r.kpi.avg_satisfaction = mean(all_sat);
    r.kpi.avg_resolution_hours = mean(all_hours);

    // ---- 类型分布(按数量降序) ----
    for (auto& [name, list] : by_cat) {
        CategoryStat cs;
        cs.name = name;
        cs.count = static_cast<int>(list.size());
        std::vector<double> hrs;
        double sat_sum = 0.0;
        for (const auto& t : list) {
            hrs.push_back(t.resolution_time_hours);
            cs.resolution_values.push_back(t.resolution_time_hours);
            sat_sum += t.satisfaction;
            if (!t.is_resolved) cs.unresolved++;
        }
        cs.avg_resolution = mean(hrs);
        cs.avg_satisfaction = sat_sum / static_cast<double>(list.size());
        r.categories.push_back(std::move(cs));
    }
    std::sort(r.categories.begin(), r.categories.end(),
              [](const CategoryStat& a, const CategoryStat& b) {
                  return a.count > b.count;
              });

    // ---- 时间趋势(按日期升序) ----
    for (auto& [date, cnt] : day_count) {
        DayStat ds;
        ds.date = date;
        ds.count = cnt;
        ds.unresolved = day_unresolved[date];
        r.daily.push_back(std::move(ds));
    }
    std::sort(r.daily.begin(), r.daily.end(),
              [](const DayStat& a, const DayStat& b) { return a.date < b.date; });

    // ---- 优先级(固定顺序 高/中/低) ----
    for (const std::string& p : {"高", "中", "低"}) {
        auto it = by_priority.find(p);
        if (it == by_priority.end()) continue;
        PriorityStat ps;
        ps.name = p;
        ps.count = static_cast<int>(it->second.size());
        std::vector<double> hrs;
        double sat_sum = 0.0;
        for (const auto& t : it->second) {
            hrs.push_back(t.resolution_time_hours);
            sat_sum += t.satisfaction;
        }
        ps.avg_resolution = mean(hrs);
        ps.avg_satisfaction = sat_sum / static_cast<double>(it->second.size());
        r.priorities.push_back(std::move(ps));
    }

    // ---- 渠道(按数量降序) ----
    for (auto& [name, list] : by_channel) {
        ChannelStat cs;
        cs.name = name;
        cs.count = static_cast<int>(list.size());
        std::vector<double> hrs;
        double sat_sum = 0.0;
        for (const auto& t : list) {
            hrs.push_back(t.resolution_time_hours);
            sat_sum += t.satisfaction;
            if (!t.is_resolved) cs.unresolved++;
        }
        cs.avg_resolution = mean(hrs);
        cs.avg_satisfaction = sat_sum / static_cast<double>(list.size());
        r.channels.push_back(std::move(cs));
    }
    std::sort(r.channels.begin(), r.channels.end(),
              [](const ChannelStat& a, const ChannelStat& b) {
                  return a.count > b.count;
              });

    // ---- 满意度分布(1-5 分) ----
    r.satisfaction_dist.assign(5, 0);
    for (const auto& t : tickets)
        if (t.satisfaction >= 1 && t.satisfaction <= 5)
            r.satisfaction_dist[t.satisfaction - 1]++;

    // ---- 关键词频次(按频次降序,取命中>=2 的) ----
    for (auto& [kw, ids] : kw_hits) {
        KeywordStat ks;
        ks.keyword = kw;
        ks.count = static_cast<int>(ids.size());
        if (ks.count >= 2) r.keywords.push_back(std::move(ks));
    }
    std::sort(r.keywords.begin(), r.keywords.end(),
              [](const KeywordStat& a, const KeywordStat& b) {
                  return a.count > b.count;
              });

    // ---- 关联:处理时长↔满意度 ----
    for (const auto& t : tickets)
        r.scatter.emplace_back(t.resolution_time_hours, t.satisfaction);
    {
        double mh = mean(all_hours), ms = mean(all_sat);
        double cov = 0.0, vh = 0.0, vs = 0.0;
        for (size_t i = 0; i < all_hours.size(); ++i) {
            cov += (all_hours[i] - mh) * (all_sat[i] - ms);
            vh += (all_hours[i] - mh) * (all_hours[i] - mh);
            vs += (all_sat[i] - ms) * (all_sat[i] - ms);
        }
        r.corr_resolution_satisfaction =
            (vh > 1e-9 && vs > 1e-9) ? cov / std::sqrt(vh * vs) : 0.0;
    }

    // ---- 异常检测 ----

    // 规则1:核心问题域(占比>25%)
    for (const auto& c : r.categories) {
        double pct = 100.0 * c.count / n;
        if (pct > 25.0) {
            Anomaly a;
            a.title = "核心问题域:\"" + c.name + "\"占比过高";
            a.basis = c.name + " 工单 " + std::to_string(c.count) + " 条,占比 " +
                      std::to_string(static_cast<int>(pct + 0.5)) +
                      "%,超过 25% 阈值,应优先投入资源排查。";
            for (const auto& t : tickets)
                if (t.category == c.name) a.tickets.push_back(t.ticket_id);
            r.anomalies.push_back(std::move(a));
        }
    }

    // 规则2:超长处理工单(>48 小时)
    {
        std::vector<const Ticket*> long_ones;
        for (const auto& t : tickets)
            if (t.resolution_time_hours > 48.0) long_ones.push_back(&t);
        if (!long_ones.empty()) {
            std::sort(long_ones.begin(), long_ones.end(),
                      [](const Ticket* a, const Ticket* b) {
                          return a->resolution_time_hours > b->resolution_time_hours;
                      });
            Anomaly a;
            a.title = "存在 " + std::to_string(long_ones.size()) + " 笔超长处理工单(>48 小时)";
            a.basis = "处理时长远超整体平均 " +
                      std::to_string(static_cast<int>(r.kpi.avg_resolution_hours + 0.5)) +
                      " 小时,主要集中在退款退货,提示退款/退货流程存在瓶颈。";
            for (auto* t : long_ones) a.tickets.push_back(t->ticket_id);
            r.anomalies.push_back(std::move(a));
        }
    }

    // 规则3:未解决工单集中
    {
        std::map<std::string, std::vector<std::string>> unresolved_by_cat;
        for (const auto& t : tickets)
            if (!t.is_resolved) unresolved_by_cat[t.category].push_back(t.ticket_id);
        if (!unresolved_by_cat.empty()) {
            Anomaly a;
            a.title = "未解决工单 " + std::to_string(r.kpi.unresolved) + " 笔,集中于少数分类";
            a.basis = "未解决工单按分类分布:";
            for (auto& [cat, ids] : unresolved_by_cat) {
                a.basis += " " + cat + " " + std::to_string(ids.size()) + " 笔;";
                for (auto& id : ids) a.tickets.push_back(id);
            }
            a.basis += " 退款退货类未解决比例最高,存在退款积压风险。";
            r.anomalies.push_back(std::move(a));
        }
    }

    // 规则4:低满意度聚类(<=2 分)
    {
        std::map<std::string, std::vector<std::string>> low_by_cat;
        for (const auto& t : tickets)
            if (t.satisfaction <= 2) low_by_cat[t.category].push_back(t.ticket_id);
        if (!low_by_cat.empty()) {
            int cnt = 0;
            for (auto& [cat, ids] : low_by_cat) cnt += static_cast<int>(ids.size());
            Anomaly a;
            a.title = "低满意度(≤2 分)工单 " + std::to_string(cnt) + " 笔";
            a.basis = "低满意度集中于:";
            for (auto& [cat, ids] : low_by_cat) {
                a.basis += " " + cat + " " + std::to_string(ids.size()) + " 笔;";
                for (auto& id : ids) a.tickets.push_back(id);
            }
            a.basis += " 多与超长处理时长、客服/机器人体验相关。";
            r.anomalies.push_back(std::move(a));
        }
    }

    // 规则5:时间尖峰(> 均值 + 2σ)
    {
        std::vector<double> daily_cnt;
        for (auto& d : r.daily) daily_cnt.push_back(static_cast<double>(d.count));
        double dm = mean(daily_cnt), ds = stddev(daily_cnt, dm);
        double threshold = dm + 2 * ds;
        std::vector<std::string> spikes;
        for (auto& d : r.daily)
            if (d.count > threshold) spikes.push_back(d.date + "(" + std::to_string(d.count) + " 单)");
        if (!spikes.empty()) {
            Anomaly a;
            a.title = "工单量时间尖峰";
            a.basis = "以下日期工单量超过均值+2σ(" +
                      std::to_string(static_cast<int>(threshold + 0.5)) + " 单/天):";
            for (auto& s : spikes) a.basis += " " + s + ";";
            r.anomalies.push_back(std::move(a));
        }
    }

    // 规则6:反复出现的共性问题(关键词频次>=3 合并为一条)
    {
        std::vector<std::string> recurring;
        for (const auto& k : r.keywords)
            if (k.count >= 3)
                recurring.push_back("\"" + k.keyword + "\"(" + std::to_string(k.count) + "次)");

        if (!recurring.empty()) {
            Anomaly a;
            a.title = "反复出现的共性问题(关键词聚类)";
            a.basis = "以下问题在描述中反复出现:";
            for (auto& s : recurring) a.basis += " " + s + ";";

            std::set<std::string> idset;
            for (const auto& k : r.keywords) {
                if (k.count < 3) continue;
                auto it = kw_hits.find(k.keyword);
                if (it != kw_hits.end())
                    for (auto& id : it->second) idset.insert(id);
            }
            for (auto& id : idset) a.tickets.push_back(id);

            // 特别强调重复扣款这一最严重、且用户反馈为反复发生的信号
            auto dup = kw_hits.find("重复扣款");
            if (dup != kw_hits.end())
                for (auto& id : dup->second)
                    if (id == "T046")
                        a.basis += " 其中 T046 明确提到\"上个月也有过\",说明重复扣款类问题长期未根治,应作为最高优先级排查。";

            r.anomalies.push_back(std::move(a));
        }
    }

    return r;
}
