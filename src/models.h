#pragma once

#include <string>
#include <vector>

namespace tcm {

struct Doctor {
    std::string id;
    std::string name;
    std::string department;
};

struct Herb {
    std::string number;
    std::string name;
    std::string origin;
    std::string spec;
};

struct DoseLimit {
    std::string herbName;
    std::string spec;
    long long maxHundredths{0};
};

struct FormulaItem {
    std::string herbName;
    std::string spec;
    long long doseHundredths{0};
};

struct CommonFormula {
    int seq{0};
    std::string doctorId;
    std::string name;
    std::string department;
    std::vector<FormulaItem> items;
};

struct PrescriptionItem {
    int lineNo{0};
    std::string herbName;
    std::string spec;
    long long doseHundredths{0};
};

enum class PrescriptionStatus {
    Draft,
    Submitted,
    Withdrawn
};

struct Prescription {
    std::string id;
    std::string doctorId;
    std::string doctorName;
    int doses{1};
    PrescriptionStatus status{PrescriptionStatus::Draft};
    std::vector<PrescriptionItem> items;
    std::string overdoseSignature;
    std::string overdoseSignedAt;
    std::string createdAt;
    std::string updatedAt;
};

struct AppState {
    Doctor currentDoctor;
    std::vector<Herb> herbs;
    std::vector<DoseLimit> doseLimits;
    std::vector<CommonFormula> formulas;
    std::vector<Prescription> prescriptions;
    int nextFormulaSeq{1};
    int nextPrescriptionSeq{1};
};

std::string statusToText(PrescriptionStatus status);
bool statusFromText(const std::string& text, PrescriptionStatus& status);
std::string statusToStorage(PrescriptionStatus status);

}  // namespace tcm
