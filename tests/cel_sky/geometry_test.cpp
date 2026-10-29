// The expanded runtime's sky writers must preserve geometry outside s16.
#include <cstdint>
#include <cstdio>
using u8 = uint8_t; using s16 = int16_t; using s32 = int32_t;
struct Vec3f { float x, y, z; };
struct Vtx { struct { s32 ob[3]; uint16_t flag; s16 tc[2]; u8 cn[4]; } v; };
/* PRODUCTION_CLOUD_WRITER */
/* PRODUCTION_WISP_WRITER */
int main() {
    Vtx cloud{}, wisp{}; const u8 rgba[] = {12, 34, 56, 78};
    WriteVtx(&cloud, {50000, -40000, 75000}, 32, 16, rgba);
    if (cloud.v.ob[0] != 50000 || cloud.v.ob[1] != -40000 || cloud.v.ob[2] != 75000 ||
        cloud.v.tc[0] != 1024 || cloud.v.tc[1] != 512 || cloud.v.cn[3] != 78) {
        std::fputs("FAIL: cloud writer narrows wide geometry or loses material data\n", stderr); return 1;
    }
    WriteWispVtx(&wisp, -60000, 80000, 40000, rgba, 99);
    if (wisp.v.ob[0] != -60000 || wisp.v.ob[1] != 80000 || wisp.v.ob[2] != 40000 || wisp.v.cn[3] != 99) {
        std::fputs("FAIL: wisp writer narrows wide geometry or loses opacity\n", stderr); return 1;
    }
    std::puts("cel sky wide geometry: PASS");
}
