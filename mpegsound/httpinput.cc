/* MPEG/WAVE Sound library

   (C) 1997 by Woo-jae Jung */

// Httpinputstream.cc
// Inputstream for http

// It's from mpg123 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>
#include <unistd.h>

#include "mpegsound.h"
#include "mpegsound_locals.h"

#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

static const char *httpstr="http://";

bool Soundinputstreamfromhttp::writestring(int fd, char *string)
{
  int result,bytes=strlen(string);

  while(bytes)
  {
    if((result=write(fd,string,bytes))<0 && errno!=EINTR)
    {
      seterrorcode(SOUND_ERROR_HTTPWRITEFAIL);
      return false;
    }
    else if(result==0)
    {
      seterrorcode(SOUND_ERROR_HTTPWRITEFAIL);
      return false;
    }
    string += result;
    bytes -= result;
  }

  return true;
}

bool Soundinputstreamfromhttp::readstring(char *string,int maxlen,FILE *f)
{
  char *result;

  do{
    result=fgets(string,maxlen,f);
  }while(!result  && errno==EINTR);
  if(!result)
  {
    seterrorcode(SOUND_ERROR_FILEREADFAIL);
    return false;
  }

  return true;
}

static char *strndup(char *src,int num)
{
  char *dst;

  if(!(dst=(char *)malloc(num+1)))return NULL;
  dst[num]='\0';

  return strncpy(dst, src, num);
}

static char *url2hostport(char *url,char **hname,
			  unsigned long *hip,unsigned int *port)
{
  char *cptr;
  struct hostent *myhostent;
  struct in_addr myaddr;
  int isip=1;

  if(!(strncmp(url,httpstr,7)))url+=7;
  cptr=url;
  while(*cptr && *cptr!=':' && *cptr!='/')
  {
    if((*cptr<'0' || *cptr>'9') && *cptr!='.')isip=0;
    cptr++;
  }
  if(!(*hname=strndup(url,cptr-url)))
  {
    *hname=NULL;
    return NULL;
  }
  if(!isip)
  {
    if (!(myhostent=gethostbyname(*hname)))return NULL;
    memcpy(&myaddr,myhostent->h_addr,sizeof(myaddr));
    *hip=myaddr.s_addr;
  }
  else if((*hip=inet_addr(*hname))==INADDR_NONE)return NULL;
  if(!*cptr || *cptr=='/')
  {
    *port=80;
    return cptr;
  }
  *port=atoi(++cptr);
  while(*cptr && *cptr!='/')cptr++;
  return cptr;
}

char *proxyurl=NULL;
unsigned long proxyip=0;
unsigned int proxyport;

