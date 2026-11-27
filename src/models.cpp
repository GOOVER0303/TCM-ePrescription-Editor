#include "models.h"

namespace tcm {

std::string statusToText(PrescriptionStatus status) {
    switch (status) {
        case PrescriptionStatus::Draft:
            return "草稿";
        case PrescriptionStatus::Submitted:
            return "已提交";
        case PrescriptionStatus::Withdrawn:
            return "已撤销";
    }
    return "未知";
}

bool statusFromText(const std::string& text, PrescriptionStatus& status) {
    if (text == "DRAFT") {
        status = PrescriptionStatus::Draft;
        return true;
    }
    if (text == "SUBMITTED") {
        status = PrescriptionStatus::Submitted;
        return true;
    }
    if (text == "WITHDRAWN") {
        status = PrescriptionStatus::Withdrawn;
        return true;
    }
    return false;
}

std::string statusToStorage(PrescriptionStatus status) {
    switch (status) {
        case PrescriptionStatus::Draft:
            return "DRAFT";
        case PrescriptionStatus::Submitted:
            return "SUBMITTED";
        case PrescriptionStatus::Withdrawn:
            return "WITHDRAWN";
    }
    return "DRAFT";
}

}  // namespace tcm
