/*
 * YAESU FT-736 plugin for gsat
 *
 * Copyright (C) 2003 by Hiroshi Iwamoto, JH4XSY
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Look at the README for more information on the program.
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>

int plugin_open_rig( char * config );
void plugin_close_rig( void );
void plugin_set_downlink_frequency( double frequency );
void plugin_set_uplink_frequency( double frequency );

int  fd;

static void send_freq(char freq[])
{
    int i, j, frbuf;
    unsigned char buf[0];

    i = 0; /* counter for CAT */
    j = 0; /* string counter */

    if ( strncmp(freq, "12", 2) == 0 ) {  /* 1200MHz */
	frbuf  = 0xc0;                
	j = 2;
	frbuf += freq[j]-'0';
    } else {                              /* 430/144MHz */
	frbuf  = ( freq[j]-'0' ) << 4;
	++j;
	frbuf +=   freq[j]-'0';
    }
    buf[0] = frbuf;
    write(fd, buf, 1);			  /* Send 1Byte */

    ++j;
    for( i=1; i<4; ++i) {                 /* Send +3Bytes */
	frbuf = (freq[j]-'0') << 4; 
	++j;
	if ( freq[j] == '.' ) {
	    ++j;                          /* skip */
	}
	frbuf += freq[j]-'0'; 
	buf[0] = frbuf;
	write(fd, buf, 1);
	++j;
    }

}
    
char * plugin_info( void )
{
  return "YAESU FT736 V0.2";
}

int plugin_open_rig( char * config )
{
    char dumm[64];
    char tty[12];
    char Smod[4];
    char Qmod[4];
    char *ptr, *parm;
 
    unsigned char dummy[1];
    unsigned char saton[1];
    unsigned char mode[4];
    unsigned char modRX[1], freqRX[1], modTX[1], freqTX[1];

    unsigned char upMode[1], downMode[1];

    dummy[0] = 0x0;
    saton[0] = 0x0E;
    mode[0] = 0x00;	/* LSB */
    mode[1] = 0x01;	/* USB */
    mode[2] = 0x02;	/* CW */
    mode[3] = 0x08;	/* FM */ 

    modRX[0] = 0x17;	/* SAT, mode RX */
    modTX[0] = 0x27;	/* SAT, mode TX */
    freqRX[0] = 0x1e;   /* SAT, freq RX */
    freqTX[0] = 0x2e;	/* SAT, freq TX */
 
    int term;

    struct termios attr;

    char freq[12];
    char upFreq[12], downFreq[12], tmp1Freq[12], tmp2Freq[12];
    double V = 145.950;
    double U = 435.850;
    double T1 = 52.000;
    double T2 = 1260.000; 

    tty[0]='\0';
    Smod[0]='\0';
    Qmod[0]='\0';

    printf("FT736 plugin opened.\n");

    if(config) {
      strncpy(dumm,config,64);

      ptr=dumm;
      parm=ptr;
      while( parm != NULL ) {
        parm=strsep(&ptr,":");
        if(parm==NULL)
          break;
        if(strlen(parm)!=0) {
          switch( *parm ) {
          case 'D':			/* tty port */
            strcpy(tty,parm+1);
            break;
          case 'S':			/* Up/Down Freq. */
            strcpy(Smod,parm+1);
            break;
          case 'Q':			/* QSO Mode */
            strcpy(Qmod,parm+1);
            break;
          }
        }
      }
    }

    if(strlen(tty)==0)
        strcpy(tty,"/dev/ttyS0");
    if(strlen(Smod)==0)
        strcpy(Smod,"UV");
    if(strlen(Qmod)==0)
        strcpy(Qmod,"CW");

/* Open CAT port */
    if ( (fd = open(tty, O_RDWR)) < 0 ) {
	fprintf(stderr, "can't open %s\n", tty);
        return 0;
    }

/* CAT port initialize */
   tcgetattr(fd, &attr);
   memset(&attr, 0, sizeof(attr));

   attr.c_cflag = B4800|CLOCAL|CREAD;
   attr.c_cflag &= ~CSIZE;
   attr.c_cflag |= CS8;
   attr.c_cflag |= HUPCL|CSTOPB;
   attr.c_iflag = IGNPAR;
   attr.c_oflag = 0;
   attr.c_lflag = 0;
   attr.c_cc[VTIME] = 0;
   attr.c_cc[VMIN] = 1;
   tcsetattr(fd, TCSANOW, &attr);

/* Send CMD: CAT ON */
    write(fd, dummy, 1);
    write(fd, dummy, 1);
    write(fd, dummy, 1);
    write(fd, dummy, 1);
    write(fd, dummy, 1);
    usleep(10000);

/* Send CMD: SAT ON */
    write(fd, dummy, 1);
    write(fd, dummy, 1);
    write(fd, dummy, 1);
    write(fd, dummy, 1);
    write(fd, saton, 1);
    usleep(10000);
    sleep(1);

/* set QSO MODE  */
    upMode[0] = downMode[0] = mode[2];

    if ( strcmp(Qmod, "CW") == 0 ) {
      upMode[0] = downMode[0] = mode[2];  
    }
    if ( strcmp(Qmod, "SSB") == 0 ) {
      upMode[0] = mode[0];
      downMode[0] = mode[1];
    }
    if ( strcmp(Qmod, "FM") == 0 ) {
      upMode[0] = downMode[0] = mode[3];
    }

/* Send CMD: SAT RX MODE */
    write(fd, downMode, 1);
    write(fd, dummy, 1);
    write(fd, dummy, 1);
    write(fd, dummy, 1);
    write(fd, modRX, 1);
    usleep(10000);

/* Send CMD: SAT TX MODE */
    write(fd, upMode, 1);
    write(fd, dummy, 1);
    write(fd, dummy, 1);
    write(fd, dummy, 1);
    write(fd, modTX, 1);
    usleep(10000);
    sleep(1);

/* set SAT MODE */
    sprintf(upFreq, "%.5f", U);
    sprintf(downFreq, "%.5f", V);
    sprintf(tmp1Freq, "%.5f", T1);
    sprintf(tmp2Freq, "%.5f", T2); 

    if ( strcmp(Smod, "UV") == 0 ) {	/* ^ 430MHz v 144MHz */
      sprintf(upFreq, "%.5f", U);
      sprintf(downFreq, "%.5f", V);
    }
    if ( strcmp(Smod, "VU") == 0 ) {	/* ^ 144MHz v 430MHz */
      sprintf(upFreq, "%.5f", V);
      sprintf(downFreq, "%.5f", U);
    }
    if ( strcmp(Smod, "US") == 0 ) {	/* ^ 430MHz v 2.4GHz */
      sprintf(upFreq, "%.5f", U);
      sprintf(downFreq, "%.5f", V);
    }
    term = strlen(upFreq);
    upFreq[term] = '\0';
    term = strlen(downFreq);
    downFreq[term] = '\0';
    term = strlen(tmp1Freq);
    tmp1Freq[term] = '\0';
    term = strlen(tmp2Freq);
    tmp2Freq[term] = '\0'; 

/* Send CMD: dummy downlink Freq */
    send_freq(tmp2Freq);
    write(fd, freqRX, 1);
    usleep(10000);
    sleep(1);
 
/* Send CMD: dummy uplink Freq */
    send_freq(tmp1Freq);
    write(fd, freqTX, 1);
    usleep(10000);

/* Send CMD: uplink Freq */
    send_freq(upFreq);
    write(fd, freqTX, 1);
    usleep(10000); 
    sleep(1);

/* Send CMD: downlink Freq */
    send_freq(downFreq);
    write(fd, freqRX, 1);
    usleep(10000);

    return 1;
}

