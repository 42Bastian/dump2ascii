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
#include <stdint.h>

#define MAX_VARS 32
#define PUTCHAR(c) fputc(c,out)

/**
 * Read 64,32 or 16 bit number from stream with correct endianess.
 *
 */
uint64_t readNum(FILE *in, int size, int endian, int *err)
{
  uint64_t n64;
  int c;
  int i;

  *err = 0;
  n64 = 0;

  if ( endian == BIG_ENDIAN ){
    for(i = 0; i < size; ++i){
      n64 <<= 8;
      c = getc(in);
      if ( c == EOF ){
        *err = 1;
        n64 = 0;
        break;
      }
      n64 |= c & 0xff;
    }
  } else {
    for(i = 0; i < size; ++i){
      n64 >>= 8;
      c = getc(in);
      if ( c == EOF ){
        *err = 1;
        n64 = 0;
        break;
      }
      n64 |= ((uint64_t)(c & 0xff))<<(8*(size-1));
    }
  }
  return n64;
}

void dump(FILE *in,
          FILE *out,
          const char *fmt,
          int len,
          int skip,
          int endian)
{
  static const char hex[16] = {
    '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
  };
  static const char HEX[16] = {
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
  };

  const char *pf;

  int c,f;
  char fill;
  char *p;
  int width;
  char h[64];
  uint64_t n64;
  int sign;

  ssize_t err;
  int line;
  unsigned long pos;

  const char *loopStart;
  int loopCount;
  int loopCountMax;

  unsigned long long vars[MAX_VARS];
  int varCount;
  int varRef;

  pos = 0;
  line = 1;
  varRef = -1;
  varCount = 0;
  loopCount = -1;
  loopCountMax = 0;
  pf = fmt;
  c = 0;

  if ( skip ){
    if ( skip < len ){
      err = fseek(in, skip, SEEK_CUR);

      if ( err == EOF ){
        len = 0;
        pf = NULL;
      } else {
        pos += skip;
        len -= skip;
      }
    } else {
      len = 0;
      pf = NULL;
    }
  }

  while ( len || (pf && *pf) ){
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
    n64 = 0;
    if ( c == 'x' || c == 'X' || c == 'p' || c == 'P' || c == 'd' || c == 'u' ){
      if ( varRef != -1 ){
        if ( varRef < varCount ){
          n64 = vars[varRef];
        }
        varRef = -1;
      } else if ( len ){
        switch ( f ){
        case 0:
        case 3:
          n64 = readNum(in, 4, endian, &err);
          pos += 4;
          len -= 4;
          if ( err ){
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
          n64 = readNum(in, 2, endian, &err);
          pos += 2;
          len -= 2;
          if ( err ){
            len = 0;
          }
          break;
        case 4:
          n64 = readNum(in, 8, endian, &err);
          pos += 8;
          len -= 8;
          if ( err  ){
            len = 0;
          }
          break;
        }

        if ( varCount < MAX_VARS ){
          vars[varCount] = n64;
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
      if ( width ){
        loopStart = pf;
        loopCountMax = width;
        loopCount = 0;
      }
      continue;

    case '}':
      if ( loopCountMax ){
        ++loopCount;
        if ( loopCount < loopCountMax ){
          pf = loopStart;
        } else {
          loopCount = -1;
        }
      }
      continue;

    case 'j':
      err = fseek(in, width, SEEK_CUR);
      len -= width;
      pos += width;
      if ( err == EOF ){
        len = 0;
      }
      continue;

    case 'J':
      err = fseek(in, -width, SEEK_CUR);
      len += width;
      pos -= width;
      if ( err == EOF ){
        len = 0;
      }
      continue;

    case 'a':
      if ( width ){
        sprintf(h,"%%%c%dx",fill,width);
        printf("%s",h);
      } else {
        strcpy(h,"%08x");
      }
      fprintf(out,h,pos);
      continue;

    case 'A':
      if ( width ){
        sprintf(h,"%%%c%dX",fill,width);
      } else {
        strcpy(h,"%08X");
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
    case 'p':
      do {
        *p++ = hex[(n64 & 0xfULL)];
        n64 >>= 4;
      } while (n64);
      break;

    case 'X':
    case 'P':
      do {
        *p++ = HEX[(n64 & 0xfULL)];
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
      }while( c && p < h+sizeof(h));
      *p = 0;

      width = ( widthq > (sizeof(h)-1) ) ? (sizeof(h)-1) : width;

      for( width -= strlen(h); width > 0 ; --width ){
        PUTCHAR(fill);
      }
      fprintf(out,"%s",h);
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
  int endian = LITTLE_ENDIAN;

  while ((c = getopt (argc, argv, "bhi:o:f:s:")) != -1) {
    switch (c) {
    case 'h':
      fprintf(stderr,
              "Usage: %s [-i <input>] [-o <output>] [-b] [-s <skip>] [-f <format>]\n",argv[0]);
      fprintf(stderr,
              "input:  stdin if not set\n"
              "output: stdout if not set\n"
              "format: normal printf() format (no float), default %%ld\n"
              "        extra: %%[fill][size]q outputs line number decimal\n"
              "               %%[fill][size](a|A) outputs input stream position in hex\n"
              "               %%[size]j skips bytes in the input stream\n"
              "               %%[size]J go back in the input stream\n"
              "               Typemodifiers: b - byte: %%02bx\n"
              "                              h - short: %%04hx\n"
              "               %%[size]$ back-reference to a previous %%d|%%x\n"
              "               %%0$ use loop counter as index\n"
              "skip: skip bytes at beginning of file\n"
              "-b: Select big endian reading\n"
              "Examples\n"
              "-f \"%%08a: %%16{ %%02bx%%} : %%16{%%0$%%c%%}\\n\"\n"
              "00000000:  23 69 6E 63 6C 75 64 65 20 3C 73 74 64 69 6F 2E : #include <stdio.\n"
              "00000010:  68 3E 0D 0A 23 69 6E 63 6C 75 64 65 20 3C 73 74 : h>..#include <st\n"
              "\n"
              "-f \"%%A %%16{%%02bx %%}: %%16J%%16{%%c%%}\\n\\t%%16J %%8{%%04hx  %%}\\n%%16J\\t %%4{%%08x    %%}\\n\"\n"

              "00000000 2f 2a 2a 0a 20 2a 2a 20 42 69 6e 61 72 79 20 74 : /**. ** Binary t\n"
              "         2a2f  0a2a  2a20  202a  6942  616e  7972  7420\n"
              "         0a2a2a2f    202a2a20    616e6942    74207972\n"
              "00000010 6f 20 41 53 43 49 49 20 64 75 6d 70 65 72 0a 20 : o ASCII dumper.\n"
              "         206f  5341  4943  2049  7564  706d  7265  200a\n"
              "         5341206f    20494943    706d7564    200a7265\n"
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
      break;

    case 'b':
      endian = BIG_ENDIAN;
      break;

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

  dump(in, out, format, len, skip, endian);
  return 0;
}
