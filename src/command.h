#pragma once

#include "models.h"

#include <string>

namespace tcm {

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual bool execute(AppState& state, std::string& error) = 0;
    virtual bool undo(AppState& state, std::string& error) = 0;
    virtual std::string description() const = 0;
};

}  // namespace tcm
