#pragma once

#include <string>
#include <vector>

namespace tcm {

std::string trim(const std::string& value);
std::vector<std::string> splitPipe(const std::string& line);
bool containsInvalidFieldChar(const std::string& value);
bool parseIntStrict(const std::string& text, int& value);
bool parseDoseHundredths(const std::string& text, long long& hundredths);
std::string formatHundredths(long long hundredths);
std::string currentTimestamp();
std::string makePrescriptionId(int sequence);
std::string makeFormulaCode(int sequence);
std::string joinWithPipe(const std::vector<std::string>& fields);

}  // namespace tcm
