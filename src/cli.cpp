#include "cli.h"

#include "commands.h"
#include "state.h"
#include "text_utils.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace tcm {
namespace {

std::string itemKey(const std::string& name, const std::string& spec) {
    return name + "|" + spec;
}

const Herb* findHerbByNumber(const AppState& state, const std::string& number) {
    const auto it = std::find_if(state.herbs.begin(), state.herbs.end(),
                                 [&](const Herb& herb) { return herb.number == number; });
    return it == state.herbs.end() ? nullptr : &*it;
}

}  // namespace

Cli::Cli(AppState& state, Storage& storage, CommandManager& commands)
    : state_(state), storage_(storage), commands_(commands) {}

int Cli::run() {
    while (true) {
        printHeader();
        printMainMenu();
        int choice = -1;
        if (!promptInt("请选择", 0, 8, choice)) {
            std::cout << "\n输入结束，程序退出。\n";
            return 0;
        }
        switch (choice) {
            case 1:
                commonFormulaMenu();
                break;
            case 2:
                prescriptionMenu();
                break;
            case 3:
                herbMenu();
                break;
            case 4:
                doseLimitMenu();
                break;
            case 5:
                historyMenu();
                break;
            case 6:
                systemMenu();
                break;
            case 7:
                if (commands_.undoDescriptions().empty()) {
                    std::cout << "没有可以撤销的操作。\n";
                } else {
                    const std::string description = commands_.undoDescriptions().front();
                    std::string error;
                    if (commands_.undo(state_, error)) {
                        std::cout << "已撤销：" << description << "\n";
                    } else {
                        std::cout << "撤销失败：" << error << "\n";
                    }
                }
                break;
            case 8:
                if (commands_.redoDescriptions().empty()) {
                    std::cout << "没有可以重做的操作。\n";
                } else {
                    const std::string description = commands_.redoDescriptions().front();
                    std::string error;
                    if (commands_.redo(state_, error)) {
                        std::cout << "已重做：" << description << "\n";
                    } else {
                        std::cout << "重做失败：" << error << "\n";
                    }
                }
                break;
            case 0:
                std::cout << "\n感谢使用，再见。\n";
                return 0;
            default:
                break;
        }
    }
}

void Cli::printHeader() const {
    std::cout << "\n============================================================\n";
    std::cout << "        中医电子处方编辑系统（C++17 课程演示）\n";
    std::cout << "============================================================\n";
    std::cout << "当前医生：" << state_.currentDoctor.name << "  "
              << state_.currentDoctor.department << "  [编号 " << state_.currentDoctor.id
              << "]\n";
    std::cout << "Undo：" << commands_.undoSize() << " 步    Redo：" << commands_.redoSize()
              << " 步\n";
    std::cout << "提示：本系统数据仅用于数据结构课程演示，不作为临床用药依据。\n";
}

void Cli::printMainMenu() const {
    std::cout << "\n1. 常用方管理\n";
    std::cout << "2. 处方管理\n";
    std::cout << "3. 中药信息管理\n";
    std::cout << "4. 药量控制\n";
    std::cout << "5. 撤销/重做与历史\n";
    std::cout << "6. 系统与演示数据\n";
    std::cout << "7. 快速撤销上一步\n";
    std::cout << "8. 快速重做上一步\n";
    std::cout << "0. 退出\n";
}

void Cli::commonFormulaMenu() {
    while (true) {
        std::cout << "\n------------ 常用方管理 ------------\n";
        listFormulas();
        std::cout << "1. 新增常用方\n";
        std::cout << "2. 修改常用方\n";
        std::cout << "3. 删除常用方\n";
        std::cout << "4. 查看常用方详情\n";
        std::cout << "5. 应用到草稿处方\n";
        std::cout << "0. 返回\n";
        int choice = -1;
        if (!promptInt("请选择", 0, 5, choice)) {
            return;
        }
        switch (choice) {
            case 1:
                addFormula();
                break;
            case 2:
                modifyFormula();
                break;
            case 3:
                deleteFormula();
                break;
            case 4: {
                int seq = 0;
                if (chooseFormula(seq)) {
                    showFormula(seq);
                }
                break;
            }
            case 5:
                applyFormulaToPrescription();
                break;
            case 0:
                return;
        }
    }
}

void Cli::prescriptionMenu() {
    while (true) {
        std::cout << "\n------------ 处方管理 ------------\n";
        listPrescriptions();
        std::cout << "1. 新增处方草稿\n";
        std::cout << "2. 修改草稿处方\n";
        std::cout << "3. 查看处方详情\n";
        std::cout << "4. 提交处方\n";
        std::cout << "5. 撤销已提交处方（业务撤销）\n";
        std::cout << "6. 恢复已撤销处方为草稿\n";
        std::cout << "7. 删除草稿处方\n";
        std::cout << "0. 返回\n";
        int choice = -1;
        if (!promptInt("请选择", 0, 7, choice)) {
            return;
        }
        switch (choice) {
            case 1:
                createPrescription();
                break;
            case 2:
                editDraftPrescription();
                break;
            case 3: {
                std::string id;
                if (promptText("处方编号（空取消）", id, true) && !id.empty()) {
                    showPrescription(id);
                }
                break;
            }
            case 4:
                submitPrescription();
                break;
            case 5:
                withdrawPrescription();
                break;
            case 6:
                recoverPrescription();
                break;
            case 7:
                deletePrescription();
                break;
            case 0:
                return;
        }
    }
}

