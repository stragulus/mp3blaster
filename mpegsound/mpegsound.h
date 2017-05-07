/* MPEG/WAVE Sound library

   (C) 1997 by Woo-jae Jung */

// Mpegsound.h
//   This is typeset for functions in MPEG/WAVE Sound library.
//   Nasplayer class added by Willem (willem@stack.nl), thanks!

/************************************/
/* Inlcude default library packages */
/************************************/
#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_BOOL_H
//#include <bool.h>
#endif
#ifdef LIBPTH
# include <pth.h>
#elif defined(PTHREADEDMPEG)
# include <pthread.h>
#endif

#ifndef _L__SOUND__
#define _L__SOUND__

/* Not all OSS implementations define an endian-independant samplesize. 
 * This code is taken from linux' <sys/soundcard.h, OSS version 0x030802
 */
#ifndef AFMT_S16_NE
#ifdef WORDS_BIGENDIAN
#  define _PATCHKEY(id) (0xfd00|id)
#  define AFMT_S16_NE AFMT_S16_BE
#else
#  define _PATCHKEY(id) ((id<<8)|0xfd)
#  define AFMT_S16_NE AFMT_S16_LE
#endif
#endif

#ifdef LIBPTH
#define USLEEP(x) pth_usleep(x)
#else
#define USLEEP(x) usleep(x)
#endif

/*\ ncurses #defines a lot of functions.  That sucks, because it
|*| clashes with other things, such as C++ iostream stuff.
|*| Basically, you can't just use C++ iostreams and ncurses together.
\*/
#undef clear
#undef bool

/****************/
/* Sound Errors */
/****************/
// General error
#define SOUND_ERROR_OK                0
#define SOUND_ERROR_FINISH           -1

// Device error (for player)
#define SOUND_ERROR_DEVOPENFAIL       1
#define SOUND_ERROR_DEVBUSY           2
#define SOUND_ERROR_DEVBADBUFFERSIZE  3
#define SOUND_ERROR_DEVCTRLERROR      4

// Sound file (for reader)
#define SOUND_ERROR_FILEOPENFAIL      5
#define SOUND_ERROR_FILEREADFAIL      6

// Network
#define SOUND_ERROR_UNKNOWNPROXY      7
#define SOUND_ERROR_UNKNOWNHOST       8
#define SOUND_ERROR_SOCKET            9
#define SOUND_ERROR_CONNECT          10
#define SOUND_ERROR_FDOPEN           11
#define SOUND_ERROR_HTTPFAIL         12
#define SOUND_ERROR_HTTPWRITEFAIL    13
#define SOUND_ERROR_TOOMANYRELOC     14

// Miscellneous (for translater)
#define SOUND_ERROR_MEMORYNOTENOUGH  15
#define SOUND_ERROR_EOF              16
#define SOUND_ERROR_BAD              17

#define SOUND_ERROR_THREADFAIL       18

#define SOUND_ERROR_BADHEADER        19

#define SOUND_ERROR_UNKNOWN          20


/**************************/
/* Define values for MPEG */
/**************************/
#define SCALEBLOCK     12
#define CALCBUFFERSIZE 512
#define MAXSUBBAND     32
#define MAXCHANNEL     2
#define MAXTABLE       2
#define SCALE          32768
#define MAXSCALE       (SCALE-1)
#define MINSCALE       (-SCALE)
#define RAWDATASIZE    (2*2*32*SSLIMIT)

#define LS 0
#define RS 1

#define SSLIMIT      18
#define SBLIMIT      32

#define WINDOWSIZE    4096

// Huffmancode
#define HTN 34


/*******************************************/
/* Define values for Microsoft WAVE format */
/*******************************************/
#ifdef WORDS_BIGENDIAN

#define RIFF            0x52494646
#define WAVE            0x57415645
#define FMT             0x666D7420
#define DATA            0x64617461
#define PCM_CODE        (1 << 8)
#define WAVE_MONO       (1 << 8)
#define WAVE_STEREO     (2 << 8)

#define MODE_MONO   0
#define MODE_STEREO 1

#else

