/* MPEG/WAVE Sound library

   (C) 1997 by Woo-jae Jung */

// Mpegsound.h
//   This is typeset for functions in MPEG/WAVE Sound library.
//   Now, it's for only linux-pc-?86

/************************************/
/* Inlcude default library packages */
/************************************/
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_BOOL_H
#include <bool.h>
#endif
#ifdef PTHREADEDMPEG
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#else
#ifdef HAVE_PTHREAD_MIT_PTHREAD_H
#include <pthread/mit/pthread.h>
#endif
#endif
#endif

#ifndef _L__SOUND__
#define _L__SOUND__

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

#define SOUND_ERROR_UNKNOWN          19


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
#define RIFF		0x46464952
#define WAVE		0x45564157
#define FMT		0x20746D66
#define DATA		0x61746164
#define PCM_CODE	1
#define WAVE_MONO	1
#define WAVE_STEREO	2

#define MODE_MONO   0
#define MODE_STEREO 1

/********************/
/* Type definitions */
/********************/
typedef float REAL;

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

/*********************************/
/* Sound input interface classes */
/*********************************/
// Superclass for Inputbitstream // Yet, Temporary
class Soundinputstream
{
public:
  Soundinputstream();
  virtual ~Soundinputstream();

  static Soundinputstream *hopen(char *filename,int *errcode);

  int geterrorcode(void)  {return __errorcode;};

  virtual bool open(char *filename)              =0;
  virtual int  getbytedirect(void)               =0;
  virtual bool _readbuffer(char *buffer,int size)=0;
  virtual bool eof(void)                         =0;
  virtual int  getblock(char *buffer,int size)   =0;

  virtual int  getsize(void)                     =0;
  virtual int  getposition(void)                 =0;
  virtual void setposition(int pos)              =0;

protected:
  void seterrorcode(int errorcode) {__errorcode=errorcode;};

private:
  int __errorcode;
};

// Inputstream from file
class Soundinputstreamfromfile : public Soundinputstream
{
public:
  Soundinputstreamfromfile()  {fp=NULL;};
  ~Soundinputstreamfromfile();

  bool open(char *filename);
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

  bool open(char *filename);
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
  FILE *http_open(char *url);
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

  virtual bool initialize(char *filename)                       =0;
  virtual void abort(void);
  virtual int  getprocessed(void);

  virtual bool setsoundtype(int stereo,int samplesize,int speed)=0;
  virtual bool resetsoundtype(void);

  virtual bool putblock(void *buffer,int size)                  =0;
  virtual int  getblocksize(void);

  int geterrorcode(void) {return __errorcode;};

protected:
  bool seterrorcode(int errorno) {__errorcode=errorno; return false;};

private:
  int  __errorcode;
};


class Rawtofile : public Soundplayer
{
public:
  ~Rawtofile();

  bool initialize(char *filename);
  bool setsoundtype(int stereo,int samplesize,int speed);
  bool putblock(void *buffer,int size);

private:
  int filehandle;
  int rawstereo,rawsamplesize,rawspeed;
};

// Class for playing raw data
class Rawplayer : public Soundplayer
{
public:
  ~Rawplayer();

  bool initialize(char *filename);
  void abort(void);
  int  getprocessed(void);

  bool setsoundtype(int stereo,int samplesize,int speed);
  bool resetsoundtype(void);

  bool putblock(void *buffer,int size);

  int  getblocksize(void);

  void setquota(int q){quota=q;};
  int  getquota(void) {return quota;};

  static char *defaultdevice;
  static int  setvolume(int volume);

private:
  short int rawbuffer[RAWDATASIZE];
  int  rawbuffersize;
  int  audiohandle,audiobuffersize;
  int  rawstereo,rawsamplesize,rawspeed;
  short forcetomono,forceto8;
  int  quota;
};



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
  void seterrorcode(int errorcode) {__errorcode=errorcode;};

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
  int layer,protection,bitrateindex,padding,extendedmode;
  enum _mpegversion  {mpeg1,mpeg2}                               version;
  enum _mode    {fullstereo,joint,dual,single}                   mode;
  enum _frequency {frequency44100,frequency48000,frequency32000} frequency;

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
    //    char type         ;
  }songinfo;

  /* Song information functions */
public:
  const char *getname    (void) const { return (const char *)songinfo.name;   };
  const char *getartist  (void) const { return (const char *)songinfo.artist; };
  const char *getalbum   (void) const { return (const char *)songinfo.album;  };
  const char *getyear    (void) const { return (const char *)songinfo.year;   };
  const char *getcomment (void) const { return (const char *)songinfo.comment;};

  /*******************/
  /* Mpegtoraw class */
  /*******************/
public:
  Mpegtoraw(Soundinputstream *loader,Soundplayer *player);
  ~Mpegtoraw();
  void initialize(char *filename);
  bool run(int frames);
  int  geterrorcode(void) {return __errorcode;};
  void clearbuffer(void);

private:
  int __errorcode;
  bool seterrorcode(int errorno){__errorcode=errorno;return false;};

  /*****************************/
  /* Loading MPEG-Audio stream */
  /*****************************/
private:
  Soundinputstream *loader;   // Interface
  union
  {
    unsigned char store[4];
    unsigned int  current;
  }u;
  char buffer[4096];
  int  bitindex;
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
  bool loadheader(void);

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
  void flushrawdata(void);

  /***************************/
  /* Interface for threading */
  /***************************/
#ifdef PTHREADEDMPEG
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
#endif
};



/***********************/
/* File player classes */
/***********************/
// Superclass for playing file
class Fileplayer
{
public:
  Fileplayer();
  virtual ~Fileplayer();

  int geterrorcode(void)        {return __errorcode;};

  virtual bool openfile(char *filename,char *device)=0;
  virtual void setforcetomono(short flag)            =0;
  virtual bool playing(int verbose)                 =0;

protected:
  bool seterrorcode(int errorno){__errorcode=errorno;return false;};
  Soundplayer *player;

private:
  int __errorcode;
};


// Class for playing wave file
class Wavefileplayer : public Fileplayer
{
public:
  Wavefileplayer();
  ~Wavefileplayer();

  bool openfile(char *filename,char *device);
  void setforcetomono(short flag);
  bool playing(int verbose);
  
private:
  Soundinputstream *loader;

protected:
  Wavetoraw *server;
};


// Class for playing MPEG file
class Mpegfileplayer : public Fileplayer
{
public:
  Mpegfileplayer();
  ~Mpegfileplayer();

  bool openfile(char *filename,char *device);
  void setforcetomono(short flag);
  void setdownfrequency(int value);
  bool playing(int verbose);
#if PTHREADEDMPEG
  bool playingwiththread(int verbose,int framenumbers);
#endif

private:
  Soundinputstream *loader;

protected:
  Mpegtoraw *server;

  void showverbose(int verbose);
};

#endif