void Cli::herbMenu() {
    while (true) {
        std::cout << "\n------------ 中药信息管理 ------------\n";
        listHerbs();
        std::cout << "1. 新增中药\n2. 修改中药\n3. 删除中药\n0. 返回\n";
        int choice = -1;
        if (!promptInt("请选择", 0, 3, choice)) {
            return;
        }
        if (choice == 0) {
            return;
        }
        if (choice == 1) {
            addHerb();
        } else if (choice == 2) {
            modifyHerb();
        } else {
            deleteHerb();
        }
    }
}

void Cli::doseLimitMenu() {
    while (true) {
        std::cout << "\n------------ 药量控制 ------------\n";
        listDoseLimits();
        std::cout << "1. 新增药量控制\n2. 修改药量控制\n3. 删除药量控制\n0. 返回\n";
        int choice = -1;
        if (!promptInt("请选择", 0, 3, choice)) {
            return;
        }
        if (choice == 0) {
            return;
        }
        if (choice == 1) {
            addDoseLimit();
        } else if (choice == 2) {
            modifyDoseLimit();
        } else {
            deleteDoseLimit();
        }
    }
}

void Cli::historyMenu() {
    while (true) {
        std::cout << "\n------------ 撤销 / 重做 ------------\n";
        std::cout << "Undo 栈：" << commands_.undoSize() << " 步\n";
        const auto undoItems = commands_.undoDescriptions();
        for (std::size_t i = 0; i < undoItems.size(); ++i) {
            std::cout << "  U" << (i + 1) << ". " << undoItems[i] << "\n";
        }
        std::cout << "Redo 栈：" << commands_.redoSize() << " 步\n";
        const auto redoItems = commands_.redoDescriptions();
        for (std::size_t i = 0; i < redoItems.size(); ++i) {
            std::cout << "  R" << (i + 1) << ". " << redoItems[i] << "\n";
        }
        std::cout << "1. 撤销上一步\n2. 重做上一步\n0. 返回\n";
        int choice = -1;
        if (!promptInt("请选择", 0, 2, choice) || choice == 0) {
            return;
        }
        if (choice == 1) {
            if (commands_.undoDescriptions().empty()) {
                std::cout << "没有可以撤销的操作。\n";
                continue;
            }
            const std::string description = commands_.undoDescriptions().front();
            std::string error;
            if (commands_.undo(state_, error)) {
                std::cout << "已撤销：" << description << "\n";
            } else {
                std::cout << "撤销失败：" << error << "\n";
            }
        } else {
            if (commands_.redoDescriptions().empty()) {
                std::cout << "没有可以重做的操作。\n";
                continue;
            }
            const std::string description = commands_.redoDescriptions().front();
            std::string error;
            if (commands_.redo(state_, error)) {
                std::cout << "已重做：" << description << "\n";
            } else {
                std::cout << "重做失败：" << error << "\n";
            }
        }
    }
}

void Cli::systemMenu() {
    while (true) {
        std::cout << "\n------------ 系统与演示数据 ------------\n";
        std::cout << "1. 查看数据统计\n";
        std::cout << "2. 恢复全部演示数据（清空历史）\n";
        std::cout << "3. 查看使用说明\n0. 返回\n";
        int choice = -1;
        if (!promptInt("请选择", 0, 3, choice) || choice == 0) {
            return;
        }
        if (choice == 1) {
            showStatistics();
        } else if (choice == 2) {
            if (!confirm("确认恢复演示数据？输入 y 继续")) {
                std::cout << "已取消。\n";
                continue;
            }
            std::string error;
            if (!storage_.resetToSeed(error)) {
                std::cout << "恢复失败：" << error << "\n";
                continue;
            }
            AppState loaded;
            if (!storage_.load(loaded, error)) {
                std::cout << "恢复后加载失败：" << error << "\n";
                continue;
            }
            state_ = std::move(loaded);
            commands_.clear();
            std::cout << "演示数据已恢复，Undo/Redo 历史已清空。\n";
        } else {
            std::cout << "\n使用步骤：\n";
            std::cout << "1. 先维护中药库，再维护常用方。\n";
            std::cout << "2. 新建处方草稿，可手工添加药物或应用常用方。\n";
            std::cout << "3. 提交时检测名称、规格与药量；超量需确认签名。\n";
            std::cout << "4. 已提交处方只读，可业务撤销；恢复为草稿后可修改并重新提交。\n";
            std::cout << "5. 业务修改均支持 Undo/Redo，新操作会清空 Redo。\n";
            std::cout << "6. 数据目录：" << storage_.dataDir().string() << "\n";
            std::cout << "   重置来源：" << storage_.seedDir().string() << "\n";
        }
    }
}

