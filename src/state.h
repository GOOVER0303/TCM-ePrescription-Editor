#pragma once

#include "models.h"

#include <string>
#include <vector>

namespace tcm {

const Herb* findHerb(const AppState& state, const std::string& name, const std::string& spec);
Herb* findHerb(AppState& state, const std::string& name, const std::string& spec);
const CommonFormula* findFormula(const AppState& state, int seq);
CommonFormula* findFormula(AppState& state, int seq);
const Prescription* findPrescription(const AppState& state, const std::string& id);
Prescription* findPrescription(AppState& state, const std::string& id);
const DoseLimit* findDoseLimit(const AppState& state, const std::string& name, const std::string& spec);
DoseLimit* findDoseLimit(AppState& state, const std::string& name, const std::string& spec);

bool validateRequiredText(const std::string& value, const std::string& label, std::string& error);
bool validateHerbRecord(const AppState& state, const Herb& herb, const std::string& ignoredNumber,
                        std::string& error);
bool validateDoseLimitRecord(const AppState& state, const DoseLimit& limit, std::string& error);
bool validateFormulaRecord(const AppState& state, const CommonFormula& formula, std::string& error);
bool validatePrescriptionRecord(const AppState& state, const Prescription& prescription,
                                std::string& error);
bool isHerbReferenced(const AppState& state, const std::string& name, const std::string& spec);
void renumberPrescriptionItems(Prescription& prescription);

struct SubmissionValidation {
    std::vector<std::string> errors;
    std::vector<std::string> overdoseWarnings;
    bool ok() const { return errors.empty(); }
    bool hasOverdose() const { return !overdoseWarnings.empty(); }
};

SubmissionValidation validatePrescriptionForSubmit(const AppState& state,
                                                    const Prescription& prescription);

}  // namespace tcm
