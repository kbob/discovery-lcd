#include "spitmap.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct option longopts[] = {
    { "align",      required_argument, NULL, 'a' },
    { "color",            no_argument, NULL, 'c' },
    { "font",       required_argument, NULL, 'f' },
    { "help",             no_argument, NULL, 'h' },
    { "hinting",    required_argument, NULL, 'H' },
    { "renderer",   required_argument, NULL, 'r' },
    { "resolution", required_argument, NULL, 'R' },
    { "size",       required_argument, NULL, 's' },
    { "translate",  required_argument, NULL, 't' },
    { "visual",     required_argument, NULL, 'v' },
    { NULL }
};

typedef struct keyval {
    const char *key;
    int         val;
} keyval;

const char *progname;

options::options()
    : is_aligned(true),
      is_color(false),
      is_hinting(true),
      font(NULL),
      renderer(R_FREETYPE),
      resolution(72.0),
      size(12.0),
      translation(0.0),
      visual(V_ARGB8888),
      identifier(NULL),
      text(NULL)
{}

static void render_text(const options *opts)
{
    switch (opts->renderer) {
    case R_FREETYPE:

        render_freetype(opts);
        break;

    case R_AGG:
        render_agg(opts);
        break;
    }
}

static void usage(FILE *out = stderr)
{
    fprintf(out, "use: %s [options] identifier string\n", progname);
    exit(out == stderr);
}

static int match_keyval(const char *key, const keyval *keyvals)
{
    for (int i = 0; keyvals->key; i++)
        if (!strcasecmp(key, keyvals[i].key))
            return i;
    return -1;
 }

static bool parse_bool(const char *s)
{
    static const keyval bools[] = {
        { "true",  true  },
        { "yes",   true  },
        { "y",     true  },
        { "1",     true  },
        { "false", false },
        { "no",    false },
        { "n",     false },
        { "0",     false },
        { NULL }
    };
    int i                       = match_keyval(s, bools);
    if (i == -1) {
        fprintf(stderr, "%s: can't parse boolean \"%s\"\n", progname, s);
        usage();
    }
    return bools[i].val;
}

static renderer parse_renderer(const char *s)
{
    static const keyval renderers[]  = {
        { "agg",       R_AGG },
        { "antigrain", R_AGG },
        { "freetype",  R_FREETYPE },
        { NULL }
    };
    int i = match_keyval(s, renderers);
    if (i                           == -1) {
        fprintf(stderr, "%s: can't parse renderer \"%s\"\n", progname, s);
        usage();
    }
    return static_cast<renderer>(renderers[i].val);
}

static double parse_float(const char *s)
{
    char  *tail = NULL;
    double f = strtod(optarg, &tail);
    if (*tail) {
        fprintf(stderr, "%s: can't parse number \"%s\"\n", progname, s);
        usage();
    }
    return f;
}

visual parse_visual(const char *s)
{
    static const keyval visuals[] = {
        { "argb8888", V_ARGB8888 },
        { "rgb888",   V_RGB888   },
        { "rgb565",   V_RGB565   },
        { "argb1555", V_ARGB1555 },
        { "argb4444", V_ARGB4444 },
        { "al88",     V_AL88     },
        { "l8",       V_L8       },
        { "a8",       V_A8       },
        { "al44",     V_AL44     },
        { "a4",       V_A4       },
        { "l4",       V_L4       },
        { NULL }
    };
    
    int i = match_keyval(s, visuals);
    if (i                           == -1) {
        fprintf(stderr, "%s: can't parse visual \"%s\"\n", progname, s);
        usage();
    }
    return static_cast<visual>(visuals[i].val);
}

int main(int argc, char *argv[])
{
    options options;

    progname = argv[0];
    while (true) {
        int opt = getopt_long(argc, argv, "a:cf:hH:r:R:s:t:v:", longopts, NULL);
        if (opt == -1)
            break;
        switch (opt) {

        case 'a':               // --align=y|n
            options.is_aligned = parse_bool(optarg);
            break;

        case 'c':               // --color
            options.is_color = true;
            break;

        case 'f':               // --font=FILE
            options.font = optarg;
            break;

        case 'h':               // --help
            usage(stdout);

        case 'H':               // --hinting=y|n
            options.is_hinting = parse_bool(optarg);
            break;

        case 'r':               // --renderer=agg|freetype
            options.renderer = parse_renderer(optarg);
            break;

        case 'R':               // --resolution=N
            options.resolution = parse_float(optarg);
            break;

        case 's':               // --size=N
            options.size = parse_float(optarg);
            break;

        case 't':               // --translate=N
            options.translation = parse_float(optarg);
            break;

        case 'v':               // --visual=VISUAL
            options.visual = parse_visual(optarg);
            break;

        default:
            usage();
        }
    }
    if (optind != argc - 2)
        usage();

    options.identifier = argv[optind];
    options.text = argv[optind + 1];
    render_text(&options);
}