void Cli::showStatistics() const {
    std::size_t ownFormulas = 0;
    for (const auto& formula : state_.formulas) {
        if (formula.doctorId == state_.currentDoctor.id) {
            ++ownFormulas;
        }
    }
    std::size_t draft = 0;
    std::size_t submitted = 0;
    std::size_t withdrawn = 0;
    for (const auto& prescription : state_.prescriptions) {
        if (prescription.status == PrescriptionStatus::Draft) {
            ++draft;
        } else if (prescription.status == PrescriptionStatus::Submitted) {
            ++submitted;
        } else {
            ++withdrawn;
        }
    }
    std::cout << "\n中药：" << state_.herbs.size() << " 条\n";
    std::cout << "药量控制：" << state_.doseLimits.size() << " 条\n";
    std::cout << "本人常用方：" << ownFormulas << " 个\n";
    std::cout << "处方：草稿 " << draft << "，已提交 " << submitted << "，已撤销 "
              << withdrawn << "\n";
    std::cout << "命令栈不变量：" << (commands_.checkInvariants() ? "正常" : "异常") << "\n";
}

void Cli::listFormulas() const {
    bool found = false;
    for (const auto& formula : state_.formulas) {
        if (formula.doctorId != state_.currentDoctor.id) {
            continue;
        }
        found = true;
        std::cout << "  " << makeFormulaCode(formula.seq) << "  " << formula.name << "  ["
                  << formula.items.size() << " 味]\n";
    }
    if (!found) {
        std::cout << "  （暂无本人常用方）\n";
    }
}

void Cli::showFormula(int seq) const {
    const CommonFormula* formula = findFormula(state_, seq);
    if (formula == nullptr || formula->doctorId != state_.currentDoctor.id) {
        std::cout << "常用方不存在或不属于当前医生。\n";
        return;
    }
    std::cout << "\n常用方：" << makeFormulaCode(formula->seq) << " " << formula->name
              << "  科室：" << formula->department << "\n";
    std::cout << "序号 | 药物名称 | 规格 | 用量(g)\n";
    std::cout << "-----+----------+------+--------\n";
    for (std::size_t i = 0; i < formula->items.size(); ++i) {
        const auto& item = formula->items[i];
        std::cout << std::setw(4) << std::left << (i + 1) << " | " << item.herbName << " | "
                  << item.spec << " | " << formatHundredths(item.doseHundredths) << "\n";
    }
}

bool Cli::chooseFormula(int& seq) const {
    listFormulas();
    int value = 0;
    if (!promptInt("常用方序号（0取消）", 0, 1000000, value) || value == 0) {
        return false;
    }
    const CommonFormula* formula = findFormula(state_, value);
    if (formula == nullptr || formula->doctorId != state_.currentDoctor.id) {
        std::cout << "常用方不存在或不属于当前医生。\n";
        return false;
    }
    seq = value;
    return true;
}

void Cli::addFormula() {
    std::cout << "\n--- 新增常用方 ---\n";
    CommonFormula formula;
    formula.doctorId = state_.currentDoctor.id;
    formula.department = state_.currentDoctor.department;
    if (!promptText("常用方名称", formula.name)) {
        return;
    }

    std::cout << "逐味录入药物，药物名称为空时结束。\n";
    while (true) {
        std::string herbName;
        if (!promptText("药物名称（空结束）", herbName, true)) {
            return;
        }
        if (herbName.empty()) {
            break;
        }
        std::string spec;
        if (!selectHerb(herbName, herbName, spec)) {
            continue;
        }
        const auto duplicate = std::find_if(formula.items.begin(), formula.items.end(),
                                            [&](const FormulaItem& item) {
                                                return item.herbName == herbName && item.spec == spec;
                                            });
        if (duplicate != formula.items.end()) {
            std::cout << "该药物已在此常用方中，请勿重复添加。\n";
            continue;
        }
        long long dose = 0;
        if (!promptDose("用量(g)", dose)) {
            continue;
        }
        formula.items.push_back(FormulaItem{herbName, spec, dose});
    }
    if (formula.items.empty()) {
        std::cout << "未录入药味，已取消新增。\n";
        return;
    }
    executeCommand(std::make_unique<AddFormulaCommand>(formula));
}

