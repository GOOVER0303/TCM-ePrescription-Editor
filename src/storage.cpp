#include "storage.h"

#include "state.h"
#include "text_utils.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <set>
#include <sstream>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace tcm {
namespace {

constexpr std::array<std::string_view, 7> kDataFiles = {
    "doctor.txt",          "herbs.txt",          "dose_limits.txt",
    "common_formulas.txt", "common_formula_items.txt", "prescriptions.txt",
    "prescription_items.txt"};

struct Record {
    std::size_t lineNumber{0};
    std::vector<std::string> fields;
};

std::string fileLabel(const std::filesystem::path& path, std::size_t line) {
    return path.filename().string() + ":" + std::to_string(line);
}

bool readRecords(const std::filesystem::path& path, std::size_t expectedFields,
                 std::vector<Record>& records, std::string& error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "无法打开数据文件：" + path.string();
        return false;
    }

    std::string line;
    std::size_t lineNumber = 0;
    bool firstLine = true;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (firstLine) {
            firstLine = false;
            if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line.erase(0, 3);
            }
        }
        const std::string cleaned = trim(line);
        if (cleaned.empty() || cleaned[0] == '#') {
            continue;
        }
        std::vector<std::string> fields = splitPipe(cleaned);
        if (fields.size() != expectedFields) {
            error = fileLabel(path, lineNumber) + " 字段数应为 " + std::to_string(expectedFields) +
                    "，实际为 " + std::to_string(fields.size()) + "。";
            return false;
        }
        for (auto& field : fields) {
            field = trim(field);
        }
        records.push_back(Record{lineNumber, std::move(fields)});
    }
    if (input.bad()) {
        error = "读取数据文件失败：" + path.string();
        return false;
    }
    return true;
}

std::string describeFileError(const std::filesystem::path& path, const Record& record,
                              const std::string& detail) {
    return fileLabel(path, record.lineNumber) + " " + detail;
}

std::string buildDoctorFile(const AppState& state) {
    std::ostringstream out;
    out << "# 医生编号|姓名|科室\n";
    out << state.currentDoctor.id << '|' << state.currentDoctor.name << '|'
        << state.currentDoctor.department << '\n';
    return out.str();
}

std::string buildHerbsFile(const AppState& state) {
    std::ostringstream out;
    out << "# 中药编号|药物名称|产地|规格\n";
    for (const auto& herb : state.herbs) {
        out << herb.number << '|' << herb.name << '|' << herb.origin << '|' << herb.spec << '\n';
    }
    return out.str();
}

std::string buildDoseLimitsFile(const AppState& state) {
    std::ostringstream out;
    out << "# 药物名称|规格|最大用量(g)\n";
    for (const auto& limit : state.doseLimits) {
        out << limit.herbName << '|' << limit.spec << '|' << formatHundredths(limit.maxHundredths)
            << '\n';
    }
    return out.str();
}

std::string buildFormulaFile(const AppState& state) {
    std::ostringstream out;
    out << "# 序号|医生编号|常用方名称|科室\n";
    for (const auto& formula : state.formulas) {
        out << formula.seq << '|' << formula.doctorId << '|' << formula.name << '|'
            << formula.department << '\n';
    }
    return out.str();
}

std::string buildFormulaItemsFile(const AppState& state) {
    std::ostringstream out;
    out << "# 常用方序号|药物名称|规格|用量(g)\n";
    for (const auto& formula : state.formulas) {
        for (const auto& item : formula.items) {
            out << formula.seq << '|' << item.herbName << '|' << item.spec << '|'
                << formatHundredths(item.doseHundredths) << '\n';
        }
    }
    return out.str();
}

std::string buildPrescriptionsFile(const AppState& state) {
    std::ostringstream out;
    out << "# 处方编号|医生编号|医生姓名|剂数|状态|超量签名|签名时间|创建时间|更新时间\n";
    for (const auto& prescription : state.prescriptions) {
        out << prescription.id << '|' << prescription.doctorId << '|' << prescription.doctorName << '|'
            << prescription.doses << '|' << statusToStorage(prescription.status) << '|'
            << prescription.overdoseSignature << '|' << prescription.overdoseSignedAt << '|'
            << prescription.createdAt << '|' << prescription.updatedAt << '\n';
    }
    return out.str();
}

