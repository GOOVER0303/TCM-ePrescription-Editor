#pragma once

#include "command.h"
#include "linked_stack.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tcm {

using SaveCallback = std::function<bool(const AppState&, std::string&)>;

class CommandManager {
public:
    static constexpr std::size_t kMaxHistory = 100;

    explicit CommandManager(SaveCallback saveCallback);

    bool execute(std::unique_ptr<ICommand> command, AppState& state, std::string& error);
    bool undo(AppState& state, std::string& error);
    bool redo(AppState& state, std::string& error);
    void clear();

    std::size_t undoSize() const { return undoStack_.size(); }
    std::size_t redoSize() const { return redoStack_.size(); }
    std::vector<std::string> undoDescriptions() const;
    std::vector<std::string> redoDescriptions() const;
    bool checkInvariants() const;

private:
    SaveCallback saveCallback_;
    DoublyLinkedStack<std::unique_ptr<ICommand>> undoStack_;
    DoublyLinkedStack<std::unique_ptr<ICommand>> redoStack_;
};

}  // namespace tcm
