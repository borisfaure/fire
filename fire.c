#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

unsigned short _width;
unsigned short _height;
char *_line = NULL;

#define DIE(STR)  do { \
     perror(STR);      \
     exit(1);          \
} while (0)


#define NB_COLORS  37
const char *bg_colors[NB_COLORS] = {
     "\033[48;2;7;7;7m",        // 0x07,0x07,0x07
     "\033[48;2;31;7;7m",       // 0x1F,0x07,0x07
     "\033[48;2;47;7;7m",       // 0x2F,0x07,0x07
     "\033[48;2;71;15;7m",      // 0x47,0x0F,0x07
     "\033[48;2;87;23;7m",      // 0x57,0x17,0x07
     "\033[48;2;103;23;7m",     // 0x67,0x1F,0x07
     "\033[48;2;119;23;7m",     // 0x77,0x1F,0x07
     "\033[48;2;143;39;7m",     // 0x8F,0x27,0x07
     "\033[48;2;159;47;7m",     // 0x9F,0x2F,0x07
     "\033[48;2;175;63;7m",     // 0xAF,0x3F,0x07
     "\033[48;2;191;71;7m",     // 0xBF,0x47,0x07
     "\033[48;2;199;71;7m",     // 0xC7,0x47,0x07
     "\033[48;2;223;79;7m",     // 0xDF,0x4F,0x07
     "\033[48;2;223;87;7m",     // 0xDF,0x57,0x07
     "\033[48;2;223;87;7m",     // 0xDF,0x57,0x07
     "\033[48;2;215;95;7m",     // 0xD7,0x5F,0x07
     "\033[48;2;215;95;7m",     // 0xD7,0x5F,0x07
     "\033[48;2;215;103;15m",   // 0xD7,0x65,0x0F
     "\033[48;2;207;111;15m",   // 0xCF,0x6F,0x0F
     "\033[48;2;207;119;15m",   // 0xCF,0x77,0x0F
     "\033[48;2;207;127;15m",   // 0xCF,0x7F,0x0F
     "\033[48;2;207;135;23m",   // 0xCF,0x87,0x17
     "\033[48;2;199;135;23m",   // 0xC7,0x87,0x17
     "\033[48;2;199;143;23m",   // 0xC7,0x8F,0x17
     "\033[48;2;199;151;31m",   // 0xC7,0x97,0x1F
     "\033[48;2;191;159;31m",   // 0xBF,0x9F,0x1F
     "\033[48;2;191;159;31m",   // 0xBF,0x9F,0x1F
     "\033[48;2;191;167;39m",   // 0xBF,0xA7,0x27
     "\033[48;2;191;167;39m",   // 0xBF,0xA7,0x27
     "\033[48;2;191;175;47m",   // 0xBF,0xAF,0x2F
     "\033[48;2;183;175;47m",   // 0xB7,0xAF,0x2F
     "\033[48;2;183;183;47m",   // 0xB7,0xB7,0x2F
     "\033[48;2;183;183;55m",   // 0xB7,0xB7,0x37
     "\033[48;2;207;207;111m",  // 0xCF,0xCF,0x6F
     "\033[48;2;223;223;159m",  // 0xDF,0xDF,0x9F
     "\033[48;2;239;239;199m",  // 0xEF,0xEF,0xC7
     "\033[48;2;255;255;255m",  // 0xFF,0xFF,0xFF
};

static ssize_t
xwrite(const void *buf, size_t count)
{
   const char *data = buf;
   ssize_t len = count;

   while (len > 0)
     {
        ssize_t res = write(STDOUT_FILENO, data, len);

        if (res < 0)
          {
             if (errno == EINTR || errno == EAGAIN)
               continue;
             DIE("unable to write");
          }
        data += res;
        len  -= res;
     }
   return count;
}

static void
_go_to(int x, int y)
{
   char buf[64] = {};
   int len;

   len = snprintf(buf, sizeof(buf), "\033[%d;%dH", y+1, x+1); // Positions start at 1
   xwrite(buf, len);
}

static void
init_screen(void)
{
   const char *esc_clear_screen = "\033[2J";
   const char *esc_color_black_bg = "\033[48;5;232m";
   const char *esc_color_black_fg = "\033[38;5;232m";

   xwrite(esc_color_black_bg, strlen(esc_color_black_bg));
   xwrite(esc_color_black_fg, strlen(esc_color_black_fg));
   xwrite(esc_clear_screen, strlen(esc_clear_screen));
   xwrite(bg_colors[NB_COLORS-1], strlen(bg_colors[NB_COLORS-1]));
   _go_to(0, _height - 1);
   xwrite(_line, _width);
}



static int
run(void)
{
   struct winsize w;
   int i;

   if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1)
     DIE("failed to get terminal size with ioctl");

   _width = w.ws_col;
   _height = w.ws_row;

   if (_line)
     free(_line);
   _line = malloc(sizeof(char) * _width);
   if (!_line)
     DIE("malloc");
   for (i = 0; i < _width; i++)
     _line[i] = ' ';

   init_screen();

   return 0;
}

int main (void)
{
   return run();
}
