#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

// we need this since we're not defining a main() function.
// if we didn't #define this, SDL would try to #define main SDL_main
// and put startup code in main and also die becasue we aren't calling main.
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

/////////////////////
// metatable names //
/////////////////////

static const char *EU_WINDOW_MT = "eu.Window";
static const char *EU_TEXTURE_MT = "eu.Texture";
static const char *EU_SPRITE_MT = "eu.Sprite";
static const char *EU_SOUND_MT = "eu.Sound";
static const char *EU_MUSIC_MT = "eu.Music";
static const char *EU_SDLK_TO_STR_TBL = "eu.sdlk_to_str";
static const char *EU_STR_TO_SDLK_TBL = "eu.str_to_sdlk";
static const char *EU_KEYMOD_TO_STR_TBL = "eu.keymod_to_str";
static const char *EU_STR_TO_KEYMOD_TBL = "eu.str_to_keymod";

/////////////////
// struct defs //
/////////////////
typedef struct eu_Window {
	SDL_Window *window;
	SDL_Renderer *renderer;
	int closed;
} eu_Window;

typedef struct eu_Texture {
	SDL_Texture *texture;
	int raw_w;
	int raw_h;
	SDL_Rect frame; // maybe get rid of?
	int is_framed;  // this too
} eu_Texture;

typedef struct eu_Sound {
	Mix_Chunk *chunk;
	int channel;
} eu_Sound;

typedef struct eu_Music {
	Mix_Music *music;
} eu_Music;

////////////////////////
// SDL Keyboard state //
////////////////////////
const Uint8 *KEYBOARD_STATE;

///////////////////
// main function //
///////////////////
int main(int argc, char **argv)
{
}

