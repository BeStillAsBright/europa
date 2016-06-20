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

//////////////////////////
// lua compat functions //
//////////////////////////
void eu_lua_copy (lua_State *L, int from, int to) {
  int abs_to = lua_absindex(L, to);
  luaL_checkstack(L, 1, "not enough stack slots");
  lua_pushvalue(L, from);
  lua_replace(L, abs_to);
}

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
	
	int mixflags = MIX_INIT_FLAC | MIX_INIT_OGG;
#ifdef EU_USE_MP3
	mixflags |= MIX_INIT_MP3;
#endif // EU_USE_MP3
	int mixinitted = Mix_Init(mixflags);
	if ((mixinitted&mixflags) != mixflags) {
		lua_pushfstring(L, "eu.init-Mix_Init: %s\n", Mix_GetError());
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

static void euh_set_keycode(lua_State *L, SDL_Keysym *ks)
{
	lua_pushstring(L, LUNA_SDLK_TO_STR_TBL); // evt, tblnm
	lua_gettable(L, LUA_REGISTRYINDEX); // evt, tbl
	lua_pushinteger(L, ks->sym); // evt, tbl, int
	lua_gettable(L, -2); // evt, tbl, str
	lua_remove(L, -2); // remove lookup table from stack
	lua_setfield(L,-2,"key"); // set field in event table
}

static void euh_make_key_event(lua_State *L, SDL_Event *e)
{
	// add our table
	lua_newtable(L);

	// set the table's "type" field so user can switch on it
	if (e->type == SDL_KEYDOWN) {
		lua_pushliteral(L,"key_down");
	} else {
		lua_pushliteral(L,"key_up");
	}
	lua_setfield(L,-2,"type");

	// add "window_id" field
	lua_pushinteger(L,e->key.windowID);
	lua_setfield(L,-2,"window_id");

	// add "timestamp" field
	lua_pushinteger(L, e->key.timestamp);
	lua_setfield(L,-2,"timestamp");

	// set the "repeat" field; true if a repeated press
	lua_pushboolean(L,e->key.repeat);
	lua_setfield(L,-2,"repeat");

	// set the "state" field to "pressed" or "released"
	if (e->key.state == SDL_PRESSED) {
		lua_pushliteral(L,"pressed");
	} else {
		lua_pushliteral(L,"released");
	}
	lua_setfield(L,-2,"state");

	// set keysym info. It gets ugly here, so we're using a helper fn.
	euh_set_keycode(L,&e->key.keysym);
}

static void euh_make_mouse_motion_event(lua_State *L, SDL_Event *e)
{
	// the event table to be returned
	lua_newtable(L);

	// add type field
	lua_pushliteral(L,"mouse_motion");
	lua_setfield(L,-2,"type");

	// add timestamp
	lua_pushinteger(L,e->motion.timestamp);
	lua_setfield(L,-2,"timestamp");

	// add window_id
	lua_pushinteger(L,e->motion.windowID);
	lua_setfield(L,-2,"window_id");

	// add is_touch field
	int istouch = (e->motion.which == SDL_TOUCH_MOUSEID);
	lua_pushboolean(L,istouch);
	lua_setfield(L,-2,"is_touch");

	// set state table fields (event.buttons)
	lua_newtable(L); // table for event.buttons
	int left = (e->motion.state & SDL_BUTTON_LMASK);
	int middle = (e->motion.state & SDL_BUTTON_MMASK);
	int right = (e->motion.state & SDL_BUTTON_RMASK);
	int x1 = (e->motion.state & SDL_BUTTON_X1MASK);
	int x2 = (e->motion.state & SDL_BUTTON_X2MASK);
	lua_pushboolean(L,left);
	lua_setfield(L,-2,"left");
	lua_pushboolean(L,middle);
	lua_setfield(L,-2,"middle");
	lua_pushboolean(L,right);
	lua_setfield(L,-2,"right");
	lua_pushboolean(L,x1);
	lua_setfield(L,-2,"x1");
	lua_pushboolean(L,x2);
	lua_setfield(L,-2,"x2");
	lua_setfield(L,-2,"buttons"); // set event.buttons

	// set x field
	lua_pushinteger(L,e->motion.x);
	lua_setfield(L,-2,"x");

	// set y field
	lua_pushinteger(L, e->motion.y);
	lua_setfield(L,-2,"y");

	// set x_rel field
	lua_pushinteger(L, e->motion.xrel);
	lua_setfield(L,-2,"x_rel");
	
	// set y_rel field
	lua_pushinteger(L, e->motion.yrel);
	lua_setfield(L,-2,"y_rel");
}

static void euh_make_mouse_button_event(lua_State *L, SDL_Event *e)
{
	// push our table to return
	lua_newtable(L);

	// set type field
	if (e->type == SDL_MOUSEBUTTONDOWN) {
		lua_pushliteral(L,"mouse_down");
	} else {
		lua_pushliteral(L,"mouse_up");
	}
	lua_setfield(L,-2,"type");

	// set timestamp
	lua_pushinteger(L,e->button.timestamp);
	lua_setfield(L,-2,"timestamp");

	// window_id
	lua_pushinteger(L,e->button.windowID);
	lua_setfield(L,-2,"window_id");

	// is_touch
	int istouch = (e->button.which == SDL_TOUCH_MOUSEID);
	lua_pushboolean(L,istouch);
	lua_setfield(L,-2,"is_touch");

	// set button field
	switch (e->button.button) {
		case SDL_BUTTON_LEFT:
			lua_pushliteral(L,"left");
			break;
		case SDL_BUTTON_MIDDLE:
			lua_pushliteral(L,"middle");
			break;
		case SDL_BUTTON_RIGHT:
			lua_pushliteral(L,"right");
			break;
		case SDL_BUTTON_X1:
			lua_pushliteral(L,"x1");
			break;
		case SDL_BUTTON_X2:
			lua_pushliteral(L,"x2");
			break;
	}
	lua_setfield(L,-2,"button");

	// set state field
	if (e->button.state == SDL_PRESSED) {
		lua_pushliteral(L,"pressed");
	} else {
		lua_pushliteral(L,"released");
	}
	lua_setfield(L,-2,"state");
	
	// x field
	lua_pushinteger(L,e->button.x);
	lua_setfield(L,-2,"x");
	
	// y field
	lua_pushinteger(L,e->button.y);
	lua_setfield(L,-2,"y");
}

static void euh_make_mouse_wheel_event(lua_State *L, SDL_Event *e)
{
	lua_newtable(L);
	
	// timestamp
	lua_pushinteger(L, e->wheel.timestamp);
	lua_setfield(L,-2,"timestamp");
	// window_id
	lua_pushinteger(L, e->wheel.windowID);
	lua_setfield(L,-2,"window_id");
	// is_touch
	int istouch = (e->wheel.which == SDL_TOUCH_MOUSEID);
	lua_pushboolean(L,istouch);
	lua_setfield(L,-2,"is_touch");
	// x
	lua_pushinteger(L,e->wheel.x);
	lua_setfield(L,-2,"x");
	// y
	lua_pushinteger(L,e->wheel.y);
	lua_setfield(L,-2,"y");
}

static void euh_make_controller_axis_event(lua_State *L, SDL_Event *e)
{
	// event table
	lua_newtable(L);

	// event type
	lua_pushliteral(L,"controller_axis");
	lua_setfield(L,-2,"type");

	// timestamp
	lua_pushinteger(L,e->caxis.timestamp);
	lua_setfield(L,-2,"timestamp");

	// controller_id
	lua_pushinteger(L,e->caxis.which);
	lua_setfield(L,-2,"controller_id");

	// axis
	switch (e->caxis.axis) {
		case SDL_CONTROLLER_AXIS_LEFTX:
			lua_pushliteral(L,"left_x");
			break;
		case SDL_CONTROLLER_AXIS_LEFTY:
			lua_pushliteral(L,"left_y");
			break;
		case SDL_CONTROLLER_AXIS_RIGHTX:
			lua_pushliteral(L,"right_x");
			break;
		case SDL_CONTROLLER_AXIS_RIGHTY:
			lua_pushliteral(L,"right_y");
			break;
		case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
			lua_pushliteral(L,"left_trigger");
			break;
		case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
			lua_pushliteral(L,"right_trigger");
			break;
	}
	lua_setfield(L,-2,"axis");

	// value:
	lua_pushinteger(L,e->caxis.value);
	lua_setfield(L,-2,"value");
}

static void euh_make_controller_button_event(lua_State *L, SDL_Event *e)
{
	// table
	lua_newtable(L);

	// type
	if (e->cbutton.type == SDL_CONTROLLERBUTTONDOWN) {
		lua_pushliteral(L,"controller_button_down");
	} else {
		lua_pushliteral(L,"controller_button_up");
	}
	lua_setfield(L,-2,"type");

	// timestamp
	lua_pushinteger(L,e->cbutton.timestamp);
	lua_setfield(L,-2,"timestamp");

	// controller_id
	lua_pushinteger(L,e->cbutton.which);
	lua_setfield(L,-2,"controller_id");

	// button
	switch (e->cbutton.button) {
		case SDL_CONTROLLER_BUTTON_A:
			lua_pushliteral(L,"a");
			break;
		case SDL_CONTROLLER_BUTTON_B:
			lua_pushliteral(L,"b");
			break;
		case SDL_CONTROLLER_BUTTON_X:
			lua_pushliteral(L,"x");
			break;
		case SDL_CONTROLLER_BUTTON_Y:
			lua_pushliteral(L,"y");
			break;
		case SDL_CONTROLLER_BUTTON_BACK:
			lua_pushliteral(L,"back");
			break;
		case SDL_CONTROLLER_BUTTON_GUIDE:
			lua_pushliteral(L,"guide");
			break;
		case SDL_CONTROLLER_BUTTON_START:
			lua_pushliteral(L,"start");
			break;
		case SDL_CONTROLLER_BUTTON_LEFTSTICK:
			lua_pushliteral(L,"left_stick");
			break;
		case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
			lua_pushliteral(L,"right_stick");
			break;
		case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
			lua_pushliteral(L,"left_shoulder");
			break;
		case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
			lua_pushliteral(L,"right_shoulder");
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			lua_pushliteral(L,"dpad_up");
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			lua_pushliteral(L,"dpad_down");
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			lua_pushliteral(L,"dpad_left");
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			lua_pushliteral(L,"dpad_right");
			break;
	}
	lua_setfield(L,-2,"button");

	// state
	if (e->cbutton.state == SDL_PRESSED) {
		lua_pushliteral(L,"pressed");
	} else {
		lua_pushliteral(L,"released");
	}
	lua_setfield(L,-2,"state");
}

static void euh_make_controller_device_event(lua_State *L, SDL_Event *e)
{
	lua_newtable(L);
	switch (e->type) {
		case SDL_CONTROLLERDEVICEADDED:
			lua_pushliteral(L,"controller_added");
			break;
		case SDL_CONTROLLERDEVICEREMAPPED:
			lua_pushliteral(L,"controller_remapped");
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			lua_pushliteral(L,"controller_removed");
			break;
	}
	lua_setfield(L,-2,"type");
	
	// timestamp
	lua_pushinteger(L,e->cdevice.timestamp);
	lua_setfield(L,-2,"timestamp");
	
	// controller_id
	lua_pushinteger(L,e->cdevice.which);
	lua_setfield(L,-2,"controller_id");
}

static void euh_make_window_event(lua_State *L, SDL_Event *e)
{
	lua_pushnil(L); // this gets copied over by the event type
	lua_newtable(L);

	// timestamp
	lua_pushinteger(L,e->window.timestamp);
	lua_setfield(L,-2,"timestamp");
	// window_id
	lua_pushinteger(L,e->window.windowID);
	lua_setfield(L,-2,"window_id");
	// type (and data)
	switch (e->window.event) {
		case SDL_WINDOWEVENT_SHOWN:
			lua_pushliteral(L,"window_shown");
			eu_lua_copy(L,-1,-3);
			lua_setfield(L,-2,"type");
			break;
		case SDL_WINDOWEVENT_HIDDEN:
			lua_pushliteral(L,"window_hidden");
			eu_lua_copy(L,-1,-3);
			lua_setfield(L,-2,"type");
			break;
		case SDL_WINDOWEVENT_EXPOSED:
			lua_pushliteral(L,"window_exposed");
			eu_lua_copy(L,-1,-3);
			lua_setfield(L,-2,"type");
			break;
		case SDL_WINDOWEVENT_MOVED:
			lua_pushinteger(L,e->window.data1);
			lua_setfield(L,-2,"x");
			lua_pushinteger(L,e->window.data2);
			lua_setfield(L,-2,"y");
			lua_pushliteral(L,"window_moved");
			eu_lua_copy(L,-1,-3);
			lua_setfield(L,-2,"type");
			break;
		case SDL_WINDOWEVENT_RESIZED:
			lua_pushinteger(L,e->window.data1);
			lua_setfield(L,-2,"w");
			lua_pushinteger(L,e->window.data2);
			lua_setfield(L,-2,"h");
			lua_pushliteral(L,"window_resized");
			eu_lua_copy(L,-1,-3);
			lua_setfield(L,-2,"type");
			break;
		case SDL_WINDOWEVENT_MINIMIZED:
			lua_pushliteral(L,"window_minimized");
			eu_lua_copy(L,-1,-3);
			lua_setfield(L,-2,"type");
			break;
		case SDL_WINDOWEVENT_MAXIMIZED:
			lua_pushliteral(L,"window_maximized");
			eu_lua_copy(L,-1,-3);
			lua_setfield(L,-2,"type");
			break;
		case SDL_WINDOWEVENT_RESTORED:
			lua_pushliteral(L,"window_restored");
			eu_lua_copy(L,-1,-3);
			lua_setfield(L,-2,"type");
			break;
		case SDL_WINDOWEVENT_ENTER:
			lua_pushliteral(L,"window_enter");
			eu_lua_copy(L,-1,-3);
			lua_setfield(L,-2,"type");
			break;
		case SDL_WINDOWEVENT_LEAVE:
			lua_pushliteral(L,"window_leave");
			eu_lua_copy(L,-1,-3);
			lua_setfield(L,-2,"type");
			break;
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			lua_pushliteral(L,"window_focus_gained");
			eu_lua_copy(L,-1,-3);
			lua_setfield(L,-2,"type");
			break;
		case SDL_WINDOWEVENT_FOCUS_LOST:
			lua_pushliteral(L,"window_focus_lost");
			eu_lua_copy(L,-1,-3);
			lua_setfield(L,-2,"type");
			break;
		case SDL_WINDOWEVENT_CLOSE:
			lua_pushliteral(L,"window_close");
			eu_lua_copy(L,-1,-3);
			lua_setfield(L,-2,"type");
			break;
	}
}

// eu.event.poll() -> type: string, event: eu.Event
static int eu_event_poll(lua_State *L)
{
	SDL_Event event;
	SDL_PollEvent(&event);
	// push a table to use as our return value
	switch (event.type) {
		// keyboard events
		case SDL_KEYDOWN:
			lua_pushliteral(L,"key_down");
			euh_make_key_event(L,&event);
			break;
		case SDL_KEYUP:
			lua_pushliteral(L,"key_up");
			euh_make_key_event(L, &event);
			break;
		// mouse events
		case SDL_MOUSEMOTION:
			lua_pushliteral(L,"mouse_motion");
			euh_make_mouse_motion_event(L,&event);
			break;
		case SDL_MOUSEBUTTONDOWN:
			lua_pushliteral(L,"mouse_down");
			euh_make_mouse_button_event(L,&event);
			break;
		case SDL_MOUSEBUTTONUP:
			lua_pushliteral(L,"mouse_up");
			euh_make_mouse_button_event(L,&event);
			break;
		case SDL_MOUSEWHEEL:
			lua_pushliteral(L,"mouse_wheel");
			euh_make_mouse_wheel_event(L,&event);
			break;
		// controller events
		case SDL_CONTROLLERAXISMOTION:
			lua_pushliteral(L,"controller_axis");
			euh_make_controller_axis_event(L,&event);
			break;
		case SDL_CONTROLLERBUTTONDOWN:
			lua_pushliteral(L,"controller_button_down");
			euh_make_controller_button_event(L,&event);
			break;
		case SDL_CONTROLLERBUTTONUP:
			lua_pushliteral(L,"controller_button_up");
			euh_make_controller_button_event(L,&event);
			break;
		case SDL_CONTROLLERDEVICEADDED:
			lua_pushliteral(L,"controller_added");
			euh_make_controller_device_event(L,&event);
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			lua_pushliteral(L,"controller_removed");
			euh_make_controller_device_event(L,&event);
			break;
		case SDL_CONTROLLERDEVICEREMAPPED:
			lua_pushliteral(L,"controller_remapped");
			euh_make_controller_device_event(L,&event);
			break;
		case SDL_WINDOWEVENT:
			// helper function handles pushing the type too!
			euh_make_window_event(L,&event);
		case SDL_QUIT:
			// this is small enough to handle here
			lua_pushliteral(L,"quit");
			lua_newtable(L);
			lua_pushvalue(L,-2); // push copy of "quit"
			lua_setfield(L,-2,"type");
			lua_pushinteger(L, event.quit.timestamp);
			lua_setfield(L,-2,"timestamp");
			break;
		default:
			break;
	}
	return 2; // returns: type:string, event:table
}

// eu.event module
static const luaL_Reg eu_event_module_fns[] = {
	{"poll", &eu_event_poll},
	{NULL,NULL}
};

// ////////////////////////////
// luna.keyboard.* functions //
// ////////////////////////////

static int euh_init_keymod_table(lua_State *L)
{
	// str->kmod
	lua_newtable(L);
	// kmod->str
	lua_newtable(L);

	// KMOD_NONE
	lua_pushliteral(L,"none");
	lua_pushinteger(L,KMOD_NONE);
	lua_rawset(L,-3);
	lua_pushliteral(L,"none");
	lua_rawseti(L,-3,KMOD_NONE);
	// KMOD_LSHIFT
	lua_pushliteral(L,"lshift");
	lua_pushinteger(L,KMOD_LSHIFT);
	lua_rawset(L,-3);
	lua_pushliteral(L,"lshift");
	lua_rawseti(L,-3,KMOD_LSHIFT);
	// KMOD_RSHIFT
	lua_pushliteral(L,"rshift");
	lua_pushinteger(L,KMOD_RSHIFT);
	lua_rawset(L,-3);
	lua_pushliteral(L,"rshift");
	lua_rawseti(L,-3,KMOD_RSHIFT);
	// KMOD_LCTRL
	lua_pushliteral(L,"lctrl");
	lua_pushinteger(L,KMOD_LCTRL);
	lua_rawset(L,-3);
	lua_pushliteral(L,"lctrl");
	lua_rawseti(L,-3,KMOD_LCTRL);
	// KMOD_RCTRL
	lua_pushliteral(L,"rctrl");
	lua_pushinteger(L,KMOD_RCTRL);
	lua_rawset(L,-3);
	lua_pushliteral(L,"rctrl");
	lua_rawseti(L,-3,KMOD_RCTRL);
	// KMOD_LALT
	lua_pushliteral(L,"lalt");
	lua_pushinteger(L,KMOD_LALT);
	lua_rawset(L,-3);
	lua_pushliteral(L,"lalt");
	lua_rawseti(L,-3,KMOD_LALT);
	// KMOD_RALT
	lua_pushliteral(L,"ralt");
	lua_pushinteger(L,KMOD_RALT);
	lua_rawset(L,-3);
	lua_pushliteral(L,"ralt");
	lua_rawseti(L,-3,KMOD_RALT);
	// KMOD_LGUI
	lua_pushliteral(L,"lgui");
	lua_pushinteger(L,KMOD_LGUI);
	lua_rawset(L,-3);
	lua_pushliteral(L,"lgui");
	lua_rawseti(L,-3,KMOD_LGUI);
	// KMOD_RGUI
	lua_pushliteral(L,"rgui");
	lua_pushinteger(L,KMOD_RGUI);
	lua_rawset(L,-3);
	lua_pushliteral(L,"rgui");
	lua_rawseti(L,-3,KMOD_RGUI);
	// KMOD_NUM
	lua_pushliteral(L,"num");
	lua_pushinteger(L,KMOD_NUM);
	lua_rawset(L,-3);
	lua_pushliteral(L,"num");
	lua_rawseti(L,-3,KMOD_NUM);
	// KMOD_CAPS
	lua_pushliteral(L,"caps");
	lua_pushinteger(L,KMOD_CAPS);
	lua_rawset(L,-3);
	lua_pushliteral(L,"caps");
	lua_rawseti(L,-3,KMOD_CAPS);
	// KMOD_MODE
	lua_pushliteral(L,"mode");
	lua_pushinteger(L,KMOD_MODE);
	lua_rawset(L,-3);
	lua_pushliteral(L,"mode");
	lua_rawseti(L,-3,KMOD_MODE);
	// KMOD_CTRL
	lua_pushliteral(L,"ctrl");
	lua_pushinteger(L,KMOD_CTRL);
	lua_rawset(L,-3);
	lua_pushliteral(L,"ctrl");
	lua_rawseti(L,-3,KMOD_CTRL);
	// KMOD_SHIFT
	lua_pushliteral(L,"shift");
	lua_pushinteger(L,KMOD_SHIFT);
	lua_rawset(L,-3);
	lua_pushliteral(L,"shift");
	lua_rawseti(L,-3,KMOD_SHIFT);
	// KMOD_ALT
	lua_pushliteral(L,"alt");
	lua_pushinteger(L,KMOD_ALT);
	lua_rawset(L,-3);
	lua_pushliteral(L,"alt");
	lua_rawseti(L,-3,KMOD_ALT);
	// KMOD_GUI
	lua_pushliteral(L,"gui");
	lua_pushinteger(L,KMOD_GUI);
	lua_rawset(L,-3);
	lua_pushliteral(L,"gui");
	lua_rawseti(L,-3,KMOD_GUI);

	lua_setfield(L, LUA_REGISTRYINDEX, EU_STR_TO_KEYMOD_TBL);
	lua_setfield(L, LUA_REGISTRYINDEX, EU_KEYMOD_TO_STR_TBL);
	return 0;
}

// eu.keyboard.key_down(key:key_string) -> boolean
static int eu_keyboard_key_down(lua_State *L)
{
	const char *keystr = luaL_checkstring(L, 1);

	lua_pushstring(L, EU_STR_TO_SDLK_TBL); // key, tblnm
	lua_gettable(L, LUA_REGISTRYINDEX); // key, tbl
	lua_pushstring(L, keystr); // key, tbl, key
	lua_gettable(L, -2); // key, tbl, sdlk
	// get the sdlk constant
	int sdlk = lua_tointeger(L, -1);
	int sc = SDL_GetScancodeFromKey(sdlk);
	lua_pushboolean(L, KEYBOARD_STATE[sc]);
	return 1;
}

static int eu_keyboard_mod_down(lua_State *L)
{
	const char *modstr = luaL_checkstring(L, 1);
	lua_pushstring(L, EU_STR_TO_KEYMOD_TBL);
	lua_gettable(L, LUA_REGISTRYINDEX);
	lua_pushstring(L, modstr);
	lua_gettable(L, -2);
	int mod = lua_tointeger(L,-1);
	int ret = mod & SDL_GetModState();
	lua_pushboolean(L, ret);
	return 1;
}

static luaL_Reg eu_keyboard_module_fns[] = {
	{"key_down", &eu_keyboard_key_down},
	{"mod_down", &eu_keyboard_mod_down},
	{NULL,NULL}
};


///////////////////
// main function //
///////////////////
int main(int argc, char **argv)
{
}

