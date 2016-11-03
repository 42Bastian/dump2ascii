/**
 ** Binary to ASCII dumper
 **
 ** (c) 2016 42Bastian Schick
 **
 ** License: Public Domain
 **
 **
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#define MAX_VARS 32
#define PUTCHAR(c) fputc(c,out)

static const char hex[16] = {
  '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
};

void dump(FILE *in,
          FILE *out,
          const char *fmt,
          int len,
          int skip)
{
  const char *pf;
  int c,f;
  char fill;
  char *p;
  const char *fp;
  int width;
  char h[64];
  unsigned long n = 0;
  unsigned long long n64;
  int sign;
  ssize_t err;
  int line;
  unsigned long pos;
  const char *loopStart;
  int loopCount;
  int loopCountMax;
  unsigned long long vars[MAX_VARS];
  int varSize[MAX_VARS];
  int varCount;
  int varRef;

  pos = 0;
  if ( skip > len ){
    skip = len;
  }
  c = 0;
  while( skip && c != EOF ){
    c = getc(in);
    ++pos;
    --len;
    --skip;
  }

  c = 0;
  line = 1;
  varRef = -1;
  varCount = 0;
  loopCount = -1;
  pf = fmt;

  while ( len || *pf ){
    do{
      c = *pf++;
      if ( c == 0 ){
        pf = fmt;
        varCount = 0;
      }
    }while ( c == 0 );

    if ( c != '%' ){
      if ( c == '\\' ){
        c = *pf++;
        switch( c ){
        case '0':
          putc(0,out); break;
        case 'n':
          putc('\n',out); ++line; break;
        case 'r':
          putc('\r',out); break;
        case 't':
          putc('\t',out); break;
        case 'b':
          putc('\b',out); break;
        case 'f':
          putc('\f',out); break;
        case '"':
          putc('"',out); break;
        case '\\':
          putc('\\',out); break;
        default:
          fprintf(out,"%02x",c);
        }
      }else{
        PUTCHAR(c);
      }
      continue;
    }
    c = *pf++;

    fill = ' ';
    if ( c == '0' ){
      fill = c;
      c = *pf++;
    }

    for( width = 0; '0' <= c && c <= '9'; c = *pf++ ){
      width *= 10;
      width += c - '0';
    }

    p = h;
    *p = '\0';

    /* size of data: b - byte h - short l - long ll - long long */
    f = 0;
    if ( c == 'b' || c == 'B' ){
      f = 1;
      c = *pf++;
    } else if ( c == 'h' || c == 'H' ){
      f = 2;
      c = *pf++;
    } else {
      if ( c == 'l' || c == 'L' ){
        f = 3;
        c = *pf++;
      }
      if ( (c == 'l' || c == 'L') && f == 3 ){
        f = 4;
        c = *pf++;
      }
    }

    /* get data from stream or variable */
    if ( c == 'x' || c == 'X' || c == 'p' || c == 'P' || c == 'd' || c == 'u' ){
      n64 = 0;
      if ( varRef != -1 ){
        if ( varRef < varCount ){
          n64 = vars[varRef];
        }
        varRef = -1;
      } else if ( len ){
        switch ( f ){
        case 0:
        case 3:
          err = fread((void *)&n64, sizeof(n), 1, in);
          pos += sizeof(n);
          len -= sizeof(n);
          if ( err != 1 ){
            n64 = 0;
            len = 0;
          }
          break;
        case 1:
          n64 = getc(in);
          ++pos;
          --len;
          if ( n64 == EOF ){
            n64 = 0;
            len = 0;
          }
          break;
        case 2:
          err = fread((void *)&n64, 2, 1, in);
          pos += 2;
          len -= 2;
          if ( err != 1 ){
            n64 = 0;
            len = 0;
          }
          break;
        case 4:
          err = fread((void *)&n64, sizeof(n64), 1, in);
          pos += sizeof(n64);
          len -= sizeof(n64);
          if ( err != 1 ){
            n64 = 0;
            len = 0;
          }
          break;
        }

        if ( varCount < MAX_VARS ){
          vars[varCount] = n64;
          varSize[varCount] = f;
          ++varCount;
        }
      } else {
        continue;
      }
    }

    /* do command */
    switch( c ){
    case '$':
      if ( width == 0 ){
        if ( loopCount >= 0 ){
          varRef = loopCount;
        }
      } else {
        varRef = width-1;
      }
      continue;

    case '{':
      loopStart = pf;
      loopCountMax = width;
      loopCount = 0;
      continue;

    case '}':
      if ( loopCount < loopCountMax ){
        ++loopCount;
        if ( loopCount < loopCountMax ){
          pf = loopStart;
        } else {
          loopCount = -1;
        }
      }
      continue;

    case 'j':
      err = 0;
      while( width && err != EOF ){
        err = getc(in);
        --len;
        --width;
        ++pos;
      }
      continue;

    case 'a':
      if ( width ){
        sprintf(h,"%%%c%dx",fill,width);
      } else {
        strcpy(h,"%08x");
      }
      fprintf(out,h,pos);
      continue;

    case 'q':
      if ( width ){
        sprintf(h,"%%%c%dd",fill,width);
      } else {
        strcpy(h,"%06d");
      }
      fprintf(out,h,line);
      continue;

    case 'x':
    case 'X':
    case 'p':
    case 'P':
      do {
        *p++ = hex[(n64 & 0xfULL)];
        n64 >>= 4;
      } while (n64);
      break;
    case 'd':
    case 'u':
      sign = 0;
      if ( c == 'd' ){
        switch ( f ){
        case 0:
        case 3:
          if ( n64 > 0x7ffffff ){
            sign = -1;
            n64 = (~n64 & 0xffffffffUL)+1;
          }
          break;
        case 1:
          if ( n64 > 0x7f ){
            sign = -1;
            n64 = (~n64 & 0xffUL)+1;
          }
          break;
        case 2:
          if ( n64 > 0x7fff ){
            sign = -1;
            n64 = (~n64 & 0xffffUL)+1;
          }
          break;
        case 4:
          if ((signed long long) n64 < 0) {
            sign = -1;
            n64 = (unsigned long long) (-(signed long long) n64);
          }
          break;
        }
      }
      do {
        *p++ = hex[(int)(n64 % 10ULL)];
        n64 /= 10ULL;
      } while (n64);
      if ( sign == -1 ){
        *p++ = '-';
      }
      break;
    case 'c':
      c = 0;
      if ( varRef != -1 ){
        if ( varRef < varCount ){
          c = (int)vars[varRef];
        }
        varRef = -1;
      } else {
        c = getc(in);
        ++pos;
        --len;
        if ( c == EOF ){
          c = ' ';
          len = 1;
        }
      }
      if ( c < 32 ){
        c = '.';
      }
      *p++ = c;
      break;
    case 's':
      p = h;
      do {
        c = getc(in);
        ++pos;
        --len;
        if ( c == EOF ){
          c = 0;
          len = 0;
        }
        *p++ = c;
      }while( c );

      p = h;

      width = ( width > (sizeof(h)-1) ) ? (sizeof(h)-1) : width;

      for( width -= strlen(p); width > 0 ; --width ){
        PUTCHAR(fill);
      }
      while( *p ){
        PUTCHAR(*p);
        ++p;
      }
      continue;
    default:
      PUTCHAR(c);
      continue;
    }
    width = ( width > (sizeof(h)-1) ) ? (sizeof(h)-1) : width;

    for( width = width - (int)(p-h); width > 0 ; --width ){
      PUTCHAR(fill);
    }
    for( --p; p >= h ; --p){
      PUTCHAR(*p);
    }
  }
  fprintf(out,"\n");
  fclose(out);
  fclose(in);
}

