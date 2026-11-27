#pragma once

#include "command_manager.h"
#include "models.h"
#include "storage.h"

namespace tcm {

class Cli {
public:
    Cli(AppState& state, Storage& storage, CommandManager& commands);

    int run();

private:
    void printHeader() const;
    void printMainMenu() const;
    void showStatistics() const;

    void commonFormulaMenu();
    void prescriptionMenu();
    void herbMenu();
    void doseLimitMenu();
    void historyMenu();
    void systemMenu();

    void listFormulas() const;
    void showFormula(int seq) const;
    bool chooseFormula(int& seq) const;
    void addFormula();
    void modifyFormula();
    void deleteFormula();
    void applyFormulaToPrescription();

    void listPrescriptions() const;
    void showPrescription(const std::string& id) const;
    bool chooseOwnedPrescription(PrescriptionStatus status, std::string& id) const;
    void createPrescription();
    void editDraftPrescription();
    void submitPrescription();
    void withdrawPrescription();
    void recoverPrescription();
    void deletePrescription();
    void showValidation(const Prescription& prescription) const;

    void listHerbs() const;
    void addHerb();
    void modifyHerb();
    void deleteHerb();

    void listDoseLimits() const;
    void addDoseLimit();
    void modifyDoseLimit();
    void deleteDoseLimit();

    bool executeCommand(std::unique_ptr<ICommand> command);
    bool promptText(const std::string& label, std::string& value, bool allowEmpty = false) const;
    bool promptDefaultText(const std::string& label, const std::string& defaultValue,
                           std::string& value) const;
    bool promptInt(const std::string& label, int minValue, int maxValue, int& value) const;
    bool promptDose(const std::string& label, long long& hundredths) const;
    bool confirm(const std::string& question) const;
    bool selectHerb(const std::string& initialName, std::string& name, std::string& spec) const;
    bool inputFormulaItem(std::vector<FormulaItem>& items) const;
    bool inputPrescriptionItem(Prescription& prescription, int editingIndex) const;
    std::string prescriptionItemText(const PrescriptionItem& item) const;

    AppState& state_;
    Storage& storage_;
    CommandManager& commands_;
};

}  // namespace tcm