#define RIFF		0x46464952
#define WAVE		0x45564157
#define FMT		0x20746D66
#define DATA		0x61746164
#define PCM_CODE	1
#define WAVE_MONO	1
#define WAVE_STEREO	2

#define MODE_MONO   0
#define MODE_STEREO 1

#endif

/********************/
/* Type definitions */
/********************/
typedef float REAL;

#ifdef NEWTHREAD
struct  mpeg_buffer {
    short int data[RAWDATASIZE];
    long framenumber;
    int framesize;
    short claimed;
};

struct buffer_node {
    short index;
    buffer_node *next;
};
#endif

enum soundtype { ST_NONE, ST_RAW, ST_WAV };

typedef struct _waveheader {
  u_int32_t     main_chunk;  // 'RIFF'
  u_int32_t     length;      // filelen
  u_int32_t     chunk_type;  // 'WAVE'

  u_int32_t     sub_chunk;   // 'fmt '
  u_int32_t     sc_len;      // length of sub_chunk, =16
  u_int16_t     format;      // should be 1 for PCM-code
  u_int16_t     modus;       // 1 Mono, 2 Stereo
  u_int32_t     sample_fq;   // frequence of sample
  u_int32_t     byte_p_sec;
  u_int16_t     byte_p_spl;  // samplesize; 1 or 2 bytes
  u_int16_t     bit_p_spl;   // 8, 12 or 16 bit

  u_int32_t     data_chunk;  // 'data'
  u_int32_t     data_length; // samplecount
}WAVEHEADER;

typedef struct
{
  bool         generalflag;
  unsigned int part2_3_length;
  unsigned int big_values;
  unsigned int global_gain;
  unsigned int scalefac_compress;
  unsigned int window_switching_flag;
  unsigned int block_type;
  unsigned int mixed_block_flag;
  unsigned int table_select[3];
  unsigned int subblock_gain[3];
  unsigned int region0_count;
  unsigned int region1_count;
  unsigned int preflag;
  unsigned int scalefac_scale;
  unsigned int count1table_select;
}layer3grinfo;

typedef struct
{
  unsigned main_data_begin;
  unsigned private_bits;
  struct
  {
    unsigned scfsi[4];
    layer3grinfo gr[2];
  }ch[2];
}layer3sideinfo;

typedef struct
{
  int l[23];            /* [cb] */
  int s[3][13];         /* [window][cb] */
}layer3scalefactor;     /* [ch] */

typedef struct
{
  int tablename;
  unsigned int xlen,ylen;
  unsigned int linbits;
  unsigned int treelen;
  const unsigned int (*val)[2];
}HUFFMANCODETABLE;

//TODO: remove mp3-specific stuff out of this struct
struct song_info
{
	char songname[31];
	char artist[31];
	char comment[31];
	char year[5];
	char album[31];
	unsigned char genre;
	char mode[20];
	int bitrate;
	int mp3_layer;
	int mp3_version;
	int samplerate;
	int totaltime;
};

/*********************************/
/* Sound input interface classes */
/*********************************/
// Superclass for Inputbitstream // Yet, Temporary
class Soundinputstream
{
public:
  Soundinputstream();
  virtual ~Soundinputstream();

  static Soundinputstream *hopen(const char *filename,int *errcode);

  int geterrorcode(void)  {return __errorcode;};

  virtual bool open(const char *filename)              =0;
  virtual void close(void)                       =0;
  virtual int  getbytedirect(void)               =0;
  virtual bool _readbuffer(char *buffer,int size)=0;
  virtual bool eof(void)                         =0;
  virtual int  getblock(char *buffer,int size)   =0;

  virtual int  getsize(void)                     =0;
  virtual int  getposition(void)                 =0;
  virtual void setposition(int pos)              =0;

  bool canseek(void) { return __canseek; }

protected:
  void seterrorcode(int errorcode) {__errorcode=errorcode;};
  bool __canseek;

private:
  int __errorcode;
};

// Inputstream from file
class Soundinputstreamfromfile : public Soundinputstream
{
public:
  Soundinputstreamfromfile();
  ~Soundinputstreamfromfile();

  bool open(const char *filename);
  void close(void);
  bool _readbuffer(char *buffer,int bytes);
  int  getbytedirect(void);
  bool eof(void);
  int  getblock(char *buffer,int size);