void Cli::modifyFormula() {
    int seq = 0;
    if (!chooseFormula(seq)) {
        return;
    }
    CommonFormula formula = *findFormula(state_, seq);
    while (true) {
        std::cout << "\n--- 修改常用方 " << makeFormulaCode(formula.seq) << " ---\n";
        showFormula(formula.seq);
        std::cout << "1. 修改名称\n2. 新增药味\n3. 修改药味\n4. 删除药味\n0. 返回\n";
        int choice = -1;
        if (!promptInt("请选择", 0, 4, choice) || choice == 0) {
            return;
        }
        CommonFormula after = formula;
        std::string detail;
        if (choice == 1) {
            std::string nextName;
            if (!promptDefaultText("新名称", formula.name, nextName) || nextName == formula.name) {
                continue;
            }
            after.name = nextName;
            detail = "修改名称";
        } else if (choice == 2) {
            if (!inputFormulaItem(after.items)) {
                continue;
            }
            detail = "新增药味";
        } else if (choice == 3) {
            if (after.items.empty()) {
                std::cout << "药方为空。\n";
                continue;
            }
            int index = 0;
            if (!promptInt("药味序号", 1, static_cast<int>(after.items.size()), index)) {
                continue;
            }
            std::string herbName;
            if (!promptText("新药物名称（空取消）", herbName, true) || herbName.empty()) {
                continue;
            }
            std::string spec;
            if (!selectHerb(herbName, herbName, spec)) {
                continue;
            }
            bool duplicate = false;
            for (std::size_t i = 0; i < after.items.size(); ++i) {
                if (static_cast<int>(i) != index - 1 && after.items[i].herbName == herbName &&
                    after.items[i].spec == spec) {
                    duplicate = true;
                }
            }
            if (duplicate) {
                std::cout << "该药物已在药方中。\n";
                continue;
            }
            long long dose = 0;
            if (!promptDose("用量(g)", dose)) {
                continue;
            }
            after.items[static_cast<std::size_t>(index - 1)] = FormulaItem{herbName, spec, dose};
            detail = "修改药味";
        } else {
            if (after.items.size() <= 1) {
                std::cout << "常用方至少保留一味药。\n";
                continue;
            }
            int index = 0;
            if (!promptInt("药味序号", 1, static_cast<int>(after.items.size()), index)) {
                continue;
            }
            after.items.erase(after.items.begin() + (index - 1));
            detail = "删除药味";
        }

        if (executeCommand(std::make_unique<UpdateFormulaCommand>(formula, after, detail))) {
            formula = *findFormula(state_, seq);
        }
    }
}

void Cli::deleteFormula() {
    int seq = 0;
    if (!chooseFormula(seq)) {
        return;
    }
    const CommonFormula formula = *findFormula(state_, seq);
    if (!confirm("确认删除常用方 " + formula.name + "？")) {
        return;
    }
    executeCommand(std::make_unique<DeleteFormulaCommand>(formula));
}

void Cli::applyFormulaToPrescription() {
    int seq = 0;
    if (!chooseFormula(seq)) {
        return;
    }
    const CommonFormula formula = *findFormula(state_, seq);
    std::string prescriptionId;
    if (!chooseOwnedPrescription(PrescriptionStatus::Draft, prescriptionId)) {
        return;
    }
    Prescription before = *findPrescription(state_, prescriptionId);
    Prescription after = before;
    std::set<std::string> keys;
    for (const auto& item : after.items) {
        keys.insert(itemKey(item.herbName, item.spec));
    }
    for (const auto& item : formula.items) {
        if (!keys.insert(itemKey(item.herbName, item.spec)).second) {
            std::cout << "处方中已存在药物：" << item.herbName << " / " << item.spec
                      << "，整次应用已取消。\n";
            return;
        }
    }
    for (const auto& item : formula.items) {
        after.items.push_back(PrescriptionItem{0, item.herbName, item.spec, item.doseHundredths});
    }
    renumberPrescriptionItems(after);
    after.updatedAt = currentTimestamp();
    executeCommand(std::make_unique<ApplyFormulaCommand>(before, after, formula.name));
}

void Cli::listPrescriptions() const {
    if (state_.prescriptions.empty()) {
        std::cout << "  （暂无处方）\n";
        return;
    }
    std::cout << "处方编号 | 医生 | 状态 | 剂数 | 药味数 | 超量签名\n";
    std::cout << "---------+------+------+------+--------+----------\n";
    for (const auto& prescription : state_.prescriptions) {
        const bool own = prescription.doctorId == state_.currentDoctor.id;
        std::cout << prescription.id << " | " << prescription.doctorName
                  << (own ? "(本人)" : "(他人)") << " | " << statusToText(prescription.status)
                  << " | " << prescription.doses << " | " << prescription.items.size() << " | "
                  << (prescription.overdoseSignature.empty() ? "无" : prescription.overdoseSignature)
                  << "\n";
    }
}

void Cli::showPrescription(const std::string& id) const {
    const Prescription* prescription = findPrescription(state_, id);
    if (prescription == nullptr) {
        std::cout << "处方不存在。\n";
        return;
    }
    std::cout << "\n处方编号：" << prescription->id << "   状态：" << statusToText(prescription->status)
              << "\n医生：" << prescription->doctorName << " [" << prescription->doctorId
              << "]   剂数：" << prescription->doses << "\n";
    std::cout << "创建：" << prescription->createdAt << "   更新：" << prescription->updatedAt
              << "\n";
    if (!prescription->overdoseSignature.empty()) {
        std::cout << "超量签名：" << prescription->overdoseSignature << "  "
                  << prescription->overdoseSignedAt << "\n";
    }
    std::cout << "行号 | 药物名称 | 规格 | 用量(g) | 药量提示\n";
    std::cout << "-----+----------+------+---------+---------\n";
    for (const auto& item : prescription->items) {
        const DoseLimit* limit = findDoseLimit(state_, item.herbName, item.spec);
        std::string hint = "无限制";
        if (limit != nullptr) {
            hint = item.doseHundredths > limit->maxHundredths
                       ? "超量，上限 " + formatHundredths(limit->maxHundredths)
                       : "上限 " + formatHundredths(limit->maxHundredths);
        }
        std::cout << std::setw(4) << std::left << item.lineNo << " | " << item.herbName << " | "
                  << item.spec << " | " << formatHundredths(item.doseHundredths) << " | " << hint
                  << "\n";
    }
}

