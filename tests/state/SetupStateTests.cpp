#include "state/SetupState.h"

int main()
{
    using pmx::state::SetupState;
    using pmx::state::SetupStep;

    SetupState state;
    if (state.step() != SetupStep::welcome) return 1;
    if (state.continueToReady()) return 2;

    state.setDeviceStatus(true, true, true);
    if (state.step() != SetupStep::deviceFound) return 3;
    if (state.continueToReady()) return 4;

    state.setAudioTestPassed(true);
    if (! state.continueToReady()) return 5;
    if (state.step() != SetupStep::ready) return 6;
    if (! state.canFinish()) return 7;

    state.reset();
    state.setDeviceStatus(true, false, true);
    if (state.step() != SetupStep::welcome) return 8;
    if (state.canFinish()) return 9;
    return 0;
}
