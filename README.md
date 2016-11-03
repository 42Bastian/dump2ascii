# dump2ascii
Binary to ASCII dumper

This is a small versatile command line tool I wrote to dump binary files which contain arbitrary data.
The source should compile fine with any compiler. I tested it with GCC on cygwin.

Parameters:
<pre>
Usage: dump2ascii [-i &lt;input&gt;] [-o &lt;output&gt;] [-s &lt;skip&gt;] [-f &lt;format&gt;]
input:  stdin if not set
output: stdout if not set
format: normal printf() format (no float), default %ld
        extra: %[fill][size]q outputs line number
               %[fill][size]a outputs input stream  position
               %[size]j skips bytes in the input stream
               Typemodifiers: b - byte: %bx
                              h - short: %hx
               %[size]$ back-reference to a previous %d|%x
               %0$ use loop counter as index
skip: skip bytes at beginning of file
Example
-f "%08a: %16{ %02bx%} : %16{%0$%c%}\n" for canonical dump:
00000000:  23 69 6E 63 6C 75 64 65 20 3C 73 74 64 69 6F 2E : #include &lt;stdio.
00000010:  68 3E 0D 0A 23 69 6E 63 6C 75 64 65 20 3C 73 74 : h&gt;..#include &lt;st
</pre>
If the name is "hexdump" the format defaults to the canonical dump.<br>

# Author
42Bastian
