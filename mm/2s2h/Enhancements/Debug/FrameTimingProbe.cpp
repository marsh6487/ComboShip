// Compile a module-local instance of the proven recorder. Both game libraries
// already hide their internal engine symbols on ELF and do not export these
// bridges on Windows. Capture budgets and files therefore belong to each host.
#define FRAME_TIMING_HOST_LABEL "MM"
#define FRAME_TIMING_CAPTURE_SCOPE "MM BlackMarket interior only; live active gameplay; actor attribution unavailable"
#include "../../../../soh/soh/Enhancements/debugger/FrameTimingProbe.cpp"
