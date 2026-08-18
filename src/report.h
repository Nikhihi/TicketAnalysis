#pragma once

#include <string>

#include "analyzer.h"

// 生成交互式 HTML Dashboard(单文件,中文,ECharts)
std::string generate_dashboard(const AnalysisResult& r);