bool Cli::chooseOwnedPrescription(PrescriptionStatus status, std::string& id) const {
    listPrescriptions();
    std::string value;
    if (!promptText("处方编号（空取消）", value, true) || value.empty()) {
        return false;
    }
    const Prescription* prescription = findPrescription(state_, value);
    if (prescription == nullptr) {
        std::cout << "处方不存在。\n";
        return false;
    }
    if (prescription->doctorId != state_.currentDoctor.id) {
        std::cout << "权限拒绝：该处方属于其他医生，当前医生不能执行此操作。\n";
        return false;
    }
    if (prescription->status != status) {
        std::cout << "状态不符：当前状态为 " << statusToText(prescription->status)
                  << "，需要 " << statusToText(status) << "。\n";
        return false;
    }
    id = value;
    return true;
}

void Cli::createPrescription() {
    std::cout << "\n--- 新增处方草稿 ---\n";
    int doses = 1;
    if (!promptInt("剂数", 1, 999, doses)) {
        return;
    }
    Prescription prescription;
    prescription.doctorId = state_.currentDoctor.id;
    prescription.doctorName = state_.currentDoctor.name;
    prescription.doses = doses;
    prescription.status = PrescriptionStatus::Draft;
    if (executeCommand(std::make_unique<AddPrescriptionCommand>(prescription))) {
        std::cout << "新处方编号：" << state_.prescriptions.back().id
                  << "。接下来可在“修改草稿处方”中添加药味。\n";
    }
}

void Cli::editDraftPrescription() {
    std::string id;
    if (!chooseOwnedPrescription(PrescriptionStatus::Draft, id)) {
        return;
    }
    while (true) {
        const Prescription* current = findPrescription(state_, id);
        if (current == nullptr || current->status != PrescriptionStatus::Draft ||
            current->doctorId != state_.currentDoctor.id) {
            std::cout << "处方已不再是当前医生的草稿，返回。\n";
            return;
        }
        showPrescription(id);
        std::cout << "1. 修改剂数\n2. 新增药味\n3. 修改药味\n4. 删除药味\n";
        std::cout << "5. 应用常用方\n6. 规范检测\n0. 返回\n";
        int choice = -1;
        if (!promptInt("请选择", 0, 6, choice) || choice == 0) {
            return;
        }
        Prescription before = *current;
        Prescription after = before;
        if (choice == 1) {
            int doses = before.doses;
            if (!promptInt("新剂数", 1, 999, doses) || doses == before.doses) {
                continue;
            }
            after.doses = doses;
            after.updatedAt = currentTimestamp();
            executeCommand(
                std::make_unique<UpdatePrescriptionDosesCommand>(before, after, before.doses, doses));
        } else if (choice == 2) {
            if (!inputPrescriptionItem(after, -1)) {
                continue;
            }
            after.updatedAt = currentTimestamp();
            executeCommand(std::make_unique<AddPrescriptionItemCommand>(
                before, after, prescriptionItemText(after.items.back())));
        } else if (choice == 3) {
            if (after.items.empty()) {
                std::cout << "处方暂无药味。\n";
                continue;
            }
            int index = 0;
            if (!promptInt("药味行号", 1, static_cast<int>(after.items.size()), index)) {
                continue;
            }
            if (!inputPrescriptionItem(after, index - 1)) {
                continue;
            }
            after.updatedAt = currentTimestamp();
            executeCommand(std::make_unique<UpdatePrescriptionItemCommand>(
                before, after,
                prescriptionItemText(after.items[static_cast<std::size_t>(index - 1)])));
        } else if (choice == 4) {
            if (after.items.empty()) {
                std::cout << "处方暂无药味。\n";
                continue;
            }
            int index = 0;
            if (!promptInt("药味行号", 1, static_cast<int>(after.items.size()), index)) {
                continue;
            }
            const std::string itemText =
                prescriptionItemText(after.items[static_cast<std::size_t>(index - 1)]);
            after.items.erase(after.items.begin() + (index - 1));
            renumberPrescriptionItems(after);
            after.updatedAt = currentTimestamp();
            executeCommand(
                std::make_unique<DeletePrescriptionItemCommand>(before, after, itemText));
        } else if (choice == 5) {
            int seq = 0;
            if (!chooseFormula(seq)) {
                continue;
            }
            const CommonFormula formula = *findFormula(state_, seq);
            std::set<std::string> keys;
            for (const auto& item : after.items) {
                keys.insert(itemKey(item.herbName, item.spec));
            }
            bool duplicate = false;
            for (const auto& item : formula.items) {
                if (!keys.insert(itemKey(item.herbName, item.spec)).second) {
                    duplicate = true;
                    std::cout << "处方中已存在药物：" << item.herbName << " / " << item.spec
                              << "，整次应用已取消。\n";
                    break;
                }
            }
            if (duplicate) {
                continue;
            }
            for (const auto& item : formula.items) {
                after.items.push_back(
                    PrescriptionItem{0, item.herbName, item.spec, item.doseHundredths});
            }
            renumberPrescriptionItems(after);
            after.updatedAt = currentTimestamp();
            executeCommand(std::make_unique<ApplyFormulaCommand>(before, after, formula.name));
        } else {
            showValidation(before);
        }
    }
}

