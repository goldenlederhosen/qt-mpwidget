#include "mpstate.h"

static_var const char *const MpState_2_asciidesc[MpState_maxidx] = {
    "NotStartedState",
    "IdleState",
    "LoadingState",
    "StoppedState",
    "PlayingState",
    "BufferingState",
    "PausedState",
    "ErrorState"
};

char const *convert_MpState_2_asciidesc(const MpState s)
{
    return MpState_2_asciidesc[MpState_2_idx(s)];
}

char const *convert_MpStateidx_2_asciidesc(const unsigned u)
{
    return MpState_2_asciidesc[u];
}

