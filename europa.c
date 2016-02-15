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

////////////////////////
// General functions: //
// eu.*               //
////////////////////////

// eu.Init()
//   Initialize Europa; call at start of program
static int eu_init(lua_State *L)
{
	SDL_SetMainReady(); // because SDL_MAIN_HANDLED
	SDL_Init(SDL_INIT_EVERYTHING); // everything for now
	int flags = IMG_INIT_JPG | IMG_INIT_PNG; // I don't think we need TIF
	int init_res = IMG_Init(flags);
	if ((init_res & flags) != flags) {
		lua_pushfstring(L,"SDL_image init failed: %s\n",IMG_GetError());
		lua_error(L);
	}

	Mix_AllocateChannels(100);
	int merr = Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT,2,1024);
	if (merr) {
		lua_pushfstring(L,"eu.init-Mix_OpenAudio: %s\n",Mix_GetError());
		lua_error(L);
	}

	return 0;
}

// eu.quit()
//   Tear down Europa; call at end of program
static int eu_quit(lua_State *L)
{
	Mix_CloseAudio();
	Mix_Quit();
	IMG_Quit();
	SDL_Quit();
	return 0;
}

// eu.delay(ms:int)
static int eu_delay(lua_State *L)
{
	int ms = luaL_checkinteger(L,1);
	SDL_Delay(ms);
	return 0;
}

// eu.main module table
static const luaL_Reg eu_module_fns[] = {
	{"init", &eu_init},
	{"quit", &eu_quit},
	{"delay", &eu_delay},
	{NULL, NULL}
};

/////////////////////
// Event handling: //
// eu.event.*      //
/////////////////////

// a helper function to set up our keymap tables
static void euh_init_keymaps(lua_State *L)
{
	// SDLK_* const-> string
	lua_newtable(L);
	// string -> SDLK_* const
	lua_newtable(L);
	
	// put the >800 lines of setting fields in a separate file!
#include "euh_init_keymaps.inc"
	
	// set the tables in the registery
	lua_setfield(L, LUA_REGISTRYINDEX, EU_STR_TO_SDLK_TBL);
	lua_setfield(L, LUA_REGISTRYINDEX, EU_SDLK_TO_STR_TBL);

}


///////////////////
// main function //
///////////////////
int main(int argc, char **argv)
{
}