void Cli::submitPrescription() {
    std::string id;
    if (!chooseOwnedPrescription(PrescriptionStatus::Draft, id)) {
        return;
    }
    const Prescription before = *findPrescription(state_, id);
    const SubmissionValidation validation = validatePrescriptionForSubmit(state_, before);
    if (!validation.ok()) {
        std::cout << "\n规范检测未通过：\n";
        for (const auto& error : validation.errors) {
            std::cout << "  - " << error << "\n";
        }
        return;
    }
    bool confirmedOverdose = false;
    if (validation.hasOverdose()) {
        std::cout << "\n检测到超量用药：\n";
        for (const auto& warning : validation.overdoseWarnings) {
            std::cout << "  ! " << warning << "\n";
        }
        if (!confirm("确认超量并由当前医生电子签名？")) {
            std::cout << "已取消提交，处方保持草稿状态。\n";
            return;
        }
        confirmedOverdose = true;
    }
    Prescription after = before;
    after.status = PrescriptionStatus::Submitted;
    after.updatedAt = currentTimestamp();
    if (confirmedOverdose) {
        after.overdoseSignature = state_.currentDoctor.name;
        after.overdoseSignedAt = currentTimestamp();
    }
    executeCommand(std::make_unique<SubmitPrescriptionCommand>(before, after));
}

void Cli::withdrawPrescription() {
    std::string id;
    if (!chooseOwnedPrescription(PrescriptionStatus::Submitted, id)) {
        return;
    }
    const Prescription before = *findPrescription(state_, id);
    if (!confirm("确认撤销（业务）处方 " + id + "？")) {
        return;
    }
    Prescription after = before;
    after.status = PrescriptionStatus::Withdrawn;
    after.updatedAt = currentTimestamp();
    executeCommand(std::make_unique<WithdrawPrescriptionCommand>(before, after));
}

void Cli::recoverPrescription() {
    std::string id;
    if (!chooseOwnedPrescription(PrescriptionStatus::Withdrawn, id)) {
        return;
    }
    const Prescription before = *findPrescription(state_, id);
    if (!confirm("确认恢复处方 " + id + " 为草稿？旧超量签名将被清除。")) {
        return;
    }
    Prescription after = before;
    after.status = PrescriptionStatus::Draft;
    after.overdoseSignature.clear();
    after.overdoseSignedAt.clear();
    after.updatedAt = currentTimestamp();
    executeCommand(std::make_unique<RecoverPrescriptionCommand>(before, after));
    std::cout << "恢复成功。修改后必须重新提交。\n";
}

void Cli::deletePrescription() {
    std::string id;
    if (!chooseOwnedPrescription(PrescriptionStatus::Draft, id)) {
        return;
    }
    const Prescription prescription = *findPrescription(state_, id);
    if (!confirm("确认删除草稿处方 " + id + "？")) {
        return;
    }
    executeCommand(std::make_unique<DeletePrescriptionCommand>(prescription));
}

void Cli::showValidation(const Prescription& prescription) const {
    const SubmissionValidation validation = validatePrescriptionForSubmit(state_, prescription);
    std::cout << "\n规范检测结果：\n";
    if (validation.ok()) {
        std::cout << "  - 药物名称、规格和剂数检查通过。\n";
    } else {
        for (const auto& error : validation.errors) {
            std::cout << "  - 错误：" << error << "\n";
        }
    }
    if (validation.hasOverdose()) {
        for (const auto& warning : validation.overdoseWarnings) {
            std::cout << "  ! 超量：" << warning << "\n";
        }
    } else {
        std::cout << "  - 未发现超过药量控制上限的药物。\n";
    }
}

void Cli::listHerbs() const {
    if (state_.herbs.empty()) {
        std::cout << "  （暂无中药）\n";
        return;
    }
    std::cout << "编号 | 药物名称 | 产地 | 规格\n";
    std::cout << "-----+----------+------+------\n";
    for (const auto& herb : state_.herbs) {
        std::cout << herb.number << " | " << herb.name << " | " << herb.origin << " | "
                  << herb.spec << "\n";
    }
}

void Cli::addHerb() {
    Herb herb;
    if (!promptText("中药编号", herb.number) || !promptText("药物名称", herb.name) ||
        !promptText("产地", herb.origin) || !promptText("规格", herb.spec)) {
        return;
    }
    executeCommand(std::make_unique<AddHerbCommand>(herb));
}