  int  getsize(void);
  int  getposition(void);
  void setposition(int pos);

private:
  FILE *fp;
  int  size;
};

// Inputstream from http
class Soundinputstreamfromhttp : public Soundinputstream
{
public:
  Soundinputstreamfromhttp();
  ~Soundinputstreamfromhttp();

  bool open(const char *filename);
  void close(void) {}
  bool _readbuffer(char *buffer,int bytes);
  int  getbytedirect(void);
  bool eof(void);
  int  getblock(char *buffer,int size);

  int  getsize(void);
  int  getposition(void);
  void setposition(int pos);

private:
  FILE *fp;
  int  size;

  bool writestring(int fd,char *string);
  bool readstring(char *string,int maxlen,FILE *f);
  FILE *http_open(const char *url);
};


/**********************************/
/* Sound player interface classes */
/**********************************/
// Superclass for player
class Soundplayer
{
public:
  Soundplayer() {__errorcode=SOUND_ERROR_OK;};
  virtual ~Soundplayer();

  virtual void abort(void);

  virtual bool setsoundtype(int stereo,int samplesize,int speed)=0;
  virtual void set8bitmode()=0;
  virtual bool resetsoundtype(void);
  virtual void releasedevice(void) = 0;
  virtual bool attachdevice(void) = 0;

  virtual bool putblock(void *buffer,int size)                  =0;
  virtual int  putblock_nt(void *buffer, int size)		=0;
  virtual int  getblocksize(void);

  int geterrorcode(void) {return __errorcode;};
  int audiohandle;  
  virtual int fix_samplesize(void *buffer, int size);
protected:
  bool seterrorcode(int errorno) {__errorcode=errorno; return false;};

private:
  int  __errorcode;
};


class Rawtofile : public Soundplayer
{
public:
  ~Rawtofile();

  static Rawtofile *opendevice(const char *filename);

  bool setsoundtype(int stereo,int samplesize,int speed);
  void set8bitmode() { want8bit = 1; }
  bool setfiletype(enum soundtype);
  bool putblock(void *buffer,int size);
  int putblock_nt(void *buffer,int size);
  void releasedevice(void) {};
  bool attachdevice(void) { return true; };
private:
  Rawtofile(int filehandle);
  int init_putblock;
  int rawstereo,rawsamplesize,rawspeed,want8bit;
  soundtype filetype;
  WAVEHEADER hdr;
};

#ifdef WANT_OSS

// Class for playing raw data using OSS audio driver
class Rawplayer : public Soundplayer
{
public:
  ~Rawplayer();

  static Rawplayer *opendevice(const char *filename);
  static int getdevicehandle(const char *filename);

  void abort(void);

  bool setsoundtype(int stereo,int samplesize,int speed);
  void set8bitmode() { want8bit = 1; }
  bool resetsoundtype(void);
  void releasedevice(void);
  bool attachdevice(void);

  bool putblock(void *buffer,int size);
  int  putblock_nt(void *buffer,int size);
  int  getblocksize(void);

  void setquota(int q){quota=q;};
  int  getquota(void) {return quota;};

  static const char *defaultdevice;
  static int  setvolume(int volume);
  int  fix_samplesize(void *buffer, int size);
private:
  Rawplayer(const char *filename, int audiohandle, int audiobuffersize);
  short int rawbuffer[RAWDATASIZE];
  int  rawbuffersize;
  int  audiobuffersize;
	short first_init;
	char *filename;
  int  rawstereo,rawsamplesize,rawspeed,want8bit;
  short forcetomono,forceto8;
  int  quota;
#ifdef NEWTHREAD
	char *fnbak;
#endif
};

#endif /* WANT_OSS */

#ifdef WANT_ESD
// Class for playing raw data via Enlightened Sound Daemon
class EsdPlayer : public Soundplayer
{
public:
  EsdPlayer(const char *host = NULL);

  ~EsdPlayer(void);

  void abort(void);

  bool setsoundtype(int stereo,int samplesize,int speed);
  void set8bitmode();
  bool resetsoundtype(void);
  void releasedevice(void);
  bool attachdevice(void);

