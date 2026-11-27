#include "text_utils.h"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <limits>
#include <sstream>

namespace tcm {
namespace {

bool isSpaceByte(unsigned char ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

}  // namespace

std::string trim(const std::string& value) {
    std::size_t begin = 0;
    std::size_t end = value.size();
    const std::string fullWidthSpace = "\xE3\x80\x80";

    while (begin < end && isSpaceByte(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }
    while (end > begin && isSpaceByte(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }

    while (end - begin >= fullWidthSpace.size() &&
           value.compare(begin, fullWidthSpace.size(), fullWidthSpace) == 0) {
        begin += fullWidthSpace.size();
    }
    while (end - begin >= fullWidthSpace.size() &&
           value.compare(end - fullWidthSpace.size(), fullWidthSpace.size(), fullWidthSpace) == 0) {
        end -= fullWidthSpace.size();
    }
    return value.substr(begin, end - begin);
}

std::vector<std::string> splitPipe(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (true) {
        const std::size_t pos = line.find('|', start);
        if (pos == std::string::npos) {
            fields.push_back(line.substr(start));
            break;
        }
        fields.push_back(line.substr(start, pos - start));
        start = pos + 1;
    }
    return fields;
}

bool containsInvalidFieldChar(const std::string& value) {
    return value.find('|') != std::string::npos || value.find('\n') != std::string::npos ||
           value.find('\r') != std::string::npos;
}

bool parseIntStrict(const std::string& text, int& value) {
    const std::string cleaned = trim(text);
    if (cleaned.empty()) {
        return false;
    }
    int parsed = 0;
    const auto* begin = cleaned.data();
    const auto* end = cleaned.data() + cleaned.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end) {
        return false;
    }
    value = parsed;
    return true;
}

bool parseDoseHundredths(const std::string& text, long long& hundredths) {
    const std::string cleaned = trim(text);
    if (cleaned.empty() || cleaned[0] == '-' || cleaned[0] == '+') {
        return false;
    }

    const std::size_t dot = cleaned.find('.');
    if (dot != std::string::npos && cleaned.find('.', dot + 1) != std::string::npos) {
        return false;
    }

    const std::string wholeText = dot == std::string::npos ? cleaned : cleaned.substr(0, dot);
    const std::string fractionText = dot == std::string::npos ? std::string{} : cleaned.substr(dot + 1);
    if (wholeText.empty() || fractionText.size() > 2) {
        return false;
    }
    if (!std::all_of(wholeText.begin(), wholeText.end(), [](unsigned char ch) { return std::isdigit(ch); })) {
        return false;
    }
    if (!std::all_of(fractionText.begin(), fractionText.end(), [](unsigned char ch) { return std::isdigit(ch); })) {
        return false;
    }

    long long whole = 0;
    const auto wholeResult = std::from_chars(wholeText.data(), wholeText.data() + wholeText.size(), whole);
    if (wholeResult.ec != std::errc{} || wholeResult.ptr != wholeText.data() + wholeText.size()) {
        return false;
    }

    long long fraction = 0;
    if (!fractionText.empty()) {
        const auto fractionResult = std::from_chars(
            fractionText.data(), fractionText.data() + fractionText.size(), fraction);
        if (fractionResult.ec != std::errc{} ||
            fractionResult.ptr != fractionText.data() + fractionText.size()) {
            return false;
        }
        if (fractionText.size() == 1) {
            fraction *= 10;
        }
    }

    if (whole > (std::numeric_limits<long long>::max() - fraction) / 100) {
        return false;
    }
    hundredths = whole * 100 + fraction;
    return true;
}

std::string formatHundredths(long long hundredths) {
    std::ostringstream out;
    const long long whole = hundredths / 100;
    const long long fraction = hundredths % 100;
    out << whole << '.' << std::setw(2) << std::setfill('0') << fraction;
    return out.str();
}

std::string currentTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &time);
#else
    localtime_r(&time, &local);
#endif
    std::ostringstream out;
    out << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

std::string makePrescriptionId(int sequence) {
    std::ostringstream out;
    out << 'P' << std::setw(4) << std::setfill('0') << sequence;
    return out.str();
}

std::string makeFormulaCode(int sequence) {
    std::ostringstream out;
    out << "CF" << std::setw(4) << std::setfill('0') << sequence;
    return out.str();
}

std::string joinWithPipe(const std::vector<std::string>& fields) {
    std::ostringstream out;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i != 0) {
            out << '|';
        }
        out << fields[i];
    }
    return out.str();
}

}  // namespace tcm
