#pragma once
#include "MRMesh/MRMeshFwd.h"
#include "MRHistoryAction.h"
#include <memory>

namespace MR
{

class MRMESH_CLASS CombinedHistoryAction : public HistoryAction
{
public:
    // Will call action() for each actions in given order (undo in reverse, redo in forward)
    MRMESH_API CombinedHistoryAction( const std::string& name, const std::vector<std::shared_ptr<HistoryAction>>& actions );

    virtual std::string name() const override
    {
        return name_;
    }

    MRMESH_API virtual void action( HistoryAction::Type type ) override;

    /// Remove some actions according to condition inside combined actions.
    /// Do deep filtering.
    MRMESH_API bool filter( HistoryStackFilter filteringCondition );

    bool empty() const { return actions_.empty(); };
private:
    HistoryActionsVector actions_;
    std::string name_;
};

}