std::string buildPrescriptionItemsFile(const AppState& state) {
    std::ostringstream out;
    out << "# 处方编号|行号|药物名称|规格|用量(g)\n";
    for (const auto& prescription : state.prescriptions) {
        for (const auto& item : prescription.items) {
            out << prescription.id << '|' << item.lineNo << '|' << item.herbName << '|' << item.spec
                << '|' << formatHundredths(item.doseHundredths) << '\n';
        }
    }
    return out.str();
}

bool writeFile(const std::filesystem::path& path, const std::string& content, std::string& error) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        error = "无法写入文件：" + path.string();
        return false;
    }
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!output) {
        error = "写入文件失败：" + path.string();
        return false;
    }
    return true;
}

bool replaceFile(const std::filesystem::path& temporary, const std::filesystem::path& target,
                 std::string& error) {
#ifdef _WIN32
    if (!MoveFileExW(temporary.wstring().c_str(), target.wstring().c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        error = "替换数据文件失败：" + target.string() + "（Windows 错误 " +
                std::to_string(GetLastError()) + "）";
        return false;
    }
#else
    std::error_code ec;
    std::filesystem::rename(temporary, target, ec);
    if (ec) {
        error = "替换数据文件失败：" + target.string() + "：" + ec.message();
        return false;
    }
#endif
    return true;
}

bool validateForSave(const AppState& state, std::string& error) {
    if (!validateRequiredText(state.currentDoctor.id, "医生编号", error) ||
        !validateRequiredText(state.currentDoctor.name, "医生姓名", error) ||
        !validateRequiredText(state.currentDoctor.department, "医生科室", error)) {
        return false;
    }
    for (const auto& herb : state.herbs) {
        if (!validateRequiredText(herb.number, "中药编号", error) ||
            !validateRequiredText(herb.name, "药物名称", error) ||
            !validateRequiredText(herb.origin, "产地", error) ||
            !validateRequiredText(herb.spec, "规格", error)) {
            return false;
        }
    }
    for (const auto& prescription : state.prescriptions) {
        if (!validateRequiredText(prescription.id, "处方编号", error) ||
            !validateRequiredText(prescription.doctorId, "医生编号", error) ||
            !validateRequiredText(prescription.doctorName, "医生姓名", error)) {
            return false;
        }
    }
    return true;
}

}  // namespace

Storage::Storage(std::filesystem::path dataDir)
    : dataDir_(std::move(dataDir)), seedDir_(dataDir_ / "seed") {}

bool Storage::initialize(std::string& error) {
    std::error_code ec;
    std::filesystem::create_directories(dataDir_, ec);
    if (ec) {
        error = "无法创建数据目录：" + dataDir_.string() + "：" + ec.message();
        return false;
    }
    if (!std::filesystem::exists(seedDir_)) {
        error = "种子数据目录不存在：" + seedDir_.string();
        return false;
    }
    for (const auto name : kDataFiles) {
        const auto seedPath = seedDir_ / std::string(name);
        if (!std::filesystem::exists(seedPath)) {
            error = "缺少种子数据文件：" + seedPath.string();
            return false;
        }
    }

    bool missingActiveFile = false;
    for (const auto name : kDataFiles) {
        if (!std::filesystem::exists(dataDir_ / std::string(name))) {
            missingActiveFile = true;
            break;
        }
    }
    if (missingActiveFile) {
        return copySeedToActive(error);
    }
    return true;
}

