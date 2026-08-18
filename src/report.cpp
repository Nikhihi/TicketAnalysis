#include "report.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "../third_party/json.hpp"

using json = nlohmann::json;

namespace {

std::string f2(double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f", v);
    return buf;
}

std::string f1(double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.1f", v);
    return buf;
}

// HTML 转义(表格展示用)
std::string html_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += c;
        }
    }
    return out;
}

// 箱线图五点统计 [min, q1, median, q3, max]
double percentile(std::vector<double> v, double p) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    double idx = p * (v.size() - 1);
    int lo = static_cast<int>(idx);
    int hi = std::min(lo + 1, static_cast<int>(v.size()) - 1);
    double frac = idx - lo;
    return v[lo] * (1.0 - frac) + v[hi] * frac;
}

json boxplot_item(const std::vector<double>& v) {
    json arr = json::array();
    if (!v.empty()) {
        arr.push_back(v.empty() ? 0 : *std::min_element(v.begin(), v.end()));
        arr.push_back(percentile(v, 0.25));
        arr.push_back(percentile(v, 0.50));
        arr.push_back(percentile(v, 0.75));
        arr.push_back(*std::max_element(v.begin(), v.end()));
    }
    return arr;
}

}  // namespace

std::string generate_dashboard(const AnalysisResult& r) {
    std::ostringstream html;

    // ---- 各图表数据 ----
    json trend_dates = json::array(), trend_count = json::array(),
         trend_unresolved = json::array();
    for (const auto& d : r.daily) {
        trend_dates.push_back(d.date);
        trend_count.push_back(d.count);
        trend_unresolved.push_back(d.unresolved);
    }

    json pie_data = json::array();
    for (const auto& c : r.categories)
        pie_data.push_back({{"name", c.name}, {"value", c.count}});

    json pri_names = json::array(), pri_count = json::array(),
         pri_sat = json::array();
    for (const auto& p : r.priorities) {
        pri_names.push_back(p.name);
        pri_count.push_back(p.count);
        pri_sat.push_back(std::round(p.avg_satisfaction * 100.0) / 100.0);
    }

    json box_names = json::array(), box_data = json::array();
    for (const auto& c : r.categories) {
        box_names.push_back(c.name);
        box_data.push_back(boxplot_item(c.resolution_values));
    }

    json sat_labels = json::array({"1分", "2分", "3分", "4分", "5分"});
    json sat_values = json::array();
    for (int v : r.satisfaction_dist) sat_values.push_back(v);

    json chn_names = json::array(), chn_count = json::array(),
         chn_sat = json::array(), chn_hours = json::array();
    for (const auto& c : r.channels) {
        chn_names.push_back(c.name);
        chn_count.push_back(c.count);
        chn_sat.push_back(std::round(c.avg_satisfaction * 100.0) / 100.0);
        chn_hours.push_back(std::round(c.avg_resolution * 100.0) / 100.0);
    }

    json scatter_data = json::array();
    for (auto& [h, s] : r.scatter)
        scatter_data.push_back(json::array({h, s}));

    // ---- 头部 + 样式 ----
    html << R"(<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>客服工单分析 Dashboard</title>
<script src="https://cdn.jsdelivr.net/npm/echarts@5.5.0/dist/echarts.min.js"></script>
<style>
  :root { --primary:#409EFF; --danger:#F56C6C; --warn:#E6A23C; --ok:#67C23A; --bg:#f5f7fa; }
  * { box-sizing:border-box; margin:0; padding:0; }
  body { font-family:"Microsoft YaHei","PingFang SC",sans-serif; background:var(--bg); color:#303133; }
  .wrap { max-width:1200px; margin:0 auto; padding:24px 16px; }
  h1 { font-size:24px; margin-bottom:6px; }
  .sub { color:#909399; font-size:13px; margin-bottom:20px; }
  .kpi { display:grid; grid-template-columns:repeat(4,1fr); gap:16px; margin-bottom:20px; }
  .card { background:#fff; border-radius:10px; padding:18px; box-shadow:0 1px 4px rgba(0,0,0,.06); }
  .card .num { font-size:30px; font-weight:700; }
  .card .lbl { color:#909399; font-size:13px; margin-top:4px; }
  .c-blue{color:var(--primary)} .c-red{color:var(--danger)} .c-orange{color:var(--warn)} .c-green{color:var(--ok)}
  h2 { font-size:17px; margin:28px 0 12px; padding-left:10px; border-left:4px solid var(--primary); }
  .grid { display:grid; grid-template-columns:1fr 1fr; gap:16px; }
  .panel { background:#fff; border-radius:10px; padding:12px; box-shadow:0 1px 4px rgba(0,0,0,.06); }
  .chart { width:100%; height:320px; }
  .anomaly .item { background:#fff; border-left:4px solid var(--danger); border-radius:8px; padding:14px 16px; margin-bottom:12px; box-shadow:0 1px 4px rgba(0,0,0,.06); }
  .anomaly .t { font-weight:700; margin-bottom:6px; color:#c0392b; }
  .anomaly .b { font-size:13px; color:#606266; line-height:1.6; }
  .anomaly .chips { margin-top:8px; }
  .chip { display:inline-block; background:#fef0f0; color:#c0392b; border-radius:4px; padding:2px 8px; font-size:12px; margin:2px 4px 0 0; }
  .filters { margin-bottom:12px; }
  .filters select { padding:6px 10px; border:1px solid #dcdfe6; border-radius:6px; margin-right:8px; font-size:13px; }
  table { width:100%; border-collapse:collapse; background:#fff; font-size:13px; box-shadow:0 1px 4px rgba(0,0,0,.06); }
  th,td { padding:8px 10px; border-bottom:1px solid #ebeef5; text-align:left; vertical-align:top; }
  th { background:#f5f7fa; font-weight:600; white-space:nowrap; }
  tr.anomaly td { background:#fef0f0; }
  .tag { display:inline-block; padding:1px 7px; border-radius:10px; font-size:12px; }
  .p-high{background:#fef0f0;color:#c0392b} .p-mid{background:#fdf6ec;color:#b88230} .p-low{background:#f0f9eb;color:#529b2e}
  .ok{color:var(--ok)} .bad{color:var(--danger)}
  @media (max-width:800px){ .kpi{grid-template-columns:1fr 1fr;} .grid{grid-template-columns:1fr;} }
</style>
</head>
<body>
<div class="wrap">
  <h1>客服工单分析 Dashboard</h1>
  <div class="sub">数据范围:2024-06-01 ~ 2024-06-11 · 共 )"
        << r.kpi.total << R"( 条工单 · 自动异常检测</div>

  <div class="kpi">
    <div class="card"><div class="num c-blue">)" << r.kpi.total << R"(</div><div class="lbl">工单总数</div></div>
    <div class="card"><div class="num c-red">)" << r.kpi.unresolved << R"(</div><div class="lbl">未解决</div></div>
    <div class="card"><div class="num c-orange">)" << f2(r.kpi.avg_satisfaction) << R"(</div><div class="lbl">平均满意度</div></div>
    <div class="card"><div class="num c-green">)" << f1(r.kpi.avg_resolution_hours) << R"(h</div><div class="lbl">平均处理时长</div></div>
  </div>

  <h2>时间趋势</h2>
  <div class="panel"><div id="chart_trend" class="chart"></div></div>

  <h2>类型 & 优先级分布</h2>
  <div class="grid">
    <div class="panel"><div id="chart_category" class="chart"></div></div>
    <div class="panel"><div id="chart_priority" class="chart"></div></div>
  </div>

  <h2>处理时长 & 满意度</h2>
  <div class="grid">
    <div class="panel"><div id="chart_boxplot" class="chart"></div></div>
    <div class="panel"><div id="chart_satisfaction" class="chart"></div></div>
  </div>

  <h2>渠道对比 & 关联分析</h2>
  <div class="grid">
    <div class="panel"><div id="chart_channel" class="chart"></div></div>
    <div class="panel"><div id="chart_scatter" class="chart"></div></div>
  </div>

  <h2>异常告警</h2>
  <div class="anomaly">)";

    for (const auto& a : r.anomalies) {
        html << "<div class=\"item\"><div class=\"t\">⚠ " << html_escape(a.title)
             << "</div><div class=\"b\">" << html_escape(a.basis) << "</div><div class=\"chips\">";
        const size_t max_chips = 15;
        for (size_t i = 0; i < a.tickets.size() && i < max_chips; ++i)
            html << "<span class=\"chip\">" << html_escape(a.tickets[i]) << "</span>";
        if (a.tickets.size() > max_chips)
            html << "<span class=\"chip\">…等 " << a.tickets.size() << " 条</span>";
        html << "</div></div>";
    }
    if (r.anomalies.empty()) {
        html << "<div class=\"item\"><div class=\"t\">未检测到显著异常</div>"
                "<div class=\"b\">当前数据未触发异常阈值。</div></div>";
    }

    html << R"(  </div>

  <h2>工单明细</h2>
  <div class="filters">
    <select id="f-cat" onchange='applyFilter()'><option value="all">全部分类</option>)";
    for (const auto& c : r.categories)
        html << "<option value=\"" << html_escape(c.name) << "\">" << html_escape(c.name) << "</option>";
    html << R"(</select>
    <select id="f-pri" onchange='applyFilter()'><option value="all">全部优先级</option>)";
    for (const auto& p : r.priorities)
        html << "<option value=\"" << html_escape(p.name) << "\">" << html_escape(p.name) << "</option>";
    html << R"(</select>
    <select id="f-chn" onchange='applyFilter()'><option value="all">全部渠道</option>)";
    for (const auto& c : r.channels)
        html << "<option value=\"" << html_escape(c.name) << "\">" << html_escape(c.name) << "</option>";
    html << R"(</select>
    <select id="f-res" onchange='applyFilter()'><option value="all">全部状态</option><option value="1">已解决</option><option value="0">未解决</option></select>
  </div>
  <div style="overflow-x:auto">
  <table id="ticket-table">
    <thead><tr><th>工单</th><th>时间</th><th>分类</th><th>描述</th><th>优先级</th><th>时长(h)</th><th>满意度</th><th>渠道</th><th>状态</th></tr></thead>
    <tbody>)";

    for (const auto& t : r.tickets) {
        // 高亮:未解决 或 超长处理(>48h)或 极低满意度(<=1 分)
        bool is_anom = !t.is_resolved || t.resolution_time_hours > 48.0 ||
                       t.satisfaction <= 1;
        std::string pclass = t.priority == "高" ? "p-high" : (t.priority == "中" ? "p-mid" : "p-low");
        html << "<tr" << (is_anom ? " class=\"anomaly\"" : "") << " data-category=\""
             << html_escape(t.category) << "\" data-priority=\"" << html_escape(t.priority)
             << "\" data-channel=\"" << html_escape(t.channel) << "\" data-resolved=\""
             << (t.is_resolved ? "1" : "0") << "\">";
        html << "<td>" << html_escape(t.ticket_id) << "</td>";
        html << "<td style=\"white-space:nowrap\">" << html_escape(t.created_at) << "</td>";
        html << "<td>" << html_escape(t.category) << "</td>";
        html << "<td>" << html_escape(t.description) << "</td>";
        html << "<td><span class=\"tag " << pclass << "\">" << html_escape(t.priority) << "</span></td>";
        html << "<td>" << f1(t.resolution_time_hours) << "</td>";
        html << "<td>" << t.satisfaction << "</td>";
        html << "<td>" << html_escape(t.channel) << "</td>";
        html << "<td class=\"" << (t.is_resolved ? "ok" : "bad") << "\">"
             << (t.is_resolved ? "已解决" : "未解决") << "</td>";
        html << "</tr>";
    }

    html << R"(    </tbody>
  </table>
  </div>
  <div class="sub" style="margin-top:16px">生成时间:自动 · 异常工单以红色高亮</div>
</div>

<script>
function applyFilter(){
  const cat=document.getElementById('f-cat').value;
  const pri=document.getElementById('f-pri').value;
  const chn=document.getElementById('f-chn').value;
  const res=document.getElementById('f-res').value;
  document.querySelectorAll('#ticket-table tbody tr').forEach(tr=>{
    const ok=(cat==='all'||tr.dataset.category===cat)&&
             (pri==='all'||tr.dataset.priority===pri)&&
             (chn==='all'||tr.dataset.channel===chn)&&
             (res==='all'||tr.dataset.resolved===res);
    tr.style.display=ok?'':'none';
  });
}
</script>
)";

    // ---- ECharts 初始化脚本 ----
    html << R"(<script>
const charts=[];
function mount(id,option){ const el=document.getElementById(id); const c=echarts.init(el); c.setOption(option); charts.push(c); }
window.addEventListener('resize',()=>charts.forEach(c=>c.resize()));

mount('chart_trend',{
  tooltip:{trigger:'axis'},
  legend:{data:['工单总量','未解决']},
  grid:{left:40,right:20,top:40,bottom:30},
  xAxis:{type:'category',data:)" << trend_dates.dump() << R"(},
  yAxis:{type:'value'},
  series:[
    {name:'工单总量',type:'bar',data:)" << trend_count.dump() << R"(,itemStyle:{color:'#409EFF'},barWidth:'50%'},
    {name:'未解决',type:'line',data:)" << trend_unresolved.dump() << R"(,itemStyle:{color:'#F56C6C'},smooth:true}
  ]
});

mount('chart_category',{
  tooltip:{trigger:'item',formatter:'{b}: {c} 条 ({d}%)'},
  legend:{bottom:0},
  series:[{type:'pie',radius:['42%','68%'],center:['50%','44%'],
    itemStyle:{borderRadius:6,borderColor:'#fff',borderWidth:2},
    label:{formatter:'{b}\n{c}条'},
    data:)" << pie_data.dump() << R"(}]
});

mount('chart_priority',{
  tooltip:{trigger:'axis'},
  legend:{data:['工单量','平均满意度']},
  grid:{left:40,right:50,top:40,bottom:30},
  xAxis:{type:'category',data:)" << pri_names.dump() << R"(},
  yAxis:[{type:'value',name:'工单量'},{type:'value',name:'平均满意度',min:1,max:5}],
  series:[
    {name:'工单量',type:'bar',data:)" << pri_count.dump() << R"(,itemStyle:{color:'#409EFF'},barWidth:'45%'},
    {name:'平均满意度',type:'line',yAxisIndex:1,data:)" << pri_sat.dump() << R"(,itemStyle:{color:'#E6A23C'},smooth:true}
  ]
});

mount('chart_boxplot',{
  tooltip:{trigger:'item',formatter:function(p){return p.name+'<br>最短:'+p.value[0]+'h · Q1:'+p.value[1]+'h · 中位:'+p.value[2]+'h · Q3:'+p.value[3]+'h · 最长:'+p.value[4]+'h';}},
  grid:{left:50,right:20,top:30,bottom:40},
  xAxis:{type:'category',data:)" << box_names.dump() << R"(,axisLabel:{interval:0,rotate:20}},
  yAxis:{type:'value',name:'处理时长(小时)'},
  series:[{type:'boxplot',data:)" << box_data.dump() << R"(,itemStyle:{color:'#909399',borderColor:'#606266'}}]
});

mount('chart_satisfaction',{
  tooltip:{trigger:'axis'},
  grid:{left:40,right:20,top:30,bottom:30},
  xAxis:{type:'category',data:)" << sat_labels.dump() << R"(},
  yAxis:{type:'value',name:'工单量'},
  series:[{type:'bar',data:)" << sat_values.dump() << R"(,itemStyle:{color:'#E6A23C'},barWidth:'55%'}]
});

mount('chart_channel',{
  tooltip:{trigger:'axis'},
  legend:{data:['工单量','平均满意度']},
  grid:{left:40,right:50,top:40,bottom:30},
  xAxis:{type:'category',data:)" << chn_names.dump() << R"(},
  yAxis:[{type:'value',name:'工单量'},{type:'value',name:'平均满意度',min:1,max:5}],
  series:[
    {name:'工单量',type:'bar',data:)" << chn_count.dump() << R"(,itemStyle:{color:'#67C23A'},barWidth:'45%'},
    {name:'平均满意度',type:'line',yAxisIndex:1,data:)" << chn_sat.dump() << R"(,itemStyle:{color:'#E6A23C'},smooth:true}
  ]
});

mount('chart_scatter',{
  tooltip:{trigger:'item',formatter:function(p){return '处理时长:'+p.value[0]+'h · 满意度:'+p.value[1]+'分';}},
  grid:{left:50,right:30,top:40,bottom:40},
  xAxis:{type:'value',name:'处理时长(小时)'},
  yAxis:{type:'value',name:'满意度',min:0,max:5.5},
  series:[{type:'scatter',data:)" << scatter_data.dump() << R"(,itemStyle:{color:'#909399',opacity:.6},symbolSize:10}]
});
</script>
</body>
</html>)";

    return html.str();
}
