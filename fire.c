#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

/* Based on http://fabiensanglard.net/doom_fire_psx/index.html */
typedef uint8_t u8;

static unsigned short _width;
static unsigned short _height;
static char *_space_line = NULL;
static u8 *_grid = NULL;
static int _fps_cap = -1;
static char *_argv[3];


#define DIE(STR)  do {   \
     perror(STR);        \
     exit(EXIT_FAILURE); \
} while (0)

#define NB_COLORS  37
const char *bg_colors[NB_COLORS] = {
     "\033[48;2;7;7;7m ",        // 0x07,0x07,0x07 #070707
     "\033[48;2;31;7;7m ",       // 0x1F,0x07,0x07 #1F0707
     "\033[48;2;47;7;7m ",       // 0x2F,0x07,0x07 #2F0707
     "\033[48;2;71;15;7m ",      // 0x47,0x0F,0x07 #470F07
     "\033[48;2;87;23;7m ",      // 0x57,0x17,0x07 #571707
     "\033[48;2;103;23;7m ",     // 0x67,0x1F,0x07 #671F07
     "\033[48;2;119;23;7m ",     // 0x77,0x1F,0x07 #771F07
     "\033[48;2;143;39;7m ",     // 0x8F,0x27,0x07 #8F2707
     "\033[48;2;159;47;7m ",     // 0x9F,0x2F,0x07 #9F2F07
     "\033[48;2;175;63;7m ",     // 0xAF,0x3F,0x07 #AF3F07
     "\033[48;2;191;71;7m ",     // 0xBF,0x47,0x07 #BF4707
     "\033[48;2;199;71;7m ",     // 0xC7,0x47,0x07 #C74707
     "\033[48;2;223;79;7m ",     // 0xDF,0x4F,0x07 #DF4F07
     "\033[48;2;223;87;7m ",     // 0xDF,0x57,0x07 #DF5707
     "\033[48;2;223;87;7m ",     // 0xDF,0x57,0x07 #DF5707
     "\033[48;2;215;95;7m ",     // 0xD7,0x5F,0x07 #D75F07
     "\033[48;2;215;95;7m ",     // 0xD7,0x5F,0x07 #D75F07
     "\033[48;2;215;103;15m ",   // 0xD7,0x65,0x0F #D7650F
     "\033[48;2;207;111;15m ",   // 0xCF,0x6F,0x0F #CF6F0F
     "\033[48;2;207;119;15m ",   // 0xCF,0x77,0x0F #CF770F
     "\033[48;2;207;127;15m ",   // 0xCF,0x7F,0x0F #CF7F0F
     "\033[48;2;207;135;23m ",   // 0xCF,0x87,0x17 #CF8717
     "\033[48;2;199;135;23m ",   // 0xC7,0x87,0x17 #C78717
     "\033[48;2;199;143;23m ",   // 0xC7,0x8F,0x17 #C78F17
     "\033[48;2;199;151;31m ",   // 0xC7,0x97,0x1F #C7971F
     "\033[48;2;191;159;31m ",   // 0xBF,0x9F,0x1F #BF9F1F
     "\033[48;2;191;159;31m ",   // 0xBF,0x9F,0x1F #BF9F1F
     "\033[48;2;191;167;39m ",   // 0xBF,0xA7,0x27 #BFA727
     "\033[48;2;191;167;39m ",   // 0xBF,0xA7,0x27 #BFA727
     "\033[48;2;191;175;47m ",   // 0xBF,0xAF,0x2F #BFAF2F
     "\033[48;2;183;175;47m ",   // 0xB7,0xAF,0x2F #B7AF2F
     "\033[48;2;183;183;47m ",   // 0xB7,0xB7,0x2F #B7B72F
     "\033[48;2;183;183;55m ",   // 0xB7,0xB7,0x37 #B7B737
     "\033[48;2;207;207;111m ",  // 0xCF,0xCF,0x6F #CFCF6F
     "\033[48;2;223;223;159m ",  // 0xDF,0xDF,0x9F #DFDF9F
     "\033[48;2;239;239;199m ",  // 0xEF,0xEF,0xC7 #EFEFC7
     "\033[48;2;255;255;255m ",  // 0xFF,0xFF,0xFF #FFFFFF
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
_spread_fire(int x, int y)
{
     u8 *old_value;
     u8 new_value;
     u8 r = rand() % 3;
     int pos;

     pos = x + y * _width + r;
     if (pos < 0)
         pos = 0;
     old_value = _grid + pos ;
     new_value = _grid[x + (y + 1) * _width] - (r & 1);
     if (new_value >= NB_COLORS)
       new_value = 0;

     if (*old_value != new_value)
       {
          *old_value = new_value;
          _go_to(pos % _width, pos / _width);
          xwrite(bg_colors[new_value], strlen(bg_colors[new_value]));
       }
}

static void
_display_fps(double fps, double avg)
{
   char buf[64] = {};
   int len;

   /* move to last line */
   len = snprintf(buf, sizeof(buf), "\033[%dH", _height); // Positions start at 1
   xwrite(buf, len);

   /* color fg white color, bg black, clear line */
   const char *esc= "\033[38;5;232m\033[48;5;255m\033[2K";
   xwrite(esc, strlen(esc));

   len = snprintf(buf, sizeof(buf), "%dx%d -> %6.1f fps (avg: %6.1f fps)",
                  _width, _height, fps, avg);
   xwrite(buf, len);
}

static void
_do_fire(double fps, double avg)
{
   int x, y;

   /* for each column */
   for(x = 0; x < _width; x++)
     {
        /* from bottom to top, spread the fire */
        for (y = _height -2; y >= 0; y--)
          {
             _spread_fire(x, y);
          }
     }
   _display_fps(fps, avg);
}

static void
init_screen(void)
{
   int i;
   u8 *last_line = _grid + _width * (_height - 1);
   const u8 white_color_idx = NB_COLORS -1;
   const char *esc_clear_screen = "\033[2J";
   const char *esc_color_black_bg = "\033[48;5;232m";
   const char *esc_color_black_fg = "\033[38;5;232m";

   xwrite(esc_color_black_bg, strlen(esc_color_black_bg));
   xwrite(esc_color_black_fg, strlen(esc_color_black_fg));
   xwrite(esc_clear_screen, strlen(esc_clear_screen));

   /* last line is white line */
   _go_to(0, _height - 1);
   xwrite(bg_colors[white_color_idx],
          strlen(bg_colors[white_color_idx]) -1); // skip space in color
   xwrite(_space_line, _width);
   for (i = 0; i < _width; i++)
     last_line[i] = white_color_idx;
}



static int
run(void)
{
   struct winsize w;
   struct timespec first, start, end;
   clockid_t clk;
   int i;
   double fps = 0.0, avg = 0.0;
   uint64_t n = 0;
   int64_t over = 0;

   if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1)
     DIE("failed to get terminal size with ioctl");

   _width = w.ws_col;
   _height = w.ws_row;

   free(_space_line);
   _space_line = malloc(sizeof(char) * _width);
   if (!_space_line)
     DIE("malloc");
   for (i = 0; i < _width; i++)
     _space_line[i] = ' ';

   free(_grid);
   _grid = calloc(_width * _height, sizeof(u8));
   if (!_grid)
     DIE("calloc");

   init_screen();

   clk = CLOCK_MONOTONIC_COARSE;
   if (clock_gettime(clk, &first))
       DIE("clock_gettime");
   start = first;

   while (1)
     {
        int64_t delta_ns;

        _do_fire(fps, avg);

        if (clock_gettime(clk, &end))
            DIE("clock_gettime");

        /* compute fps */
        delta_ns = end.tv_nsec - start.tv_nsec
           + 1000000000 * (end.tv_sec - start.tv_sec);

        if (_fps_cap > 0)
          {
            int us = ((1000000000/_fps_cap) - delta_ns) / 1000;
            if (us > over)
              {
                usleep(us - over);
                over = 0;
                delta_ns += (us - over) * 1000;
                end.tv_nsec += (us - over) * 1000;
              }
            else
              over -= us;
          }
        if (delta_ns)
          fps = 1000000000.0 / ((double)delta_ns);

        delta_ns = end.tv_nsec - first.tv_nsec
           + 1000000000 * (end.tv_sec - first.tv_sec);
        avg = ((double)n * 1000000000.0) / ((double)delta_ns);
        start = end;
        n++;
     }

   return EXIT_SUCCESS;
}

static void
_on_sig_winch(int signo)
{
   (void)signo;

   execvp(_argv[0], _argv);
   DIE("execvp");
}

int main (int argc, char *argv[])
{
   struct sigaction sa;

   _argv[0] = argv[0];
   _argv[1] = _argv[2] = NULL;
   if (argc == 2)
     {
        _fps_cap = atoi(argv[1]);
        _argv[1] = argv[1];
     }

   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;
   sa.sa_handler = _on_sig_winch;
   if (sigaction(SIGWINCH, &sa, NULL) == -1)
     DIE("sigaction");

   return run();
}