  bool putblock(void *buffer,int size);
  int  putblock_nt(void *buffer,int size);
  int  getblocksize(void);

  static char *defaultdevice;
  static int  setvolume(int volume);
  int  fix_samplesize(void *buffer, int size);
private:
	const char *host;
	int esd_handle;

  int  rawstereo,rawsamplesize,rawspeed,want8bit;
  short forcetomono,forceto8;
};

#endif /* WANT_ESD */

#ifdef WANT_SDL
#include "cyclicbuffer.h"

// Class for playing raw data via Enlightened Sound Daemon
class SDLPlayer : public Soundplayer
{
public:
  SDLPlayer(void);

  ~SDLPlayer(void);

  void abort(void);

  bool setsoundtype(int stereo,int samplesize,int speed);
  void set8bitmode();
  bool resetsoundtype(void);
  void releasedevice(void);
  bool attachdevice(void);

  bool putblock(void *buffer,int size);
  int  putblock_nt(void *buffer,int size);
  int  getblocksize(void);

  static char *defaultdevice;
  static int  setvolume(int volume);
  int  fix_samplesize(void *buffer, int size);

	static int sdl_init;
private:
  int  rawstereo,rawsamplesize,rawspeed,want8bit;
  short forcetomono,forceto8;
	CyclicBuffer *buffer;
	int readIndex;
	int writeIndex;
};

#endif /* WANT_ESD */


#ifdef WANT_NAS
#include <audio/audiolib.h>

// Playing raw audio over a Network Audio System
// By Willem (willem@stack.nl)
class NASplayer : public Soundplayer
{
public:
	~NASplayer();

	static NASplayer *opendevice(const char *device);

	void abort(void);

	bool setsoundtype(int stereo, int samplesize, int speed);
	void set8bitmode() { want8bit = 1; }
	bool resetsoundtype(void);
	void releasedevice(void) {}
	bool attachdevice(void) { return true; }

	bool putblock(void *buffer, int size);
	int  putblock_nt(void *buffer, int size);

	int  getblocksize(void);

	int  setvolume(int volume);

	AuBool event_handler(AuEvent *ev);
private:
	NASplayer(AuServer *aud);

	AuServer *aud;
	AuFlowID flow;
	AuDeviceID dev;
	AuEventHandlerRec *evhnd;
	unsigned char format;
	int samplerate, channels;
	int want8bit;
	int buffer_ms;
	int req_size;
};
#endif /*\ WANT_NAS \*/

/*********************************/
/* Data format converter classes */
/*********************************/
// Class for converting wave format to raw format
class Wavetoraw
{
public:
  Wavetoraw(Soundinputstream *loader,Soundplayer *player);
  ~Wavetoraw();

  bool initialize(void);
  void setforcetomono(short flag){forcetomonoflag=flag;};
  bool run(void);

  int  getfrequency(void)    const {return speed;};
  bool isstereo(void)        const {return stereo;};
  int  getsamplesize(void)   const {return samplesize;};

  int  geterrorcode(void)    const {return __errorcode;};

  int  gettotallength(void)  const {return size/pcmsize;};
  int  getcurrentpoint(void) const {return currentpoint/pcmsize;};
  void setcurrentpoint(int p);

private:
  int  __errorcode;
  bool seterrorcode(int errorcode) {__errorcode=errorcode; return false; };

  short forcetomonoflag;

  Soundinputstream *loader;
  Soundplayer *player;

  bool initialized;
  char *buffer;
  int  buffersize;
  int  samplesize,speed,stereo;
  int  currentpoint,size;
  int  pcmsize;

  bool testwave(char *buffer);
};


// Class for Mpeg layer3
class Mpegbitwindow
{
public:
  Mpegbitwindow(){bitindex=point=0;};

  void initialize(void)  {bitindex=point=0;};
  int  gettotalbit(void) const {return bitindex;};
  void putbyte(int c)    {buffer[point&(WINDOWSIZE-1)]=c;point++;};
  void wrap(void);
  void rewind(int bits)  {bitindex-=bits;};
  void forward(int bits) {bitindex+=bits;};
  int  getbit(void);
  int  getbits9(int bits);
  int  getbits(int bits);

private:
  int  point,bitindex;
  char buffer[2*WINDOWSIZE];
};