void plugin_close_rig( void )
{
    unsigned char dummy[1];
    unsigned char catof[1];
    dummy[0] = 0x0;
    catof[0] = 0x80;

    printf("FT736 plugin closed.\n");

/* Send CMD: CAT Off */
    write(fd, dummy, 1);
    write(fd, dummy, 1);
    write(fd, dummy, 1);
    write(fd, dummy, 1);
    write(fd, catof, 1);
    usleep(10000);

}

void plugin_set_downlink_frequency( double frequency )
{
    double freq;
    double conv = 2256;				/* Convertor OSC freq (MHz) */
    char downFreq[12];
    unsigned char freqRX[1];
    freqRX[0] = 0x1e;
    int term;

    freq=frequency-fmod(frequency,5);
    if(fmod(frequency,5)>2.5)
      freq+=5;

/* printf("Downlink Frequency: %f kHz -> %f kHz\n", frequency, freq); */

    frequency = frequency / 1000;		/* Convert: kHz -> MHz */

    if ( frequency > 2400 )
        frequency -= conv;


    sprintf(downFreq, "%.5f", frequency);
    term = strlen(downFreq);
    downFreq[term] = '\0';

    send_freq(downFreq);
    write(fd, freqRX, 1);
    usleep(10000);

}

void plugin_set_uplink_frequency( double frequency )
{
    double freq;
    char upFreq[12];
    unsigned char freqTX[1];
    freqTX[0] = 0x2e;
    int term;

    freq=frequency-fmod(frequency,5);
    if(fmod(frequency,5)>2.5)
        freq+=5;

/* printf("Uplink Frequency: %f kHz -> %f kHz\n", frequency, freq); */

    frequency = frequency / 1000;		/* Convert: kHz -> MHz */
    sprintf(upFreq, "%.5f", frequency);
    term = strlen(upFreq);
    upFreq[term] = '\0';

    send_freq(upFreq);
    write(fd, freqTX, 1);
    usleep(10000);

}
