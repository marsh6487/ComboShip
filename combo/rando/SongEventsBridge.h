#pragma once
#ifdef __cplusplus
extern "C" {
#endif
int SongEvents_OotAction(int scene, int room, int spawn, int vanillaAction);
int SongEvents_OotIsEventAction(int action);
int SongEvents_OotShouldCapture(int action, int nativeSong);
int SongEvents_MmSong(int event, int vanillaSong);
int SongEvents_MmInterceptSong(int nativeSong);
int SongEvents_OotClockLocked(void);
int SongEvents_OotSunSongTimeLocked(void);
void SongEvents_OotApplyClockLock(void);
#ifdef __cplusplus
}
#endif