// Class for converting mpeg format to raw format
class Mpegtoraw
{
  /*****************************/
  /* Constant tables for layer */
  /*****************************/
private:
  static const int bitrate[2][3][15],frequencies[2][3];
  static const REAL scalefactorstable[64];
  static const HUFFMANCODETABLE ht[HTN];
  static const REAL filter[512];
  static REAL hcos_64[16],hcos_32[8],hcos_16[4],hcos_8[2],hcos_4;

  /*************************/
  /* MPEG header variables */
  /*************************/
private:
  int layer,protection,bitrateindex,padding,extendedmode, is_vbr;
  int first_layer;
  enum _mpegversion  {mpeg1,mpeg2} version;
  enum _mode    {fullstereo,joint,dual,single} mode;
  enum _frequency {frequency44100,frequency48000,frequency32000} frequency;

  /*************************/
  /* Initialization vars   */
  /*************************/
  struct mpeg_header
  {
    int layer;
    int protection;
    int bitrateindex;
    int padding;
    int extendedmode;
    _mpegversion version;
    _mode mode;
    _frequency frequency;
  } header_one;

  /*******************************************/
  /* Functions getting MPEG header variables */
  /*******************************************/
public:
  // General
  int  getversion(void)   const {return version;};
  int  getlayer(void)     const {return layer;};
  bool getcrccheck(void)  const {return (!protection);};
  // Stereo or not
  int  getmode(void)      const {return mode;};
  const char *getmodestring(void) const { return (mode == fullstereo ?
  	"Stereo" : (mode == joint ? "JointStereo" : (mode == dual ?
	"DualChannel" : "Mono"))); };
  bool isstereo(void)     const {return (getmode()!=single);};
  // Frequency and bitrate
  int  getfrequency(void) const {return frequencies[version][frequency];};
  int  getbitrate(void)   const {return bitrate[version][layer-1][bitrateindex];};

  /***************************************/
  /* Interface for setting music quality */
  /***************************************/
private:
  short forcetomonoflag;
  int  downfrequency;

public:
  void setforcetomono(short flag);
  void set8bitmode() { if(player) player->set8bitmode(); }
  void setdownfrequency(int value);

  /******************************************/
  /* Functions getting other MPEG variables */
  /******************************************/
public:
  short getforcetomono(void);
  int  getdownfrequency(void);
  int  getpcmperframe(void);

  /******************************/
  /* Frame management variables */
  /******************************/
private:
  int currentframe,totalframe;
  int decodeframe;
  int *frameoffsets;

  /******************************/
  /* Frame management functions */
  /******************************/
public:
  int  getcurrentframe(void) const {return currentframe;};
  int  gettotalframe(void)   const {return totalframe;};
  void setframe(int framenumber);
#ifdef NEWTHREAD
  int skip;
  unsigned char *sound_buf;
  void continueplaying(void); 
  void createplayer(void);
  void abortplayer(void);
  void pauseplaying(void);
private:
  void setdecodeframe(int framenumber);
  /*******************************/
  /* Buffer management functions */
  /*******************************/
private:
  void settotalbuffers(short amount);
  short request_buffer(void);
  void release_buffer(short index);
  void queue_buf(short index);
  void dequeue_buf(short index);
  void dequeue_first();
  struct mpeg_buffer *get_buffer(short index);
  void makeroom(int size);

public:
	void real_alarm_handler(int bla); 

  int getfreebuffers(void);
  /*******************************/
  /* Buffer management variables */
  /*******************************/
private:
  FILE *debugfile;
  short no_buffers;
  short free_buffers;
  mpeg_buffer *buffers;
  buffer_node *queue;
  buffer_node *spare;
#endif

  /***************************************/
  /* Variables made by MPEG-Audio header */
  /***************************************/
private:
  int tableindex,channelbitrate;
  int stereobound,subbandnumber,inputstereo,outputstereo;
  REAL scalefactor;
  int framesize;

  /********************/
  /* Song information */
  /********************/
private:
  struct
  {
    char name   [30+1];
    char artist [30+1];
    char album  [30+1];
    char year   [ 4+1];
    char comment[30+1];
    unsigned char genre;
  }songinfo;
  int totaltime;

