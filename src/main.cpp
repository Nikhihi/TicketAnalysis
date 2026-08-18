#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "analyzer.h"
#include "report.h"
#include "ticket.h"

int main() {
    const std::string data_path = "task5_tickets.json";
    const std::string out_path = "output/dashboard.html";

    auto tickets_opt = load_tickets(data_path);
    if (!tickets_opt) {
        std::cerr << "分析失败:无法加载工单数据" << std::endl;
        return 1;
    }

    AnalysisResult result = analyze(*tickets_opt);
    std::string html = generate_dashboard(result);

    std::error_code ec;
    std::filesystem::create_directories("output", ec);

    std::ofstream out(out_path, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "无法写入输出文件: " << out_path << std::endl;
        return 1;
    }
    out << html;
    out.close();

    std::cout << "分析完成:" << std::endl;
    std::cout << "  工单总数: " << result.kpi.total << std::endl;
    std::cout << "  未解决:   " << result.kpi.unresolved << std::endl;
    std::cout << "  异常信号: " << result.anomalies.size() << " 条" << std::endl;
    std::cout << "Dashboard 已生成: " << out_path << std::endl;
    return 0;
}
