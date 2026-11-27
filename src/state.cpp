#include "state.h"

#include "text_utils.h"

#include <algorithm>
#include <set>
#include <utility>

namespace tcm {
namespace {

bool sameHerbKey(const std::string& lhsName, const std::string& lhsSpec,
                 const std::string& rhsName, const std::string& rhsSpec) {
    return lhsName == rhsName && lhsSpec == rhsSpec;
}

std::string keyText(const std::string& name, const std::string& spec) {
    return name + " / " + spec;
}

bool validateItems(const AppState& state, const std::vector<FormulaItem>& items,
                   std::string& error) {
    if (items.empty()) {
        error = "药方至少需要一味药。";
        return false;
    }
    std::set<std::pair<std::string, std::string>> keys;
    for (const auto& item : items) {
        if (!validateRequiredText(item.herbName, "药物名称", error) ||
            !validateRequiredText(item.spec, "规格", error)) {
            return false;
        }
        if (item.doseHundredths <= 0) {
            error = "药物用量必须大于 0 g。";
            return false;
        }
        if (findHerb(state, item.herbName, item.spec) == nullptr) {
            error = "中药库中不存在：" + keyText(item.herbName, item.spec);
            return false;
        }
        if (!keys.emplace(item.herbName, item.spec).second) {
            error = "药方中存在重复药物：" + keyText(item.herbName, item.spec);
            return false;
        }
    }
    return true;
}

bool validatePrescriptionItems(const AppState& state, const std::vector<PrescriptionItem>& items,
                               bool requireItems, std::string& error) {
    if (requireItems && items.empty()) {
        error = "处方至少需要一味药。";
        return false;
    }
    std::set<std::pair<std::string, std::string>> keys;
    for (const auto& item : items) {
        if (!validateRequiredText(item.herbName, "药物名称", error) ||
            !validateRequiredText(item.spec, "规格", error)) {
            return false;
        }
        if (item.doseHundredths <= 0) {
            error = "药物用量必须大于 0 g。";
            return false;
        }
        if (findHerb(state, item.herbName, item.spec) == nullptr) {
            error = "中药库中不存在：" + keyText(item.herbName, item.spec);
            return false;
        }
        if (!keys.emplace(item.herbName, item.spec).second) {
            error = "处方中存在重复药物：" + keyText(item.herbName, item.spec);
            return false;
        }
    }
    return true;
}

}  // namespace

const Herb* findHerb(const AppState& state, const std::string& name, const std::string& spec) {
    const auto it = std::find_if(state.herbs.begin(), state.herbs.end(), [&](const Herb& herb) {
        return herb.name == name && herb.spec == spec;
    });
    return it == state.herbs.end() ? nullptr : &*it;
}

Herb* findHerb(AppState& state, const std::string& name, const std::string& spec) {
    const auto it = std::find_if(state.herbs.begin(), state.herbs.end(), [&](const Herb& herb) {
        return herb.name == name && herb.spec == spec;
    });
    return it == state.herbs.end() ? nullptr : &*it;
}

const CommonFormula* findFormula(const AppState& state, int seq) {
    const auto it = std::find_if(state.formulas.begin(), state.formulas.end(),
                                 [&](const CommonFormula& formula) { return formula.seq == seq; });
    return it == state.formulas.end() ? nullptr : &*it;
}

CommonFormula* findFormula(AppState& state, int seq) {
    const auto it = std::find_if(state.formulas.begin(), state.formulas.end(),
                                 [&](const CommonFormula& formula) { return formula.seq == seq; });
    return it == state.formulas.end() ? nullptr : &*it;
}

const Prescription* findPrescription(const AppState& state, const std::string& id) {
    const auto it = std::find_if(state.prescriptions.begin(), state.prescriptions.end(),
                                 [&](const Prescription& p) { return p.id == id; });
    return it == state.prescriptions.end() ? nullptr : &*it;
}

Prescription* findPrescription(AppState& state, const std::string& id) {
    const auto it = std::find_if(state.prescriptions.begin(), state.prescriptions.end(),
                                 [&](const Prescription& p) { return p.id == id; });
    return it == state.prescriptions.end() ? nullptr : &*it;
}

const DoseLimit* findDoseLimit(const AppState& state, const std::string& name,
                               const std::string& spec) {
    const auto it = std::find_if(state.doseLimits.begin(), state.doseLimits.end(),
                                 [&](const DoseLimit& limit) {
                                     return sameHerbKey(limit.herbName, limit.spec, name, spec);
                                 });
    return it == state.doseLimits.end() ? nullptr : &*it;
}

DoseLimit* findDoseLimit(AppState& state, const std::string& name, const std::string& spec) {
    const auto it = std::find_if(state.doseLimits.begin(), state.doseLimits.end(),
                                 [&](const DoseLimit& limit) {
                                     return sameHerbKey(limit.herbName, limit.spec, name, spec);
                                 });
    return it == state.doseLimits.end() ? nullptr : &*it;
}

bool validateRequiredText(const std::string& value, const std::string& label, std::string& error) {
    if (trim(value).empty()) {
        error = label + "不能为空。";
        return false;
    }
    if (containsInvalidFieldChar(value)) {
        error = label + "不能包含竖线或换行。";
        return false;
    }
    return true;
}

bool validateHerbRecord(const AppState& state, const Herb& herb, const std::string& ignoredNumber,
                        std::string& error) {
    if (!validateRequiredText(herb.number, "中药编号", error) ||
        !validateRequiredText(herb.name, "药物名称", error) ||
        !validateRequiredText(herb.origin, "产地", error) ||
        !validateRequiredText(herb.spec, "规格", error)) {
        return false;
    }
    for (const auto& existing : state.herbs) {
        if (existing.number == herb.number && existing.number != ignoredNumber) {
            error = "中药编号已存在：" + herb.number;
            return false;
        }
        if (sameHerbKey(existing.name, existing.spec, herb.name, herb.spec) &&
            existing.number != ignoredNumber) {
            error = "药物名称和规格组合已存在：" + keyText(herb.name, herb.spec);
            return false;
        }
    }
    return true;
}

bool validateDoseLimitRecord(const AppState& state, const DoseLimit& limit, std::string& error) {
    if (!validateRequiredText(limit.herbName, "药物名称", error) ||
        !validateRequiredText(limit.spec, "规格", error)) {
        return false;
    }
    if (limit.maxHundredths <= 0) {
        error = "最大用量必须大于 0 g。";
        return false;
    }
    if (findHerb(state, limit.herbName, limit.spec) == nullptr) {
        error = "中药库中不存在：" + keyText(limit.herbName, limit.spec);
        return false;
    }
    return true;
}

bool validateFormulaRecord(const AppState& state, const CommonFormula& formula, std::string& error) {
    if (!validateRequiredText(formula.name, "常用方名称", error) ||
        !validateRequiredText(formula.doctorId, "医生编号", error) ||
        !validateRequiredText(formula.department, "科室", error)) {
        return false;
    }
    if (!validateItems(state, formula.items, error)) {
        return false;
    }
    for (const auto& existing : state.formulas) {
        if (existing.seq != formula.seq && existing.doctorId == formula.doctorId &&
            existing.name == formula.name) {
            error = "当前医生的常用方名称已存在：" + formula.name;
            return false;
        }
    }
    return true;
}

bool validatePrescriptionRecord(const AppState& state, const Prescription& prescription,
                                std::string& error) {
    if (!validateRequiredText(prescription.id, "处方编号", error) ||
        !validateRequiredText(prescription.doctorId, "医生编号", error) ||
        !validateRequiredText(prescription.doctorName, "医生姓名", error)) {
        return false;
    }
    if (prescription.doses < 1) {
        error = "剂数必须为正整数。";
        return false;
    }
    return validatePrescriptionItems(state, prescription.items, false, error);
}

bool isHerbReferenced(const AppState& state, const std::string& name, const std::string& spec) {
    for (const auto& limit : state.doseLimits) {
        if (sameHerbKey(limit.herbName, limit.spec, name, spec)) {
            return true;
        }
    }
    for (const auto& formula : state.formulas) {
        for (const auto& item : formula.items) {
            if (sameHerbKey(item.herbName, item.spec, name, spec)) {
                return true;
            }
        }
    }
    for (const auto& prescription : state.prescriptions) {
        for (const auto& item : prescription.items) {
            if (sameHerbKey(item.herbName, item.spec, name, spec)) {
                return true;
            }
        }
    }
    return false;
}

void renumberPrescriptionItems(Prescription& prescription) {
    for (std::size_t i = 0; i < prescription.items.size(); ++i) {
        prescription.items[i].lineNo = static_cast<int>(i + 1);
    }
}

SubmissionValidation validatePrescriptionForSubmit(const AppState& state,
                                                    const Prescription& prescription) {
    SubmissionValidation result;
    std::string error;
    if (prescription.status != PrescriptionStatus::Draft) {
        result.errors.push_back("只有草稿处方可以提交。");
    }
    if (prescription.doctorId != state.currentDoctor.id) {
        result.errors.push_back("只能提交当前医生自己的处方。");
    }
    if (!validatePrescriptionRecord(state, prescription, error)) {
        result.errors.push_back(error);
    }
    if (!validatePrescriptionItems(state, prescription.items, true, error)) {
        result.errors.push_back(error);
    }

    for (const auto& item : prescription.items) {
        const DoseLimit* limit = findDoseLimit(state, item.herbName, item.spec);
        if (limit != nullptr && item.doseHundredths > limit->maxHundredths) {
            result.overdoseWarnings.push_back(
                keyText(item.herbName, item.spec) + "：当前 " +
                formatHundredths(item.doseHundredths) + " g，上限 " +
                formatHundredths(limit->maxHundredths) + " g。");
        }
    }
    return result;
}

}  // namespace tcm