  /* Song information functions */
public:
  const char *getname    (void) const { return (const char *)songinfo.name;   };
  const char *getartist  (void) const { return (const char *)songinfo.artist; };
  const char *getalbum   (void) const { return (const char *)songinfo.album;  };
  const char *getyear    (void) const { return (const char *)songinfo.year;   };
  const char *getcomment (void) const { return (const char *)songinfo.comment;};
  unsigned char getgenre (void) { return songinfo.genre; };
  int gettotaltime() { return totaltime; }

  /*******************/
  /* Mpegtoraw class */
  /*******************/
public:
  Mpegtoraw(Soundinputstream *loader,Soundplayer *player);
  ~Mpegtoraw();
  bool initialize(const char *filename);
  bool run(int frames);
  int  geterrorcode(void) {return __errorcode;};
  void clearbuffer(void);

private:
  int __errorcode;
  bool seterrorcode(int errorno){__errorcode=errorno;return false;};

  /*****************************/
  /* Loading MPEG-Audio stream */
  /*****************************/
public:
	void set_time_scan(short scan) { scan_mp3 = (scan ? 1 : 0); }

private:
  Soundinputstream *loader;   // Interface
  union
  {
    unsigned char store[4];
    unsigned int  current;
  }u;
  char buffer[4096];
  int  bitindex;
  short scan_mp3; //default = 1 => don't scan mp3 to calculate length in sec.
  bool fillbuffer(int size){bitindex=0;return loader->_readbuffer(buffer,size);};
  void sync(void)  {bitindex=(bitindex+7)&0xFFFFFFF8;};
  bool issync(void){return (bitindex&7);};
  int getbyte(void);
  int getbits(int bits);
  int getbits9(int bits);
  int getbits8(void);
  int getbit(void);

  /********************/
  /* Global variables */
  /********************/
private:
  int lastfrequency,laststereo;
	int suppress_sound;

  // for Layer3
  int layer3slots,layer3framestart,layer3part2start;
  REAL prevblck[2][2][SBLIMIT][SSLIMIT];
  int currentprevblock;
  layer3sideinfo sideinfo;
  layer3scalefactor scalefactors[2];

  Mpegbitwindow bitwindow;
  int wgetbit(void);
  int wgetbits9(int bits);
  int wgetbits(int bits);


  /*************************************/
  /* Decoding functions for each layer */
  /*************************************/
private:
  bool loadheader(bool lookahead=true);

  //
  // Subbandsynthesis
  //
  REAL calcbufferL[2][CALCBUFFERSIZE],calcbufferR[2][CALCBUFFERSIZE];
  int  currentcalcbuffer,calcbufferoffset;

  void computebuffer(REAL *fraction,REAL buffer[2][CALCBUFFERSIZE]);
  void generatesingle(void);
  void generate(void);
  void subbandsynthesis(REAL *fractionL,REAL *fractionR);

  void computebuffer_2(REAL *fraction,REAL buffer[2][CALCBUFFERSIZE]);
  void generatesingle_2(void);
  void generate_2(void);
  void subbandsynthesis_2(REAL *fractionL,REAL *fractionR);

  // Extarctor
  void extractlayer1(void);    // MPEG-1
  void extractlayer2(void);
  void extractlayer3(void);
  void extractlayer3_2(void);  // MPEG-2


  // Functions for layer 3
  void layer3initialize(void);
  bool layer3getsideinfo(void);
  bool layer3getsideinfo_2(void);
  void layer3getscalefactors(int ch,int gr);
  void layer3getscalefactors_2(int ch);
  void layer3huffmandecode(int ch,int gr,int out[SBLIMIT][SSLIMIT]);
  REAL layer3twopow2(int scale,int preflag,int pretab_offset,int l);
  REAL layer3twopow2_1(int a,int b,int c);
  void layer3dequantizesample(int ch,int gr,int   in[SBLIMIT][SSLIMIT],
			                    REAL out[SBLIMIT][SSLIMIT]);
  void layer3fixtostereo(int gr,REAL  in[2][SBLIMIT][SSLIMIT]);
  void layer3reorderandantialias(int ch,int gr,REAL  in[SBLIMIT][SSLIMIT],
				               REAL out[SBLIMIT][SSLIMIT]);