bool Storage::load(AppState& state, std::string& error) {
    AppState loaded;
    const std::filesystem::path dataPath = dataDir_;

    std::vector<Record> doctorRecords;
    if (!readRecords(dataPath / "doctor.txt", 3, doctorRecords, error)) {
        return false;
    }
    if (doctorRecords.size() != 1) {
        error = "doctor.txt 必须且只能包含一条当前医生记录。";
        return false;
    }
    loaded.currentDoctor = {doctorRecords[0].fields[0], doctorRecords[0].fields[1],
                            doctorRecords[0].fields[2]};
    if (!validateRequiredText(loaded.currentDoctor.id, "医生编号", error) ||
        !validateRequiredText(loaded.currentDoctor.name, "医生姓名", error) ||
        !validateRequiredText(loaded.currentDoctor.department, "医生科室", error)) {
        error = "doctor.txt " + error;
        return false;
    }

    std::vector<Record> herbRecords;
    if (!readRecords(dataPath / "herbs.txt", 4, herbRecords, error)) {
        return false;
    }
    std::set<std::string> herbNumbers;
    std::set<std::pair<std::string, std::string>> herbKeys;
    for (const auto& record : herbRecords) {
        Herb herb{record.fields[0], record.fields[1], record.fields[2], record.fields[3]};
        if (!validateRequiredText(herb.number, "中药编号", error) ||
            !validateRequiredText(herb.name, "药物名称", error) ||
            !validateRequiredText(herb.origin, "产地", error) ||
            !validateRequiredText(herb.spec, "规格", error)) {
            error = describeFileError(dataPath / "herbs.txt", record, error);
            return false;
        }
        if (!herbNumbers.insert(herb.number).second) {
            error = describeFileError(dataPath / "herbs.txt", record, "中药编号重复：" + herb.number);
            return false;
        }
        if (!herbKeys.emplace(herb.name, herb.spec).second) {
            error = describeFileError(dataPath / "herbs.txt", record,
                                      "药物名称和规格组合重复：" + herb.name + " / " + herb.spec);
            return false;
        }
        loaded.herbs.push_back(std::move(herb));
    }

    std::vector<Record> limitRecords;
    if (!readRecords(dataPath / "dose_limits.txt", 3, limitRecords, error)) {
        return false;
    }
    std::set<std::pair<std::string, std::string>> limitKeys;
    for (const auto& record : limitRecords) {
        DoseLimit limit{record.fields[0], record.fields[1], 0};
        if (!parseDoseHundredths(record.fields[2], limit.maxHundredths) || limit.maxHundredths <= 0) {
            error = describeFileError(dataPath / "dose_limits.txt", record,
                                      "最大用量必须是大于 0 的克数，最多两位小数。");
            return false;
        }
        if (!limitKeys.emplace(limit.herbName, limit.spec).second) {
            error = describeFileError(dataPath / "dose_limits.txt", record,
                                      "药量控制键重复：" + limit.herbName + " / " + limit.spec);
            return false;
        }
        if (findHerb(loaded, limit.herbName, limit.spec) == nullptr) {
            error = describeFileError(dataPath / "dose_limits.txt", record,
                                      "引用的中药不存在：" + limit.herbName + " / " + limit.spec);
            return false;
        }
        loaded.doseLimits.push_back(std::move(limit));
    }

    std::vector<Record> formulaRecords;
    if (!readRecords(dataPath / "common_formulas.txt", 4, formulaRecords, error)) {
        return false;
    }
    std::set<int> formulaSeqs;
    for (const auto& record : formulaRecords) {
        CommonFormula formula;
        if (!parseIntStrict(record.fields[0], formula.seq) || formula.seq <= 0) {
            error = describeFileError(dataPath / "common_formulas.txt", record,
                                      "常用方序号必须是正整数。");
            return false;
        }
        if (!formulaSeqs.insert(formula.seq).second) {
            error = describeFileError(dataPath / "common_formulas.txt", record,
                                      "常用方序号重复：" + record.fields[0]);
            return false;
        }
        formula.doctorId = record.fields[1];
        formula.name = record.fields[2];
        formula.department = record.fields[3];
        loaded.formulas.push_back(std::move(formula));
    }

    std::vector<Record> formulaItemRecords;
    if (!readRecords(dataPath / "common_formula_items.txt", 4, formulaItemRecords, error)) {
        return false;
    }
    std::set<std::pair<int, std::pair<std::string, std::string>>> formulaItemKeys;
    for (const auto& record : formulaItemRecords) {
        int formulaSeq = 0;
        if (!parseIntStrict(record.fields[0], formulaSeq)) {
            error = describeFileError(dataPath / "common_formula_items.txt", record,
                                      "常用方序号无效。");
            return false;
        }
        CommonFormula* formula = findFormula(loaded, formulaSeq);
        if (formula == nullptr) {
            error = describeFileError(dataPath / "common_formula_items.txt", record,
                                      "引用的常用方不存在：" + record.fields[0]);
            return false;
        }
        FormulaItem item{record.fields[1], record.fields[2], 0};
        if (!parseDoseHundredths(record.fields[3], item.doseHundredths) ||
            item.doseHundredths <= 0) {
            error = describeFileError(dataPath / "common_formula_items.txt", record,
                                      "用量必须是大于 0 的克数，最多两位小数。");
            return false;
        }
        if (!formulaItemKeys.emplace(formulaSeq, std::make_pair(item.herbName, item.spec)).second) {
            error = describeFileError(dataPath / "common_formula_items.txt", record,
                                      "同一常用方内药物重复：" + item.herbName + " / " + item.spec);
            return false;
        }
        formula->items.push_back(std::move(item));
    }

    for (const auto& formula : loaded.formulas) {
        if (!validateFormulaRecord(loaded, formula, error)) {
            error = "common_formulas.txt / common_formula_items.txt：" + error;
            return false;
        }
    }

    std::vector<Record> prescriptionRecords;
    if (!readRecords(dataPath / "prescriptions.txt", 9, prescriptionRecords, error)) {
        return false;
    }
    std::set<std::string> prescriptionIds;
    int maxPrescriptionSeq = 0;
    for (const auto& record : prescriptionRecords) {
        Prescription prescription;
        prescription.id = record.fields[0];
        prescription.doctorId = record.fields[1];
        prescription.doctorName = record.fields[2];
        if (!parseIntStrict(record.fields[3], prescription.doses) || prescription.doses < 1) {
            error = describeFileError(dataPath / "prescriptions.txt", record,
                                      "剂数必须是正整数。");
            return false;
        }
        if (!statusFromText(record.fields[4], prescription.status)) {
            error = describeFileError(dataPath / "prescriptions.txt", record,
                                      "处方状态无效：" + record.fields[4]);
            return false;
        }
        prescription.overdoseSignature = record.fields[5];
        prescription.overdoseSignedAt = record.fields[6];
        prescription.createdAt = record.fields[7];
        prescription.updatedAt = record.fields[8];
        if (!prescriptionIds.insert(prescription.id).second) {
            error = describeFileError(dataPath / "prescriptions.txt", record,
                                      "处方编号重复：" + prescription.id);
            return false;
        }
        if (prescription.id.size() < 2 || prescription.id[0] != 'P') {
            error = describeFileError(dataPath / "prescriptions.txt", record,
                                      "处方编号必须为 P 加数字。");
            return false;
        }
        int idNumber = 0;
        if (!parseIntStrict(prescription.id.substr(1), idNumber) || idNumber <= 0) {
            error = describeFileError(dataPath / "prescriptions.txt", record,
                                      "处方编号必须为 P 加正数字。");
            return false;
        }
        maxPrescriptionSeq = std::max(maxPrescriptionSeq, idNumber);
        loaded.prescriptions.push_back(std::move(prescription));
    }

    std::vector<Record> prescriptionItemRecords;
    if (!readRecords(dataPath / "prescription_items.txt", 5, prescriptionItemRecords, error)) {
        return false;
    }
    for (const auto& record : prescriptionItemRecords) {
        Prescription* prescription = findPrescription(loaded, record.fields[0]);
        if (prescription == nullptr) {
            error = describeFileError(dataPath / "prescription_items.txt", record,
                                      "引用的处方不存在：" + record.fields[0]);
            return false;
        }
        int lineNo = 0;
        if (!parseIntStrict(record.fields[1], lineNo) ||
            lineNo != static_cast<int>(prescription->items.size() + 1)) {
            error = describeFileError(dataPath / "prescription_items.txt", record,
                                      "处方明细行号必须从 1 开始连续递增。");
            return false;
        }
        PrescriptionItem item{lineNo, record.fields[2], record.fields[3], 0};
        if (!parseDoseHundredths(record.fields[4], item.doseHundredths) ||
            item.doseHundredths <= 0) {
            error = describeFileError(dataPath / "prescription_items.txt", record,
                                      "用量必须是大于 0 的克数，最多两位小数。");
            return false;
        }
        prescription->items.push_back(std::move(item));
    }

    for (const auto& prescription : loaded.prescriptions) {
        if (!validatePrescriptionRecord(loaded, prescription, error)) {
            error = "prescriptions.txt / prescription_items.txt：" + error;
            return false;
        }
        if (prescription.status != PrescriptionStatus::Draft && prescription.items.empty()) {
            error = "处方 " + prescription.id + " 不是草稿，但没有任何药物明细。";
            return false;
        }
    }

    int maxFormulaSeq = 0;
    for (const auto& formula : loaded.formulas) {
        maxFormulaSeq = std::max(maxFormulaSeq, formula.seq);
    }
    loaded.nextFormulaSeq = maxFormulaSeq + 1;
    loaded.nextPrescriptionSeq = maxPrescriptionSeq + 1;
    state = std::move(loaded);
    return true;
}