int
main (int argc, char *argv[])
{
  int c;
  int len = 0;
  FILE *in = stdin;
  FILE *out = stdout;
  char *format = NULL;
  int skip = 0;
  while ((c = getopt (argc, argv, "hi:o:f:s:")) != -1) {
    switch (c) {
    case 'h':
      fprintf(stderr,
              "Usage: %s [-i <input>] [-o <output>] [-s <skip>] [-f <format>]\n",argv[0]);
      fprintf(stderr,
              "input:  stdin if not set\n"
              "output: stdout if not set\n"
              "format: normal printf() format (no float), default %%ld\n"
              "        extra: %%[fill][size]q outputs line number\n"
              "               %%[fill][size]a outputs input stream  position\n"
              "               %%[size]j skips bytes in the input stream\n"
              "               Typemodifiers: b - byte: %02bx\n"
              "                              h - short: %04hx\n"
              "               %%[size]$ back-reference to a previous %%d|%%x\n"
              "               %%0$ use loop counter as index\n"
              "skip: skip bytes at beginning of file\n"
              "Example\n"
              "-f \"%%08a: %%16{ %%02bx%%} : %%16{%%0$%%c%%}\\n\" for canonical dump:\n"
              "00000000:  23 69 6E 63 6C 75 64 65 20 3C 73 74 64 69 6F 2E : #include <stdio.\n"
              "00000010:  68 3E 0D 0A 23 69 6E 63 6C 75 64 65 20 3C 73 74 : h>..#include <st\n"
        );
      return 0;
    case 'i':
      if (!(in = fopen (optarg, "rb"))) {
        perror ("Input file open");
      }
      fseek(in,0L,SEEK_END);
      len = ftell(in);
      fseek(in,0L,SEEK_SET);
      break;
    case 'o':
      if (!(out = fopen (optarg, "wb"))) {
        perror ("Input file open");
      }
      break;
    case 'f':
      format = optarg;
      break;
    case 's':
      skip = atoi(optarg);
    default:
      break;
    }
  }
  if ( format == NULL || *format == 0 ){
    if ( !strcmp(argv[0],"hexdump") ){
      format = "%08a: %16{ %02bx%} : %16{%0$%c%}\n";
    } else {
      format = "%ld";
    }
  }
  dump(in, out, format, len, skip);
  return 0;
}
