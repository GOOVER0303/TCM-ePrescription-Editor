#include "commands.h"

#include "state.h"
#include "text_utils.h"

#include <algorithm>
#include <string>
#include <utility>

namespace tcm {
namespace {

std::string herbLabel(const std::string& name, const std::string& spec) {
    return name + " / " + spec;
}

bool eraseHerbByNumber(AppState& state, const std::string& number) {
    const auto it = std::find_if(state.herbs.begin(), state.herbs.end(),
                                 [&](const Herb& herb) { return herb.number == number; });
    if (it == state.herbs.end()) {
        return false;
    }
    state.herbs.erase(it);
    return true;
}

bool replaceHerb(AppState& state, const Herb& before, const Herb& after) {
    for (auto& herb : state.herbs) {
        if (herb.number == before.number) {
            herb = after;
            return true;
        }
    }
    return false;
}

bool eraseDoseLimit(AppState& state, const std::string& name, const std::string& spec) {
    const auto it = std::find_if(state.doseLimits.begin(), state.doseLimits.end(),
                                 [&](const DoseLimit& limit) {
                                     return limit.herbName == name && limit.spec == spec;
                                 });
    if (it == state.doseLimits.end()) {
        return false;
    }
    state.doseLimits.erase(it);
    return true;
}

bool replaceDoseLimit(AppState& state, const DoseLimit& before, const DoseLimit& after) {
    for (auto& limit : state.doseLimits) {
        if (limit.herbName == before.herbName && limit.spec == before.spec) {
            limit = after;
            return true;
        }
    }
    return false;
}

bool eraseFormula(AppState& state, int seq) {
    const auto it = std::find_if(state.formulas.begin(), state.formulas.end(),
                                 [&](const CommonFormula& formula) { return formula.seq == seq; });
    if (it == state.formulas.end()) {
        return false;
    }
    state.formulas.erase(it);
    return true;
}

bool replaceFormula(AppState& state, const CommonFormula& before, const CommonFormula& after) {
    for (auto& formula : state.formulas) {
        if (formula.seq == before.seq) {
            formula = after;
            return true;
        }
    }
    return false;
}

bool erasePrescription(AppState& state, const std::string& id) {
    const auto it = std::find_if(state.prescriptions.begin(), state.prescriptions.end(),
                                 [&](const Prescription& prescription) {
                                     return prescription.id == id;
                                 });
    if (it == state.prescriptions.end()) {
        return false;
    }
    state.prescriptions.erase(it);
    return true;
}

bool sameClinicalContent(const Prescription& lhs, const Prescription& rhs) {
    if (lhs.id != rhs.id || lhs.doctorId != rhs.doctorId || lhs.doses != rhs.doses ||
        lhs.items.size() != rhs.items.size()) {
        return false;
    }
    for (std::size_t i = 0; i < lhs.items.size(); ++i) {
        const auto& left = lhs.items[i];
        const auto& right = rhs.items[i];
        if (left.lineNo != right.lineNo || left.herbName != right.herbName ||
            left.spec != right.spec || left.doseHundredths != right.doseHundredths) {
            return false;
        }
    }
    return true;
}

bool replaceDraftPrescription(AppState& state, const Prescription& before,
                              const Prescription& after, std::string& error) {
    if (before.id != after.id) {
        error = "内部错误：修改处方编号不一致。";
        return false;
    }
    Prescription* current = findPrescription(state, before.id);
    if (current == nullptr) {
        error = "处方不存在：" + before.id;
        return false;
    }
    if (current->status != PrescriptionStatus::Draft) {
        error = "只有草稿处方可以修改。";
        return false;
    }
    if (current->doctorId != state.currentDoctor.id || after.doctorId != state.currentDoctor.id) {
        error = "只能修改当前医生自己的草稿处方。";
        return false;
    }
    if (after.status != PrescriptionStatus::Draft) {
        error = "编辑结果必须保持草稿状态。";
        return false;
    }
    if (!validatePrescriptionRecord(state, after, error)) {
        return false;
    }
    *current = after;
    return true;
}

}  // namespace

AddHerbCommand::AddHerbCommand(Herb herb) : herb_(std::move(herb)) {}

bool AddHerbCommand::execute(AppState& state, std::string& error) {
    if (!validateHerbRecord(state, herb_, "", error)) {
        return false;
    }
    state.herbs.push_back(herb_);
    return true;
}

bool AddHerbCommand::undo(AppState& state, std::string& error) {
    if (!eraseHerbByNumber(state, herb_.number)) {
        error = "撤销失败：找不到中药 " + herb_.number;
        return false;
    }
    return true;
}

std::string AddHerbCommand::description() const {
    return "新增中药 " + herb_.number + " " + herb_.name;
}

UpdateHerbCommand::UpdateHerbCommand(Herb before, Herb after)
    : before_(std::move(before)), after_(std::move(after)) {}

bool UpdateHerbCommand::execute(AppState& state, std::string& error) {
    if (findHerb(state, before_.name, before_.spec) == nullptr) {
        error = "要修改的中药不存在：" + herbLabel(before_.name, before_.spec);
        return false;
    }
    const bool keyChanged = before_.name != after_.name || before_.spec != after_.spec;
    if (keyChanged && isHerbReferenced(state, before_.name, before_.spec)) {
        error = "该中药已被常用方、处方或药量控制引用，不能修改名称或规格。";
        return false;
    }
    if (!validateHerbRecord(state, after_, before_.number, error)) {
        return false;
    }
    if (!replaceHerb(state, before_, after_)) {
        error = "修改失败：找不到原中药记录。";
        return false;
    }
    return true;
}

bool UpdateHerbCommand::undo(AppState& state, std::string& error) {
    if (!replaceHerb(state, after_, before_)) {
        error = "撤销失败：找不到修改后的中药记录。";
        return false;
    }
    return true;
}

std::string UpdateHerbCommand::description() const {
    return "修改中药 " + before_.number + " " + before_.name;
}

DeleteHerbCommand::DeleteHerbCommand(Herb herb) : herb_(std::move(herb)) {}

bool DeleteHerbCommand::execute(AppState& state, std::string& error) {
    if (findHerb(state, herb_.name, herb_.spec) == nullptr) {
        error = "要删除的中药不存在。";
        return false;
    }
    if (isHerbReferenced(state, herb_.name, herb_.spec)) {
        error = "该中药已被引用，不能删除。";
        return false;
    }
    if (!eraseHerbByNumber(state, herb_.number)) {
        error = "删除中药失败。";
        return false;
    }
    return true;
}

bool DeleteHerbCommand::undo(AppState& state, std::string& error) {
    if (findHerb(state, herb_.name, herb_.spec) != nullptr) {
        error = "撤销失败：同名同规格中药已存在。";
        return false;
    }
    state.herbs.push_back(herb_);
    return true;
}

std::string DeleteHerbCommand::description() const {
    return "删除中药 " + herb_.number + " " + herb_.name;
}

AddDoseLimitCommand::AddDoseLimitCommand(DoseLimit limit) : limit_(std::move(limit)) {}

bool AddDoseLimitCommand::execute(AppState& state, std::string& error) {
    if (findDoseLimit(state, limit_.herbName, limit_.spec) != nullptr) {
        error = "该中药和规格已存在药量控制。";
        return false;
    }
    if (!validateDoseLimitRecord(state, limit_, error)) {
        return false;
    }
    state.doseLimits.push_back(limit_);
    return true;
}

bool AddDoseLimitCommand::undo(AppState& state, std::string& error) {
    if (!eraseDoseLimit(state, limit_.herbName, limit_.spec)) {
        error = "撤销失败：找不到药量控制记录。";
        return false;
    }
    return true;
}

std::string AddDoseLimitCommand::description() const {
    return "新增药量控制 " + herbLabel(limit_.herbName, limit_.spec);
}

UpdateDoseLimitCommand::UpdateDoseLimitCommand(DoseLimit before, DoseLimit after)
    : before_(std::move(before)), after_(std::move(after)) {}

bool UpdateDoseLimitCommand::execute(AppState& state, std::string& error) {
    if (findDoseLimit(state, before_.herbName, before_.spec) == nullptr) {
        error = "要修改的药量控制不存在。";
        return false;
    }
    const bool keyChanged =
        before_.herbName != after_.herbName || before_.spec != after_.spec;
    if (keyChanged && findDoseLimit(state, after_.herbName, after_.spec) != nullptr) {
        error = "目标药物和规格已存在药量控制。";
        return false;
    }
    if (!validateDoseLimitRecord(state, after_, error)) {
        return false;
    }
    if (!replaceDoseLimit(state, before_, after_)) {
        error = "修改失败：找不到原药量控制。";
        return false;
    }
    return true;
}

bool UpdateDoseLimitCommand::undo(AppState& state, std::string& error) {
    if (!replaceDoseLimit(state, after_, before_)) {
        error = "撤销失败：找不到修改后的药量控制。";
        return false;
    }
    return true;
}

std::string UpdateDoseLimitCommand::description() const {
    return "修改药量控制 " + herbLabel(before_.herbName, before_.spec);
}

DeleteDoseLimitCommand::DeleteDoseLimitCommand(DoseLimit limit) : limit_(std::move(limit)) {}

bool DeleteDoseLimitCommand::execute(AppState& state, std::string& error) {
    if (!eraseDoseLimit(state, limit_.herbName, limit_.spec)) {
        error = "要删除的药量控制不存在。";
        return false;
    }
    return true;
}

bool DeleteDoseLimitCommand::undo(AppState& state, std::string& error) {
    if (findDoseLimit(state, limit_.herbName, limit_.spec) != nullptr) {
        error = "撤销失败：药量控制已存在。";
        return false;
    }
    state.doseLimits.push_back(limit_);
    return true;
}

std::string DeleteDoseLimitCommand::description() const {
    return "删除药量控制 " + herbLabel(limit_.herbName, limit_.spec);
}

AddFormulaCommand::AddFormulaCommand(CommonFormula formula) : formula_(std::move(formula)) {}

bool AddFormulaCommand::execute(AppState& state, std::string& error) {
    CommonFormula candidate = formula_;
    if (candidate.doctorId != state.currentDoctor.id) {
        error = "只能新增当前医生的个人常用方。";
        return false;
    }
    if (candidate.seq == 0) {
        candidate.seq = state.nextFormulaSeq;
    }
    if (findFormula(state, candidate.seq) != nullptr) {
        error = "常用方序号冲突，请重试。";
        return false;
    }
    if (!validateFormulaRecord(state, candidate, error)) {
        return false;
    }
    if (formula_.seq == 0) {
        ++state.nextFormulaSeq;
    }
    formula_ = candidate;
    state.formulas.push_back(formula_);
    return true;
}

bool AddFormulaCommand::undo(AppState& state, std::string& error) {
    if (!eraseFormula(state, formula_.seq)) {
        error = "撤销失败：找不到新增的常用方。";
        return false;
    }
    return true;
}

std::string AddFormulaCommand::description() const {
    if (formula_.seq == 0) {
        return "新增常用方 " + formula_.name;
    }
    return "新增常用方 " + makeFormulaCode(formula_.seq) + " " + formula_.name;
}

UpdateFormulaCommand::UpdateFormulaCommand(CommonFormula before, CommonFormula after,
                                           std::string detail)
    : before_(std::move(before)), after_(std::move(after)), detail_(std::move(detail)) {}

bool UpdateFormulaCommand::execute(AppState& state, std::string& error) {
    if (findFormula(state, before_.seq) == nullptr) {
        error = "要修改的常用方不存在。";
        return false;
    }
    if (before_.seq != after_.seq || after_.doctorId != state.currentDoctor.id) {
        error = "常用方归属或序号无效。";
        return false;
    }
    if (!validateFormulaRecord(state, after_, error)) {
        return false;
    }
    if (!replaceFormula(state, before_, after_)) {
        error = "修改失败：找不到原常用方。";
        return false;
    }
    return true;
}

bool UpdateFormulaCommand::undo(AppState& state, std::string& error) {
    if (!replaceFormula(state, after_, before_)) {
        error = "撤销失败：找不到修改后的常用方。";
        return false;
    }
    return true;
}

std::string UpdateFormulaCommand::description() const {
    return "修改常用方 " + makeFormulaCode(before_.seq) + " " + before_.name +
           "（" + detail_ + "）";
}

DeleteFormulaCommand::DeleteFormulaCommand(CommonFormula formula) : formula_(std::move(formula)) {}

bool DeleteFormulaCommand::execute(AppState& state, std::string& error) {
    if (formula_.doctorId != state.currentDoctor.id) {
        error = "只能删除当前医生的个人常用方。";
        return false;
    }
    if (!eraseFormula(state, formula_.seq)) {
        error = "要删除的常用方不存在。";
        return false;
    }
    return true;
}

bool DeleteFormulaCommand::undo(AppState& state, std::string& error) {
    if (findFormula(state, formula_.seq) != nullptr) {
        error = "撤销失败：常用方序号已存在。";
        return false;
    }
    state.formulas.push_back(formula_);
    return true;
}

std::string DeleteFormulaCommand::description() const {
    return "删除常用方 " + makeFormulaCode(formula_.seq) + " " + formula_.name;
}

AddPrescriptionCommand::AddPrescriptionCommand(Prescription prescription)
    : prescription_(std::move(prescription)) {}

bool AddPrescriptionCommand::execute(AppState& state, std::string& error) {
    Prescription candidate = prescription_;
    if (candidate.status != PrescriptionStatus::Draft) {
        error = "新增处方必须是草稿。";
        return false;
    }
    if (candidate.doctorId != state.currentDoctor.id) {
        error = "只能新增当前医生的处方。";
        return false;
    }
    if (candidate.id.empty()) {
        candidate.id = makePrescriptionId(state.nextPrescriptionSeq);
    }
    if (findPrescription(state, candidate.id) != nullptr) {
        error = "处方编号冲突，请重试。";
        return false;
    }
    if (candidate.createdAt.empty()) {
        candidate.createdAt = currentTimestamp();
    }
    if (candidate.updatedAt.empty()) {
        candidate.updatedAt = candidate.createdAt;
    }
    if (!validatePrescriptionRecord(state, candidate, error)) {
        return false;
    }
    if (prescription_.id.empty()) {
        ++state.nextPrescriptionSeq;
    }
    prescription_ = candidate;
    state.prescriptions.push_back(prescription_);
    return true;
}

bool AddPrescriptionCommand::undo(AppState& state, std::string& error) {
    if (!erasePrescription(state, prescription_.id)) {
        error = "撤销失败：找不到新增处方。";
        return false;
    }
    return true;
}

std::string AddPrescriptionCommand::description() const {
    if (prescription_.id.empty()) {
        return "新增处方";
    }
    return "新增处方 " + prescription_.id;
}

PrescriptionEditCommandBase::PrescriptionEditCommandBase(Prescription before, Prescription after,
                                                         std::string description)
    : before_(std::move(before)), after_(std::move(after)), description_(std::move(description)) {}

bool PrescriptionEditCommandBase::execute(AppState& state, std::string& error) {
    if (after_.updatedAt.empty()) {
        after_.updatedAt = currentTimestamp();
    }
    return replaceDraftPrescription(state, before_, after_, error);
}

bool PrescriptionEditCommandBase::undo(AppState& state, std::string& error) {
    if (after_.updatedAt.empty()) {
        after_.updatedAt = currentTimestamp();
    }
    return replaceDraftPrescription(state, after_, before_, error);
}

AddPrescriptionItemCommand::AddPrescriptionItemCommand(Prescription before, Prescription after,
                                                       std::string itemText)
    : PrescriptionEditCommandBase(std::move(before), std::move(after), "新增处方药味") {
    description_ = "新增处方药味 " + itemText;
}

std::string AddPrescriptionItemCommand::description() const { return description_; }

UpdatePrescriptionItemCommand::UpdatePrescriptionItemCommand(Prescription before, Prescription after,
                                                             std::string itemText)
    : PrescriptionEditCommandBase(std::move(before), std::move(after), "修改处方药味") {
    description_ = "修改处方药味 " + itemText;
}

std::string UpdatePrescriptionItemCommand::description() const { return description_; }

DeletePrescriptionItemCommand::DeletePrescriptionItemCommand(Prescription before, Prescription after,
                                                             std::string itemText)
    : PrescriptionEditCommandBase(std::move(before), std::move(after), "删除处方药味") {
    description_ = "删除处方药味 " + itemText;
}

std::string DeletePrescriptionItemCommand::description() const { return description_; }

UpdatePrescriptionDosesCommand::UpdatePrescriptionDosesCommand(Prescription before,
                                                               Prescription after, int beforeDoses,
                                                               int afterDoses)
    : PrescriptionEditCommandBase(std::move(before), std::move(after), "修改剂数"),
      beforeDoses_(beforeDoses),
      afterDoses_(afterDoses) {}

std::string UpdatePrescriptionDosesCommand::description() const {
    return "修改处方剂数 " + before_.id + "：" + std::to_string(beforeDoses_) + " -> " +
           std::to_string(afterDoses_);
}

ApplyFormulaCommand::ApplyFormulaCommand(Prescription before, Prescription after,
                                         std::string formulaName)
    : PrescriptionEditCommandBase(std::move(before), std::move(after), "应用常用方"),
      formulaName_(std::move(formulaName)) {}

std::string ApplyFormulaCommand::description() const {
    return "应用常用方 " + formulaName_ + " 到处方 " + before_.id;
}

SubmitPrescriptionCommand::SubmitPrescriptionCommand(Prescription before, Prescription after)
    : before_(std::move(before)), after_(std::move(after)) {}

bool SubmitPrescriptionCommand::execute(AppState& state, std::string& error) {
    Prescription* current = findPrescription(state, before_.id);
    if (current == nullptr) {
        error = "处方不存在：" + before_.id;
        return false;
    }
    if (current->status != PrescriptionStatus::Draft ||
        current->doctorId != state.currentDoctor.id) {
        error = "只能提交当前医生自己的草稿处方。";
        return false;
    }
    const SubmissionValidation validation = validatePrescriptionForSubmit(state, before_);
    if (!validation.ok()) {
        error = validation.errors.front();
        return false;
    }
    if (validation.hasOverdose() && after_.overdoseSignature.empty()) {
        error = "处方存在超量药物，必须由医生确认并签名。";
        return false;
    }
    if (after_.status != PrescriptionStatus::Submitted ||
        after_.doctorId != state.currentDoctor.id || !sameClinicalContent(before_, after_)) {
        error = "提交结果状态或处方内容无效。";
        return false;
    }
    if (!validatePrescriptionRecord(state, after_, error)) {
        return false;
    }
    if (!after_.overdoseSignature.empty() && after_.overdoseSignedAt.empty()) {
        after_.overdoseSignedAt = currentTimestamp();
    }
    if (after_.updatedAt.empty()) {
        after_.updatedAt = currentTimestamp();
    }
    *current = after_;
    return true;
}

bool SubmitPrescriptionCommand::undo(AppState& state, std::string& error) {
    Prescription* current = findPrescription(state, after_.id);
    if (current == nullptr || current->status != PrescriptionStatus::Submitted) {
        error = "撤销提交失败：处方不是已提交状态。";
        return false;
    }
    *current = before_;
    return true;
}

std::string SubmitPrescriptionCommand::description() const {
    return "提交处方 " + before_.id;
}

WithdrawPrescriptionCommand::WithdrawPrescriptionCommand(Prescription before, Prescription after)
    : before_(std::move(before)), after_(std::move(after)) {}

bool WithdrawPrescriptionCommand::execute(AppState& state, std::string& error) {
    Prescription* current = findPrescription(state, before_.id);
    if (current == nullptr || current->status != PrescriptionStatus::Submitted) {
        error = "只有已提交处方可以撤销。";
        return false;
    }
    if (current->doctorId != state.currentDoctor.id) {
        error = "只能撤销当前医生自己提交的处方。";
        return false;
    }
    if (after_.status != PrescriptionStatus::Withdrawn ||
        after_.doctorId != state.currentDoctor.id || !sameClinicalContent(before_, after_)) {
        error = "处方撤销结果无效。";
        return false;
    }
    if (after_.updatedAt.empty()) {
        after_.updatedAt = currentTimestamp();
    }
    *current = after_;
    return true;
}

bool WithdrawPrescriptionCommand::undo(AppState& state, std::string& error) {
    Prescription* current = findPrescription(state, after_.id);
    if (current == nullptr || current->status != PrescriptionStatus::Withdrawn) {
        error = "撤销失败：处方不是已撤销状态。";
        return false;
    }
    *current = before_;
    return true;
}

std::string WithdrawPrescriptionCommand::description() const {
    return "撤销处方（业务） " + before_.id;
}

RecoverPrescriptionCommand::RecoverPrescriptionCommand(Prescription before, Prescription after)
    : before_(std::move(before)), after_(std::move(after)) {}

bool RecoverPrescriptionCommand::execute(AppState& state, std::string& error) {
    Prescription* current = findPrescription(state, before_.id);
    if (current == nullptr || current->status != PrescriptionStatus::Withdrawn) {
        error = "只有已撤销处方可以恢复。";
        return false;
    }
    if (current->doctorId != state.currentDoctor.id) {
        error = "只能恢复当前医生自己的处方。";
        return false;
    }
    if (after_.status != PrescriptionStatus::Draft ||
        after_.doctorId != state.currentDoctor.id || !sameClinicalContent(before_, after_) ||
        !after_.overdoseSignature.empty()) {
        error = "处方恢复结果无效。";
        return false;
    }
    if (after_.updatedAt.empty()) {
        after_.updatedAt = currentTimestamp();
    }
    *current = after_;
    return true;
}

bool RecoverPrescriptionCommand::undo(AppState& state, std::string& error) {
    Prescription* current = findPrescription(state, after_.id);
    if (current == nullptr || current->status != PrescriptionStatus::Draft) {
        error = "撤销恢复失败：处方不是草稿状态。";
        return false;
    }
    *current = before_;
    return true;
}

std::string RecoverPrescriptionCommand::description() const {
    return "恢复处方为草稿 " + before_.id;
}

DeletePrescriptionCommand::DeletePrescriptionCommand(Prescription prescription)
    : prescription_(std::move(prescription)) {}

bool DeletePrescriptionCommand::execute(AppState& state, std::string& error) {
    Prescription* current = findPrescription(state, prescription_.id);
    if (current == nullptr) {
        error = "要删除的处方不存在。";
        return false;
    }
    if (current->status != PrescriptionStatus::Draft) {
        error = "只有草稿处方可以删除。";
        return false;
    }
    if (current->doctorId != state.currentDoctor.id) {
        error = "只能删除当前医生自己的草稿处方。";
        return false;
    }
    if (!erasePrescription(state, prescription_.id)) {
        error = "删除处方失败。";
        return false;
    }
    return true;
}

bool DeletePrescriptionCommand::undo(AppState& state, std::string& error) {
    if (findPrescription(state, prescription_.id) != nullptr) {
        error = "撤销失败：处方编号已存在。";
        return false;
    }
    state.prescriptions.push_back(prescription_);
    return true;
}

std::string DeletePrescriptionCommand::description() const {
    return "删除处方 " + prescription_.id;
}

}  // namespace tcm

