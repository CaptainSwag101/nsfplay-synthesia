#ifndef _LIBNSF_H_
#define _LIBNSF_H_
#include "../player.h"
#include "nsfconfig.h"
#include "nsf.h"

#include "../../devices/cpu/nes_cpu.h"
#include "../../devices/memory/nes_bank.h"
#include "../../devices/memory/nes_mem.h"
#include "../../devices/sound/nes_apu.h"
#include "../../devices/sound/nes_vrc7.h"
#include "../../devices/sound/nes_fme7.h"
#include "../../devices/sound/nes_vrc6.h"
#include "../../devices/sound/nes_dmc.h"
#include "../../devices/sound/nes_mmc5.h"
#include "../../devices/sound/nes_n106.h"
#include "../../devices/sound/nes_fds.h"
#include "../../devices/audio/filter.h"
#include "../../devices/audio/mixer.h"
#include "../../devices/audio/amplifier.h"
#include "../../devices/audio/rconv.h"
#include "../../devices/audio/echo.h"
#include "../../devices/audio/MedianFilter.h"
#include "../../devices/misc/nes_detect.h"
#include "../../devices/misc/log_cpu.h"

namespace xgm
{

  class NSFPlayer : public PlayerMSP
  {
  protected:
    NSFPlayerConfig *config;

    enum { PRE_CLICK, CLICKING, POST_CLICK };

    int click_mode;
    double rate;
    int nch; // number of channels
    int song;

    INT32 last_out;
    int silent_length;

    double cpu_clock_rest;
    double apu_clock_rest;

    int time_in_ms;             // Playback time (ms)
    bool playtime_detected;     // True if the play time was detected

    void Reload();
    void DetectLoop();
    void DetectSilent();
    void CheckTerminal();
    void UpdateInfo();

  public:
    Bus apu_bus;
    Layer stack;
    Layer layer;
    Mixer mixer;

    NES_CPU cpu;
    NES_MEM mem;
    NES_BANK bank;

    ISoundChip *sc[NES_DEVICE_MAX];      // Instance of the sound chip
    RateConverter rconv[NES_DEVICE_MAX]; // Sample rate converter
    Filter filter[NES_DEVICE_MAX];       // Low-pass converter
    Amplifier amp[NES_DEVICE_MAX];       // Amplifier
    DCFilter dcf;                        // DC filter to be applied to the final output stage
    Filter lpf;                          // A low-pass filter to be applied to the final output
    Compressor cmp;                      // Compressor to be applied to the final output stage
    ILoopDetector *ld;                   // Loop detector
    CPULogger *logcpu;                   // Logs CPU to file
    EchoUnit echo;
    MedianFilter *mfilter;               // Median filter of small noise measurements

    // Enumeration of the track number
    enum {
      APU1_TRK0=0, APU1_TRK1, 
      APU2_TRK0, APU2_TRK1, APU2_TRK2,
      FDS_TRK0,
      MMC5_TRK0, MMC5_TRK1, MMC5_TRK2,
      FME7_TRK0, FME7_TRK1, FME7_TRK2, FME7_TRK3, FME7_TRK4,
      VRC6_TRK0, VRC6_TRK1, VRC6_TRK2,
      VRC7_TRK0, VRC7_TRK1, VRC7_TRK2, VRC7_TRK3, VRC7_TRK4, VRC7_TRK5,
      N106_TRK0, N106_TRK1, N106_TRK2, N106_TRK3, N106_TRK4, N106_TRK5, N106_TRK6, N106_TRK7,
      NES_TRACK_MAX
    };
    InfoBuffer infobuf[NES_TRACK_MAX];   // Save the information of each track
    
    int total_render; // The number of bytes of this up to the generated waveform
    int frame_render; // The number of bytes in 1 frame
    int frame_in_ms;  // Length of one frame (ms)

    // Alias reference of each sound chip
    NES_APU *apu;
    NES_DMC *dmc;
    NES_VRC6 *vrc6;
    NES_VRC7 *vrc7;
    NES_FME7 *fme7;
    NES_MMC5 *mmc5;
    NES_N106 *n106;
    NES_FDS *fds;


  public:
    NSF *nsf;
    NSFPlayer();
    ~NSFPlayer();

    /** Set the configuration information */
    virtual void SetConfig(PlayerConfig *pc) ;

    /** Loads the data */
    virtual bool Load(SoundData * sdat);

    /** Sets the playback frequency */
    virtual void SetPlayFreq(double);

    /**
     * Number of channels to output.
     */
    virtual void SetChannels(int);

    /** Reset. Self rewriting of data is not restored even if generated in the previous performances. */
    virtual void Reset();

    /** Returns the number of the song currently playing */
    virtual int GetSong();

    /** Starts the fade-out */
    virtual void FadeOut(int fade_in_ms);

    /** Sets the song number to be played */
    virtual bool SetSong(int s);
    virtual bool PrevSong(int s);
    virtual bool NextSong(int s);

    /** Renders the data */
    virtual UINT32 Render(INT16 * b, UINT32 length);

    /** Skips the rendering */
    virtual UINT32 Skip(UINT32 length);

    /** Gets the song title */
    virtual char *GetTitleString();

    /** Gets the playback length */
    virtual int GetLength();

    /** Checks whether the playback length was automatically detected */
    virtual bool IsDetected();

    /** Checks whether playback is stopped */
    virtual bool IsStopped();

    /** Dumps the current NES memory state as a string */
    virtual void GetMemoryString(char *buf);   // Memory Dump as String

    /** Dumps the current NES memory state */
    virtual void GetMemory(UINT8 * buf);       // Memory Dump

    /** Callback to receive the notification to update the configuration information */
    virtual void Notify(int id);

    // Notify for panning
    virtual void NotifyPan(int id);

    /** Gets the device information in time_in_ms time */
    virtual IDeviceInfo *GetInfo(int time_in_ms, int device_id);

    /** Whether to use PAL/NTSC/Dendy based on flags and REGION config */
    virtual int GetRegion(UINT8 flags);
    enum {
        REGION_NTSC = 0,
        REGION_PAL,
        REGION_DENDY
    };
  };

}// namespace

#endif
