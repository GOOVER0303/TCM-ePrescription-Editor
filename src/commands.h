#pragma once

#include "command.h"

#include <string>

namespace tcm {

class AddHerbCommand : public ICommand {
public:
    explicit AddHerbCommand(Herb herb);
    bool execute(AppState& state, std::string& error) override;
    bool undo(AppState& state, std::string& error) override;
    std::string description() const override;

private:
    Herb herb_;
};

class UpdateHerbCommand : public ICommand {
public:
    UpdateHerbCommand(Herb before, Herb after);
    bool execute(AppState& state, std::string& error) override;
    bool undo(AppState& state, std::string& error) override;
    std::string description() const override;

private:
    Herb before_;
    Herb after_;
};

class DeleteHerbCommand : public ICommand {
public:
    explicit DeleteHerbCommand(Herb herb);
    bool execute(AppState& state, std::string& error) override;
    bool undo(AppState& state, std::string& error) override;
    std::string description() const override;

private:
    Herb herb_;
};

class AddDoseLimitCommand : public ICommand {
public:
    explicit AddDoseLimitCommand(DoseLimit limit);
    bool execute(AppState& state, std::string& error) override;
    bool undo(AppState& state, std::string& error) override;
    std::string description() const override;

private:
    DoseLimit limit_;
};

class UpdateDoseLimitCommand : public ICommand {
public:
    UpdateDoseLimitCommand(DoseLimit before, DoseLimit after);
    bool execute(AppState& state, std::string& error) override;
    bool undo(AppState& state, std::string& error) override;
    std::string description() const override;

private:
    DoseLimit before_;
    DoseLimit after_;
};

class DeleteDoseLimitCommand : public ICommand {
public:
    explicit DeleteDoseLimitCommand(DoseLimit limit);
    bool execute(AppState& state, std::string& error) override;
    bool undo(AppState& state, std::string& error) override;
    std::string description() const override;

private:
    DoseLimit limit_;
};

class AddFormulaCommand : public ICommand {
public:
    explicit AddFormulaCommand(CommonFormula formula);
    bool execute(AppState& state, std::string& error) override;
    bool undo(AppState& state, std::string& error) override;
    std::string description() const override;

private:
    CommonFormula formula_;
};

class UpdateFormulaCommand : public ICommand {
public:
    UpdateFormulaCommand(CommonFormula before, CommonFormula after, std::string detail);
    bool execute(AppState& state, std::string& error) override;
    bool undo(AppState& state, std::string& error) override;
    std::string description() const override;

private:
    CommonFormula before_;
    CommonFormula after_;
    std::string detail_;
};

class DeleteFormulaCommand : public ICommand {
public:
    explicit DeleteFormulaCommand(CommonFormula formula);
    bool execute(AppState& state, std::string& error) override;
    bool undo(AppState& state, std::string& error) override;
    std::string description() const override;

private:
    CommonFormula formula_;
};

class AddPrescriptionCommand : public ICommand {
public:
    explicit AddPrescriptionCommand(Prescription prescription);
    bool execute(AppState& state, std::string& error) override;
    bool undo(AppState& state, std::string& error) override;
    std::string description() const override;

private:
    Prescription prescription_;
};

class PrescriptionEditCommandBase : public ICommand {
public:
    bool execute(AppState& state, std::string& error) override;
    bool undo(AppState& state, std::string& error) override;

protected:
    PrescriptionEditCommandBase(Prescription before, Prescription after, std::string description);

    Prescription before_;
    Prescription after_;
    std::string description_;
};

class AddPrescriptionItemCommand : public PrescriptionEditCommandBase {
public:
    AddPrescriptionItemCommand(Prescription before, Prescription after, std::string itemText);
    std::string description() const override;
};

class UpdatePrescriptionItemCommand : public PrescriptionEditCommandBase {
public:
    UpdatePrescriptionItemCommand(Prescription before, Prescription after, std::string itemText);
    std::string description() const override;
};

class DeletePrescriptionItemCommand : public PrescriptionEditCommandBase {
public:
    DeletePrescriptionItemCommand(Prescription before, Prescription after, std::string itemText);
    std::string description() const override;
};

class UpdatePrescriptionDosesCommand : public PrescriptionEditCommandBase {
public:
    UpdatePrescriptionDosesCommand(Prescription before, Prescription after, int beforeDoses,
                                   int afterDoses);
    std::string description() const override;

private:
    int beforeDoses_;
    int afterDoses_;
};

class ApplyFormulaCommand : public PrescriptionEditCommandBase {
public:
    ApplyFormulaCommand(Prescription before, Prescription after, std::string formulaName);
    std::string description() const override;

private:
    std::string formulaName_;
};

class SubmitPrescriptionCommand : public ICommand {
public:
    SubmitPrescriptionCommand(Prescription before, Prescription after);
    bool execute(AppState& state, std::string& error) override;
    bool undo(AppState& state, std::string& error) override;
    std::string description() const override;

private:
    Prescription before_;
    Prescription after_;
};

class WithdrawPrescriptionCommand : public ICommand {
public:
    WithdrawPrescriptionCommand(Prescription before, Prescription after);
    bool execute(AppState& state, std::string& error) override;
    bool undo(AppState& state, std::string& error) override;
    std::string description() const override;

private:
    Prescription before_;
    Prescription after_;
};

class RecoverPrescriptionCommand : public ICommand {
public:
    RecoverPrescriptionCommand(Prescription before, Prescription after);
    bool execute(AppState& state, std::string& error) override;
    bool undo(AppState& state, std::string& error) override;
    std::string description() const override;

private:
    Prescription before_;
    Prescription after_;
};

class DeletePrescriptionCommand : public ICommand {
public:
    explicit DeletePrescriptionCommand(Prescription prescription);
    bool execute(AppState& state, std::string& error) override;
    bool undo(AppState& state, std::string& error) override;
    std::string description() const override;

private:
    Prescription prescription_;
};

}  // namespace tcm