void Cli::modifyHerb() {
    std::string number;
    if (!promptText("要修改的中药编号（空取消）", number, true) || number.empty()) {
        return;
    }
    const Herb* found = findHerbByNumber(state_, number);
    if (found == nullptr) {
        std::cout << "中药编号不存在。\n";
        return;
    }
    const Herb before = *found;
    Herb after = before;
    if (!promptDefaultText("药物名称", before.name, after.name) ||
        !promptDefaultText("产地", before.origin, after.origin) ||
        !promptDefaultText("规格", before.spec, after.spec) ||
        !promptDefaultText("中药编号", before.number, after.number)) {
        return;
    }
    if (after.name == before.name && after.origin == before.origin &&
        after.spec == before.spec && after.number == before.number) {
        std::cout << "没有修改内容。\n";
        return;
    }
    executeCommand(std::make_unique<UpdateHerbCommand>(before, after));
}

void Cli::deleteHerb() {
    std::string number;
    if (!promptText("要删除的中药编号（空取消）", number, true) || number.empty()) {
        return;
    }
    const Herb* found = findHerbByNumber(state_, number);
    if (found == nullptr) {
        std::cout << "中药编号不存在。\n";
        return;
    }
    const Herb herb = *found;
    if (isHerbReferenced(state_, herb.name, herb.spec)) {
        std::cout << "该中药已被常用方、处方或药量控制引用，不能删除。\n";
        return;
    }
    if (!confirm("确认删除 " + herb.number + " " + herb.name + "？")) {
        return;
    }
    executeCommand(std::make_unique<DeleteHerbCommand>(herb));
}

void Cli::listDoseLimits() const {
    if (state_.doseLimits.empty()) {
        std::cout << "  （暂无药量控制）\n";
        return;
    }
    std::cout << "序号 | 药物名称 | 规格 | 最大用量(g)\n";
    std::cout << "-----+----------+------+-------------\n";
    for (std::size_t i = 0; i < state_.doseLimits.size(); ++i) {
        const auto& limit = state_.doseLimits[i];
        std::cout << std::setw(4) << std::left << (i + 1) << " | " << limit.herbName << " | "
                  << limit.spec << " | " << formatHundredths(limit.maxHundredths) << "\n";
    }
}

void Cli::addDoseLimit() {
    std::string herbName;
    if (!promptText("药物名称（空取消）", herbName, true) || herbName.empty()) {
        return;
    }
    std::string spec;
    if (!selectHerb(herbName, herbName, spec)) {
        return;
    }
    long long maxDose = 0;
    if (!promptDose("最大用量(g)", maxDose)) {
        return;
    }
    executeCommand(std::make_unique<AddDoseLimitCommand>(DoseLimit{herbName, spec, maxDose}));
}

void Cli::modifyDoseLimit() {
    if (state_.doseLimits.empty()) {
        std::cout << "暂无药量控制。\n";
        return;
    }
    listDoseLimits();
    int index = 0;
    if (!promptInt("序号（0取消）", 0, static_cast<int>(state_.doseLimits.size()), index) ||
        index == 0) {
        return;
    }
    DoseLimit before = state_.doseLimits[static_cast<std::size_t>(index - 1)];
    DoseLimit after = before;
    std::string herbName;
    if (!promptText("新药物名称（空保持）", herbName, true)) {
        return;
    }
    if (!herbName.empty()) {
        std::string spec;
        if (!selectHerb(herbName, herbName, spec)) {
            return;
        }
        after.herbName = herbName;
        after.spec = spec;
    }
    std::string doseText;
    if (!promptText("新最大用量(g)（空保持）", doseText, true)) {
        return;
    }
    if (!doseText.empty()) {
        long long value = 0;
        if (!parseDoseHundredths(doseText, value) || value <= 0) {
            std::cout << "最大用量必须是大于 0 的克数，最多两位小数。\n";
            return;
        }
        after.maxHundredths = value;
    }
    if (after.herbName == before.herbName && after.spec == before.spec &&
        after.maxHundredths == before.maxHundredths) {
        std::cout << "没有修改内容。\n";
        return;
    }
    executeCommand(std::make_unique<UpdateDoseLimitCommand>(before, after));
}

void Cli::deleteDoseLimit() {
    if (state_.doseLimits.empty()) {
        std::cout << "暂无药量控制。\n";
        return;
    }
    listDoseLimits();
    int index = 0;
    if (!promptInt("序号（0取消）", 0, static_cast<int>(state_.doseLimits.size()), index) ||
        index == 0) {
        return;
    }
    const DoseLimit limit = state_.doseLimits[static_cast<std::size_t>(index - 1)];
    if (!confirm("确认删除该药量控制？")) {
        return;
    }
    executeCommand(std::make_unique<DeleteDoseLimitCommand>(limit));
}

bool Cli::executeCommand(std::unique_ptr<ICommand> command) {
    const std::string description = command->description();
    std::string error;
    if (!commands_.execute(std::move(command), state_, error)) {
        std::cout << "操作失败：" << error << "\n";
        return false;
    }
    std::cout << "操作成功：" << description << "\n";
    return true;
}