  void layer3hybrid(int ch,int gr,REAL in[SBLIMIT][SSLIMIT],
		                  REAL out[SSLIMIT][SBLIMIT]);
  
  void huffmandecoder_1(const HUFFMANCODETABLE *h,int *x,int *y);
  void huffmandecoder_2(const HUFFMANCODETABLE *h,int *x,int *y,int *v,int *w);

  /********************/
  /* Playing raw data */
  /********************/
private:
  Soundplayer *player;       // General playing interface
  int       rawdataoffset;
  short int rawdata[RAWDATASIZE];

  void clearrawdata(void)    {rawdataoffset=0;};
  void putraw(short int pcm) {rawdata[rawdataoffset++]=pcm;};
#ifdef NEWTHREAD
  void flushrawdata(short index=0);
#else
  void flushrawdata(void);
#endif

  /***************************/
  /* Interface for threading */
  /***************************/
#ifdef NEWTHREAD
public:
    pth_t play_thread;
    bool player_run;
    void threadplay(void *bla);
#elif defined(PTHREADEDMPEG)
private:
  struct
  {
    short int *buffer;
    int framenumber,frametail;
    int head,tail;
    int *sizes;
  }threadqueue;

  struct
  {
    bool thread;
    bool quit,done;
    bool pause;
    bool criticallock,criticalflag;
  }threadflags;

  pthread_t thread;

public:
  void threadedplayer(void);
  bool makethreadedplayer(int framenumbers);
  void freethreadedplayer(void);

  void stopthreadedplayer(void);
  void pausethreadedplayer(void);
  void unpausethreadedplayer(void);

  bool existthread(void);
  int  getframesaved(void);
#endif /* NEWTHREAD */
};



/***********************/
/* File player classes */
/***********************/
// Superclass for playing file
class Fileplayer
{
public:
	enum audiodriver_t {
		AUDIODRV_OSS, AUDIODRV_ESD, AUDIODRV_SDL, AUDIODRV_NAS
	};

  virtual ~Fileplayer(); //anyone may destruct a FilePlayer object directly

  int geterrorcode(void)        {return __errorcode;};
	struct song_info getsonginfo() { return info;};
  virtual bool openfile(const char *filename, const char *device, soundtype write2file=ST_NONE)=0;
  virtual void closefile(void)                       =0;
  virtual void setforcetomono(short flag)            =0;
	virtual void setdownfrequency(int)                 =0;
  virtual void set8bitmode()                         =0;
  virtual bool playing()                             =0;
	virtual bool run(int)                              =0;
	virtual void skip(int)                             =0;
	virtual bool initialize(void *)                    =0;
	//if argument to forward < 0, will skip to end - <argument>
	virtual bool forward(int)                          =0;
	virtual bool rewind(int)                           =0;
	virtual bool pause()                               =0;
	virtual bool unpause()                             =0;
	virtual bool stop()                                =0;
	virtual bool ready()                               =0;
	virtual int elapsed_time()                         =0;
	virtual int remaining_time()                       =0;
  
protected:
  Fileplayer(); //thou shallt not instantiate fileplayer itself.

  bool opendevice(const char *device, soundtype write2file=ST_NONE);
	void set_driver(audiodriver_t driver);
  bool seterrorcode(int errorno){__errorcode=errorno;return false;};
  Soundplayer *player;
	struct song_info info;
	const char *filename;

private:
  int __errorcode;
	audiodriver_t audiodriver;
};

struct init_opts
{
	char option[30];
	void *value;
	struct init_opts *next;
};

// Class for playing wave file
class Wavefileplayer : public Fileplayer
{
public:
  Wavefileplayer(audiodriver_t driver);
  ~Wavefileplayer();