bool Storage::save(const AppState& state, std::string& error) {
    return writeAllFiles(state, error);
}

bool Storage::resetToSeed(std::string& error) {
    return copySeedToActive(error);
}

bool Storage::copySeedToActive(std::string& error) {
    std::error_code ec;
    std::filesystem::create_directories(dataDir_, ec);
    if (ec) {
        error = "无法创建数据目录：" + dataDir_.string() + "：" + ec.message();
        return false;
    }
    for (const auto name : kDataFiles) {
        const auto source = seedDir_ / std::string(name);
        const auto target = dataDir_ / std::string(name);
        if (!std::filesystem::exists(source)) {
            error = "缺少种子数据文件：" + source.string();
            return false;
        }
        std::filesystem::copy_file(source, target, std::filesystem::copy_options::overwrite_existing,
                                   ec);
        if (ec) {
            error = "恢复种子数据失败：" + target.string() + "：" + ec.message();
            return false;
        }
    }
    return true;
}

bool Storage::writeAllFiles(const AppState& state, std::string& error) {
    if (!validateForSave(state, error)) {
        return false;
    }

    const std::array<std::pair<std::string, std::string>, 7> contents = {
        std::make_pair(std::string(kDataFiles[0]), buildDoctorFile(state)),
        std::make_pair(std::string(kDataFiles[1]), buildHerbsFile(state)),
        std::make_pair(std::string(kDataFiles[2]), buildDoseLimitsFile(state)),
        std::make_pair(std::string(kDataFiles[3]), buildFormulaFile(state)),
        std::make_pair(std::string(kDataFiles[4]), buildFormulaItemsFile(state)),
        std::make_pair(std::string(kDataFiles[5]), buildPrescriptionsFile(state)),
        std::make_pair(std::string(kDataFiles[6]), buildPrescriptionItemsFile(state))};

    std::vector<std::filesystem::path> tempFiles;
    for (const auto& entry : contents) {
        const auto target = dataDir_ / entry.first;
        const auto temporary = dataDir_ / (entry.first + ".tmp");
        if (!writeFile(temporary, entry.second, error)) {
            for (const auto& path : tempFiles) {
                std::error_code ignored;
                std::filesystem::remove(path, ignored);
            }
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            return false;
        }
        tempFiles.push_back(temporary);
    }

    for (std::size_t i = 0; i < contents.size(); ++i) {
        const auto target = dataDir_ / contents[i].first;
        if (!replaceFile(tempFiles[i], target, error)) {
            for (std::size_t j = i; j < tempFiles.size(); ++j) {
                std::error_code ignored;
                std::filesystem::remove(tempFiles[j], ignored);
            }
            return false;
        }
    }
    return true;
}

}  // namespace tcm