bool Cli::promptText(const std::string& label, std::string& value, bool allowEmpty) const {
    while (true) {
        std::cout << label << "：";
        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            return false;
        }
        value = trim(line);
        if (value.empty() && !allowEmpty) {
            std::cout << "不能为空，请重新输入。\n";
            continue;
        }
        if (containsInvalidFieldChar(value)) {
            std::cout << "不能包含竖线或换行，请重新输入。\n";
            continue;
        }
        return true;
    }
}

bool Cli::promptDefaultText(const std::string& label, const std::string& defaultValue,
                            std::string& value) const {
    std::cout << label << " [" << defaultValue << "]：";
    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cout << "\n";
        return false;
    }
    value = trim(line);
    if (value.empty()) {
        value = defaultValue;
    }
    if (value.empty() || containsInvalidFieldChar(value)) {
        std::cout << "输入无效，操作取消。\n";
        return false;
    }
    return true;
}

bool Cli::promptInt(const std::string& label, int minValue, int maxValue, int& value) const {
    while (true) {
        std::cout << label << "：";
        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            return false;
        }
        int parsed = 0;
        if (!parseIntStrict(line, parsed) || parsed < minValue || parsed > maxValue) {
            std::cout << "请输入 " << minValue << " 到 " << maxValue << " 之间的整数。\n";
            continue;
        }
        value = parsed;
        return true;
    }
}

bool Cli::promptDose(const std::string& label, long long& hundredths) const {
    while (true) {
        std::cout << label << "：";
        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            return false;
        }
        if (trim(line).empty()) {
            return false;
        }
        long long parsed = 0;
        if (!parseDoseHundredths(line, parsed) || parsed <= 0) {
            std::cout << "请输入大于 0 的克数，最多两位小数。\n";
            continue;
        }
        hundredths = parsed;
        return true;
    }
}

bool Cli::confirm(const std::string& question) const {
    std::cout << question << " [y/N]：";
    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cout << "\n";
        return false;
    }
    const std::string answer = trim(line);
    return answer == "y" || answer == "Y" || answer == "yes" || answer == "YES" ||
           answer == "是";
}

bool Cli::selectHerb(const std::string& initialName, std::string& name, std::string& spec) const {
    name = initialName;
    if (name.empty()) {
        if (!promptText("药物名称（空取消）", name, true) || name.empty()) {
            return false;
        }
    }
    std::vector<const Herb*> matches;
    for (const auto& herb : state_.herbs) {
        if (herb.name == name) {
            matches.push_back(&herb);
        }
    }
    if (matches.empty()) {
        std::cout << "中药库中不存在药物：" << name << "。\n";
        return false;
    }
    if (matches.size() == 1) {
        spec = matches.front()->spec;
        std::cout << "匹配规格：" << spec << "\n";
        return true;
    }
    std::cout << "该药物有多个规格：\n";
    for (const auto& herb : matches) {
        std::cout << "  " << herb->spec << "（产地 " << herb->origin << "）\n";
    }
    if (!promptText("规格（空取消）", spec, true) || spec.empty()) {
        return false;
    }
    for (const auto& herb : matches) {
        if (herb->spec == spec) {
            return true;
        }
    }
    std::cout << "该药物不存在指定规格：" << spec << "。\n";
    return false;
}

bool Cli::inputFormulaItem(std::vector<FormulaItem>& items) const {
    std::string herbName;
    if (!promptText("药物名称（空取消）", herbName, true) || herbName.empty()) {
        return false;
    }
    std::string spec;
    if (!selectHerb(herbName, herbName, spec)) {
        return false;
    }
    for (const auto& item : items) {
        if (item.herbName == herbName && item.spec == spec) {
            std::cout << "该药物已在药方中。\n";
            return false;
        }
    }
    long long dose = 0;
    if (!promptDose("用量(g)", dose)) {
        return false;
    }
    items.push_back(FormulaItem{herbName, spec, dose});
    return true;
}

bool Cli::inputPrescriptionItem(Prescription& prescription, int editingIndex) const {
    std::string herbName;
    if (!promptText("药物名称（空取消）", herbName, true) || herbName.empty()) {
        return false;
    }
    std::string spec;
    if (!selectHerb(herbName, herbName, spec)) {
        return false;
    }
    for (std::size_t i = 0; i < prescription.items.size(); ++i) {
        if (static_cast<int>(i) != editingIndex && prescription.items[i].herbName == herbName &&
            prescription.items[i].spec == spec) {
            std::cout << "该药物已在处方中。\n";
            return false;
        }
    }
    long long dose = 0;
    if (!promptDose("用量(g)", dose)) {
        return false;
    }
    if (editingIndex >= 0) {
        prescription.items[static_cast<std::size_t>(editingIndex)] =
            PrescriptionItem{editingIndex + 1, herbName, spec, dose};
    } else {
        prescription.items.push_back(
            PrescriptionItem{static_cast<int>(prescription.items.size() + 1), herbName, spec, dose});
    }
    renumberPrescriptionItems(prescription);
    return true;
}

std::string Cli::prescriptionItemText(const PrescriptionItem& item) const {
    return item.herbName + " / " + item.spec + " " + formatHundredths(item.doseHundredths) + " g";
}

}  // namespace tcm