FILE *Soundinputstreamfromhttp::http_open(char *urrel)
{
  char
		*purl=NULL,
		*host = NULL,
		*request = NULL,
		*sptr = NULL,
		*url = NULL;
  char agent[50];
	const char *url_part;
  int linelength;
  int sock;
  int relocate=0,numrelocs=0;
	int slash_count = 0;
  unsigned long myip;
  unsigned int myport;
  struct sockaddr_in server;
  FILE *myfile;

	url = new char[strlen(urrel) + 2];
	strcpy(url, urrel);

	//find hostname from URL
	url_part = url;
	while ( (url_part = strchr(url_part, '/')) != NULL)
	{
		url_part++;
		slash_count++;
	}

	debug("#slashes in URL: %d\n", slash_count);
	if (strlen(url) > 0 && url[strlen(url)-1] != '/' && slash_count == 2)
	{
		//add a trailing slash after the url's hostname
		url[strlen(url)] = '/';
		url[strlen(url)+1] = '\0';
	}

	if (!proxyip)
	{
		if (!proxyurl && !(proxyurl=getenv("MP3_HTTP_PROXY")) && 
			!(proxyurl=getenv("http_proxy")))
		{
			proxyurl = getenv("HTTP_PROXY");
		}

		if (proxyurl && proxyurl[0] && strcmp(proxyurl, "none"))
		{
			if (!(url2hostport(proxyurl, &host, &proxyip, &proxyport)))
			{
				seterrorcode(SOUND_ERROR_UNKNOWNPROXY);
				return NULL;
			}
			if (host)
			{
				free(host);
				host = NULL;
			}
		}
		else
			proxyip = INADDR_NONE;
	}
  
  if (!(purl=(char *)malloc(1024))) 
  {
    seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);
		delete[] url;
    return NULL;
  }
  strncpy(purl,url,1022);
  purl[1022]= purl[1023] = '\0';
	delete[] url;
	url = NULL;
	request = NULL;

	do
	{
		if (request)
			free(request);

  	if ((linelength = strlen(purl) * 2 + 100) < 1024)
    	linelength=1024;
		if (!(request=(char *)malloc(linelength)))
  	{
   		seterrorcode(SOUND_ERROR_MEMORYNOTENOUGH);
			free(purl);
			return NULL;
		}
		if (host)
			free(host);
		host = NULL;
		strcpy(request,"GET ");
		if (proxyip!=INADDR_NONE) 
		{
			if(strncmp(purl,httpstr,7))
				strcat(request,httpstr);
			strcat(request,purl);
			myport=proxyport;
			myip=proxyip;
		}
		else
		{
			if (!(sptr=url2hostport(purl,&host,&myip,&myport)))
			{
				seterrorcode(SOUND_ERROR_UNKNOWNHOST);
				free(purl);
				free(request);
				return NULL;
			}
			strcat (request, sptr);
		}

		strcat(request, " HTTP/1.1\r\n");
		if (host && proxyip == INADDR_NONE) 
		{
			strcat(request, "Host: ");
			strcat(request, host);
			strcat(request, "\r\n");
			free(host);
			host = NULL;
		}
    sprintf (agent, "User-Agent: %s/%s\r\n\r\n",
	     "Mp3blaster",VERSION);
    strcat (request, agent);
		debug("HTTP Request:\n\n%s", request);
    server.sin_family = AF_INET;
    server.sin_port = htons(myport);
    server.sin_addr.s_addr = myip;
    if ((sock=socket(PF_INET,SOCK_STREAM,6))<0)
    {
      seterrorcode(SOUND_ERROR_SOCKET);
			free(purl);
			free(request);	
      return NULL;
    }
    if (connect(sock,(struct sockaddr *)&server,sizeof(server)))
    {
      seterrorcode(SOUND_ERROR_CONNECT);
			free(purl);
			free(request);
      return NULL;
    }
    if (!writestring(sock,request))
		{
			free(purl);
			free(request);
			return NULL;
		}

    if (!(myfile=fdopen(sock, "rb")))
    {
      seterrorcode(SOUND_ERROR_FDOPEN);
			free(purl);
      return NULL;
    };

    relocate=false;
    purl[0]='\0';

    if (!readstring(request,linelength-1,myfile))
		{
			free(purl);
			free(request);
			return NULL;
		}

    if ((sptr=strchr(request,' ')))
    {
      switch(sptr[1])
      {
        case '3':relocate=true;
        case '2':break;
        default: seterrorcode(SOUND_ERROR_HTTPFAIL);
				free(purl);
				free(request);
	      return NULL;
      }
    }

    do
		{
      if (!readstring(request,linelength-1,myfile))
			{
				free(purl);
				free(request);
				return NULL;
			}
      if (!strncmp(request,"Location:",9))
				strncpy (purl,request+10,1023);
    } while (request[0]!='\r' && request[0]!='n');
  } while (relocate && purl[0] && numrelocs++<5);

  if (relocate)
  { 
    seterrorcode(SOUND_ERROR_TOOMANYRELOC);
    return NULL;
  }

  free(purl);
  free(request);

  return myfile;
}

Soundinputstreamfromhttp::Soundinputstreamfromhttp()
{
  fp=NULL;
  debug("httpclass\n");
}

Soundinputstreamfromhttp::~Soundinputstreamfromhttp()
{
  if(fp)fclose(fp);
}

bool Soundinputstreamfromhttp::open(char *url)
{
  if((fp=http_open(url))==NULL)
  {
		debug("Could not open url..\n");
    seterrorcode(SOUND_ERROR_FILEOPENFAIL);
    return false;
  }
	
	debug("url opened\n");
  return true;
}

int Soundinputstreamfromhttp::getbytedirect(void)
{
  int c;

  if((c=getc(fp))<0)
  {
    seterrorcode(SOUND_ERROR_FILEREADFAIL);
    return -1;
  }

  return c;
}

bool Soundinputstreamfromhttp::_readbuffer(char *buffer,int size)
{
  if(fread(buffer,size,1,fp)!=1)
  {
    seterrorcode(SOUND_ERROR_FILEREADFAIL);
    return false;
  }
  return true;
}

bool Soundinputstreamfromhttp::eof(void)
{
  return feof(fp);
};

int Soundinputstreamfromhttp::getblock(char *buffer,int size)
{
  int l;
  l=fread(buffer,1,size,fp);
  if(l==0)seterrorcode(SOUND_ERROR_FILEREADFAIL);
  return l;
}

int Soundinputstreamfromhttp::getsize(void)
{
  return 1;
}

void Soundinputstreamfromhttp::setposition(int)
{
}

int  Soundinputstreamfromhttp::getposition(void)
{
  return 0;
}

