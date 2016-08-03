#ifndef _SOUNDDATA_H_
#define _SOUNDDATA_H_

#include "../xtypes.h"
#include "../debugout.h"

namespace xgm
{
  /**
   * Sequence type performance data
   */
  class SoundData
  {
  public:
    SoundData(){};
    virtual ~SoundData(){};
    /**
     * Read from the binary image
     *
     * @param image A pointer to a binary image
     * @param size  The size of the image
     * @return Success true failure false
     */
    virtual bool Load(UINT8 *data, UINT32 size){ return false; }

    /**
     * Read from a file
     * @param fn Pointer of the file name to (or a string that indicates the file information)
     */
    virtual bool LoadFile(const char *fn){ return false; }

    /**
     * Acquisition of title
     * @return Title information
     */
    virtual char *GetTitleString(const char *format=NULL){ return ""; }

    /**
     * title setting
     * @param title A pointer to the new title (up to 255 characters)
     */
    virtual void SetTitleString(char *title){}

    /**
     * Get the playback time (ms)
     */
    virtual int GetLength(){ return 3*60*1000; }

    /**
     * Set the playback length (ms)
     */
    virtual void SetLength(int time_in_ms){}
  };

  /**
   * Multiple songs containing performance data
   */
  class SoundDataMSP : public SoundData
  {
  public:
    bool enable_multi_tracks;
    SoundDataMSP() : enable_multi_tracks(false){}
    virtual ~SoundDataMSP(){}
    virtual int GetSong()=0;
    virtual void SetSong(int)=0;
    virtual int GetSongNum()=0;
  };
}

#endif