  bool openfile(const char *filename, const char *device, soundtype write2file=ST_NONE);
  void closefile(void); 
  void setforcetomono(short flag);
	void setdownfrequency(int value) { if (value); }
  void set8bitmode() { if(player) player->set8bitmode(); }
  bool playing();
	bool run(int);
	bool initialize(void *);
	bool forward(int frames) { skip(frames); return true; }
	bool rewind(int frames) { skip(-frames); return true; }
	bool pause() { player->releasedevice(); return true; }
	bool ready() { return player->attachdevice(); }
	bool unpause() { return true; }
	bool stop() { return true; }
	int elapsed_time();
	int remaining_time();
	void skip(int);
  
private:
  Soundinputstream *loader;

protected:
  Wavetoraw *server;
};


// Class for playing MPEG file
class Mpegfileplayer : public Fileplayer
{
public:
  Mpegfileplayer(audiodriver_t driver);
  ~Mpegfileplayer();

  bool openfile(const char *filename, const char *device, soundtype write2file=ST_NONE);
  void closefile(void);
  void setforcetomono(short flag);
  void set8bitmode() { if (server) server->set8bitmode(); }
  void setdownfrequency(int value);
  bool playing();
	bool run(int);
	void skip(int);
	bool initialize(void *);
	bool forward(int);
	bool rewind(int);
#ifdef NEWTHREAD
	bool pause() { server->pauseplaying(); player->releasedevice(); return true; }
	bool unpause() { server->continueplaying(); return player->attachdevice(); }
#elif defined(PTHREADEDMPEG)
	bool pause() { if (use_threads) server->pausethreadedplayer(); player->releasedevice(); return true;}
	bool unpause() { if (use_threads) server->unpausethreadedplayer(); return player->attachdevice();}
#else
	bool pause() { player->releasedevice(); return true;}
	bool unpause() { return player->attachdevice(); }
#endif
	bool stop();
	bool ready();
	int elapsed_time();
	int remaining_time();
#ifndef NEWTHREAD
#if PTHREADEDMPEG
  bool playingwiththread(int framenumbers);
#endif
#endif /* NEWTHREAD */

private:
  Soundinputstream *loader;
	short use_threads;

protected:
  Mpegtoraw *server;
};

#ifdef INCLUDE_OGG
#include "vorbis/vorbisfile.h"

class Oggplayer : public Fileplayer
{
public:
  Oggplayer(audiodriver_t driver);
  ~Oggplayer();

  bool openfile(const char *filename, const char *device, soundtype write2file=ST_NONE);
  void closefile(void);
  void setforcetomono(short flag);
  void set8bitmode();
  void setdownfrequency(int value);
  bool playing();
	bool run(int);
	void skip(int);
	bool initialize(void *);
	bool forward(int);
	bool rewind(int);
	bool pause();
	bool unpause();
	bool stop();
	bool ready();
	int elapsed_time();
	int remaining_time();

private:
	short wordsize;
	short mono;
	short downfreq;
	short bigendian;
	short signeddata;
	int   channels;
	long  srate;
	short resetsound;

protected:
	OggVorbis_File *of;
	char soundbuf[4096];
};
#endif /* INCLUDE_OGG */


#ifdef HAVE_SIDPLAYER
//This is broken due to lack of functions
#include <sidplay/player.h>
#include <sidplay/myendian.h>
#include <sidplay/fformat.h>

class SIDfileplayer : public Fileplayer
{
public:
	SIDfileplayer(audiodriver_t driver);
	~SIDfileplayer();

	bool openfile(const char *filename, const char *device, soundtype write2file=ST_NONE);
	bool initialize(void *data) { if(data); return true; }
	void closefile(void);
	void setforcetomono(short flag);
	void setdownfrequency(int value);
	void set8bitmode() {}
	bool run(int frames);
	bool playing();
	bool ready() { return true; }
	void skip(int) {} //TODO: implement
	int elapsed_time() { return 0; }  //TODO: implement
	int remaining_time() { return 0; }
	bool forward(int sec) { skip(sec); return true; }
	bool rewind(int sec) { skip(-sec); return true; }
	bool pause() { return true; }
	bool unpause() { return true; }
	bool stop() { return true; }
protected:
	emuEngine emu;
	struct emuConfig emuConf;
	sidTune *tune;
	struct sidTuneInfo sidInfo;
	int song;
private:
	char *buffer;
	int bufSize;
};

#endif /*\ HAVE_SIDPLAYER \*/

#endif
