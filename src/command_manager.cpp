#include "command_manager.h"

#include <utility>

namespace tcm {

CommandManager::CommandManager(SaveCallback saveCallback)
    : saveCallback_(std::move(saveCallback)) {}

bool CommandManager::execute(std::unique_ptr<ICommand> command, AppState& state,
                             std::string& error) {
    if (!command) {
        error = "内部错误：命令为空。";
        return false;
    }
    AppState before = state;
    if (!command->execute(state, error)) {
        state = std::move(before);
        return false;
    }
    if (!saveCallback_(state, error)) {
        state = std::move(before);
        error = "数据保存失败，操作已回滚：" + error;
        return false;
    }
    undoStack_.push(std::move(command));
    redoStack_.clear();
    if (undoStack_.size() > kMaxHistory) {
        undoStack_.removeOldest();
    }
    return true;
}

bool CommandManager::undo(AppState& state, std::string& error) {
    ICommand* command = undoStack_.peek() == nullptr ? nullptr : undoStack_.peek()->get();
    if (command == nullptr) {
        error = "没有可以撤销的操作。";
        return false;
    }
    AppState before = state;
    if (!command->undo(state, error)) {
        state = std::move(before);
        return false;
    }
    if (!saveCallback_(state, error)) {
        state = std::move(before);
        error = "数据保存失败，撤销已回滚：" + error;
        return false;
    }
    auto moved = undoStack_.pop();
    redoStack_.push(std::move(*moved));
    return true;
}

bool CommandManager::redo(AppState& state, std::string& error) {
    ICommand* command = redoStack_.peek() == nullptr ? nullptr : redoStack_.peek()->get();
    if (command == nullptr) {
        error = "没有可以重做的操作。";
        return false;
    }
    AppState before = state;
    if (!command->execute(state, error)) {
        state = std::move(before);
        return false;
    }
    if (!saveCallback_(state, error)) {
        state = std::move(before);
        error = "数据保存失败，重做已回滚：" + error;
        return false;
    }
    auto moved = redoStack_.pop();
    undoStack_.push(std::move(*moved));
    return true;
}

void CommandManager::clear() {
    undoStack_.clear();
    redoStack_.clear();
}

std::vector<std::string> CommandManager::undoDescriptions() const {
    std::vector<std::string> result;
    undoStack_.forEachTopDown([&](const std::unique_ptr<ICommand>& command) {
        result.push_back(command->description());
    });
    return result;
}

std::vector<std::string> CommandManager::redoDescriptions() const {
    std::vector<std::string> result;
    redoStack_.forEachTopDown([&](const std::unique_ptr<ICommand>& command) {
        result.push_back(command->description());
    });
    return result;
}

bool CommandManager::checkInvariants() const {
    return undoStack_.checkInvariants() && redoStack_.checkInvariants();
}

}  // namespace tcm
