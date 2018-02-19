#ifndef _NSF_H_
#define _NSF_H_
#include "../soundData.h"

#define NSF_MAX_PATH 512

namespace xgm
{
  struct NSFE_Entry
  {
    char* tlbl;
    INT32 time;
    INT32 fade;
  };
  const unsigned int NSFE_ENTRIES = 256;

  /**
   * NSF file type
   */
  class NSF : public SoundDataMSP
  {
  public:
    char magic[5];
    UINT8 version;
    UINT8 songs;
    UINT8 total_songs;
    UINT8 start;
    UINT16 load_address;
    UINT16 init_address;
    UINT16 play_address;
    char filename[NSF_MAX_PATH];
    char print_title[256+64]; // margin 64 chars.
    char title_nsf[32];
    char artist_nsf[32];
    char copyright_nsf[32];
    char* title;
    char* artist;
    char* copyright;
    char* ripper; // NSFe only
    char* text; // NSFe only
    UINT32 text_len; // NSFe only
    UINT16 speed_ntsc;
    UINT8 bankswitch[8];
    UINT16 speed_pal;
    UINT8 pal_ntsc;
    UINT8 soundchip;
    bool use_vrc7;
    bool use_vrc6;
    bool use_fds;
    bool use_fme7;
    bool use_mmc5;
    bool use_n106;
    UINT8 extra[4];
    UINT8 *body;
    int bodysize;
    UINT8* nsfe_image;
    UINT8* nsfe_plst;
    int nsfe_plst_size;
    NSFE_Entry nsfe_entry[NSFE_ENTRIES];

    /** Currently selected song number */
    int song;
    /** If reading data from the playlist, this is true */
    bool playlist_mode;
    /** Playback time */
    int time_in_ms, default_playtime;
    /** Loop time */
    int loop_in_ms;
    /** Fade time */
    int fade_in_ms, default_fadetime;
    /** Loop count */
    int loop_num, default_loopnum;
    /** True if song length is unknown, and we are using the default playback time */
    bool playtime_unknown;
    bool title_unknown;

      NSF ();
     ~NSF ();
    /**
     * Reading the NSF file
     *
     * @param image A pointer to a binary image
     * @param size Size of the binary image
     * @return Success true, failure false
     */

    // loads file (playlist or NSF or NSFe)
    bool LoadFile (const char *fn);

    // loads NSF (or NSFe via LoadNSFe)
    bool Load (UINT8 * image, UINT32 size);

    // loads NSFe, if info is false INFO chunk is required to initialize
    bool LoadNSFe(UINT8* image, UINT32 size, bool info);

    void DebugOut ();
    /**
     * Get the title string
     *
     * @return Title string (composer - title)
     */
    char *GetTitleString (const char *format=NULL, int song=-1);
    char *GetPlaylistString (const char *format, bool b);
    int GetLength ();
    void SetTitleString (char *);
    void SetDefaults (int playtime, int fadetime, int loopnum);
    void SetLength (int time_in_ms);
    void SetSong (int);
    int GetSong ();
    int GetSongNum();

    int GetPlayTime ();
    int GetLoopTime ();
    int GetFadeTime ();
    int GetLoopNum ();

    bool UseNSFePlaytime();
  };

}                               // namespace 
#endif
