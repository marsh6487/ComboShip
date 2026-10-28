#include "global.h"

void MtxConv_F2L(Mtx* mtx, MtxF* mf) {
#if defined(GBI_FLOATS) || defined(GBI_FLOAT_MTX)
    guMtxF2L(mf->mf, mtx);
#else
    s32 i;
    s32 j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            s32 value = (mf->mf[i][j] * 0x10000);

            mtx->intPart[i][j] = value >> 16;
            mtx->fracPart[i][j] = value;
        }
    }
#endif
}

void MtxConv_L2F(MtxF* mtx, Mtx* mf) {
    guMtxL2F(mtx->mf, mf);
}
