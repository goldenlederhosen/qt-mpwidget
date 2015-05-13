#ifndef MPSTATE_H
#define MPSTATE_H

#include <type_traits>

// Warning: needs to be in sync with literal arrays in .cpp file
enum class MpState {
    NotStartedState,
    IdleState,
    LoadingState,
    StoppedState,
    PlayingState,
    BufferingState,
    PausedState,
    ErrorState
};

typedef std::underlying_type<MpState>::type MpState_utype;

constexpr inline unsigned MpState_2_idx(const MpState s)
{
    return static_cast<MpState_utype>(s) - static_cast<MpState_utype>(MpState::NotStartedState);
}

constexpr inline MpState idx_2_MpState(const unsigned i)
{
    return static_cast<MpState>(
               static_cast<MpState_utype>(i) + static_cast<MpState_utype>(MpState::NotStartedState)
           );
}

const unsigned MpState_maxidx = MpState_2_idx(MpState::ErrorState) + 1;

char const *convert_MpState_2_asciidesc(const MpState s);

char const *convert_MpStateidx_2_asciidesc(const unsigned u);

#endif /* MPSTATE_H */
