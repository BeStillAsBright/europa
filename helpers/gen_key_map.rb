def push_vals(f, sdl_const, luna_const)
    f.puts "\t// #{sdl_const}"
    f.puts "\tlua_pushliteral(L,\"#{luna_const}\");"
    f.puts "\tlua_pushinteger(L,#{sdl_const});"
    f.puts "\tlua_rawset(L,-3);"
    f.puts "\tlua_pushliteral(L,\"#{luna_const}\");"
    f.puts "\tlua_rawseti(L,-3,#{sdl_const});"
end

File.open("keymap.txt", "w") do |f|
  f.puts "static void lh_luna_init_keymaps(lua_State *L)"
  f.puts "{"
  f.puts "\t// SDLK_* const-> string"
  f.puts "\tlua_newtable(L);"

  f.puts "\t// string -> SDLK_* const"
  f.puts "\tlua_newtable(L);"

  # regular number keys
  f.puts "\n\t// number keys //\n"
  (0..9).each do |c|
    sdl_const = "SDLK_#{c}"
    luna_const = "#{c}"
    push_vals(f, sdl_const, luna_const)
  end

  # regular letters
  f.puts "\n\t// letters //\n"
  ('a'..'z').each do |c|
    sdl_const = "SDLK_#{c}"
    luna_const = "#{c}"
    push_vals(f, sdl_const, luna_const)
  end

  # F keys
  f.puts "\n\t// F keys //\n"
  (1..24).each do |c|
    sdl_const = "SDLK_F#{c}"
    luna_const = "f#{c}"
    push_vals(f, sdl_const, luna_const)
  end

  # keypad numbers
  f.puts "\n\t// KP numbers //\n"
  (0..9).each do |c|
    sdl_const = "SDLK_KP_#{c}"
    luna_const = "kp#{c}"
    push_vals(f, sdl_const, luna_const)
  end

  # keypad symbols
  f.puts "\n\t// KP symbols //\n"
  push_vals(f, "SDLK_KP_COMMA", "kp,")
  push_vals(f, "SDLK_KP_DECIMAL", "kp.")
  push_vals(f, "SDLK_KP_DIVIDE", "kp/")
  push_vals(f, "SDLK_KP_ENTER", "kpenter")
  push_vals(f, "SDLK_KP_EQUALS", "kp=")
  push_vals(f, "SDLK_KP_MINUS", "kp-")
  push_vals(f, "SDLK_KP_MULTIPLY", "kp*")
  push_vals(f, "SDLK_KP_PLUS", "kp+")

  # symbol keys
  f.puts "\n\t// symbol keys //\n"
  push_vals(f, "SDLK_SPACE", "space")
  push_vals(f, "SDLK_AMPERSAND", "&")
  push_vals(f, "SDLK_ASTERISK", "*")
  push_vals(f, "SDLK_AT", "@")
  push_vals(f, "SDLK_BACKSLASH", "\\\\")
  push_vals(f, "SDLK_BACKQUOTE", "`")
  push_vals(f, "SDLK_CARET", "^")
  push_vals(f, "SDLK_COMMA", ",")
  push_vals(f, "SDLK_DOLLAR", "$")
  push_vals(f, "SDLK_EQUALS", "=")
  push_vals(f, "SDLK_EXCLAIM", "!")
  push_vals(f, "SDLK_GREATER", ">")
  push_vals(f, "SDLK_HASH", '#')
  push_vals(f, "SDLK_LEFTBRACKET", "[")
  push_vals(f, "SDLK_LEFTPAREN", "(")
  push_vals(f, "SDLK_LESS", "<")
  push_vals(f, "SDLK_MINUS", "-")
  push_vals(f, "SDLK_PERCENT", "%")
  push_vals(f, "SDLK_PERIOD", ".")
  push_vals(f, "SDLK_PLUS", "+")
  push_vals(f, "SDLK_QUESTION", "?")
  push_vals(f, "SDLK_QUOTE", "'")
  push_vals(f, "SDLK_QUOTEDBL", '\\"')
  push_vals(f, "SDLK_RIGHTBRACKET", "]")
  push_vals(f, "SDLK_RIGHTPAREN", ")")
  push_vals(f, "SDLK_SEMICOLON", ";")
  push_vals(f, "SDLK_SLASH", "/")
  push_vals(f, "SDLK_UNDERSCORE", "_")

  # nav keys
  f.puts "\n\t// Nav keys //\n"
  push_vals(f, "SDLK_DOWN", "down")
  push_vals(f, "SDLK_LEFT", "left")
  push_vals(f, "SDLK_RIGHT", "right")
  push_vals(f, "SDLK_UP", "up")
  push_vals(f, "SDLK_END", "end")
  push_vals(f, "SDLK_HOME", "home")
  push_vals(f, "SDLK_PAGEDOWN", "pagedown")
  push_vals(f, "SDLK_PAGEUP", "pageup")

  # editing keys
  f.puts "\n\t// Editing keys //\n"
  push_vals(f, "SDLK_BACKSPACE", "backspace")
  push_vals(f, "SDLK_CLEAR", "clear")
  push_vals(f, "SDLK_DELETE", "delete")
  push_vals(f, "SDLK_INSERT", "insert")
  push_vals(f, "SDLK_RETURN", "return")
  push_vals(f, "SDLK_TAB", "tab")

  # modifier keys
  f.puts "\n\t// Modifier keys //\n"
  push_vals(f, "SDLK_CAPSLOCK", "capslock")
  push_vals(f, "SDLK_MODE", "mode")
  push_vals(f, "SDLK_NUMLOCKCLEAR", "numlock")
  push_vals(f, "SDLK_SCROLLLOCK", "scrolllock")
  push_vals(f, "SDLK_LGUI", "lgui")
  push_vals(f, "SDLK_RGUI", "rgui")
  push_vals(f, "SDLK_LSHIFT", "lshift")
  push_vals(f, "SDLK_RSHIFT", "rshift")
  push_vals(f, "SDLK_LALT", "lalt")
  push_vals(f, "SDLK_RALT", "ralt")
  push_vals(f, "SDLK_LCTRL", "lctrl")
  push_vals(f, "SDLK_RCTRL", "rctrl")

  # miscellaneous keys
  f.puts "\n\t// misc keys //\n"
  push_vals(f, "SDLK_PAUSE", "pause")
  push_vals(f, "SDLK_ESCAPE", "escape")
  push_vals(f, "SDLK_HELP", "help")
  push_vals(f, "SDLK_MENU", "menu")
  push_vals(f, "SDLK_PRINTSCREEN", "printscreen")
  push_vals(f, "SDLK_SYSREQ", "sysreq")
  push_vals(f, "SDLK_APPLICATION", "application")
  push_vals(f, "SDLK_POWER", "power")
  push_vals(f, "SDLK_CURRENCYUNIT", "currencyunit")
  push_vals(f, "SDLK_UNDO", "undo")

  # set metatables
  # LUA_REGISTRYINDEX
  f.puts "\tlua_setfield(L, LUA_REGISTRYINDEX, LUNA_STR_TO_SDLK_TBL);"
  f.puts "\tlua_setfield(L, LUA_REGISTRYINDEX, LUNA_SDLK_TO_STR_TBL);"

  #end of fn
  f.puts "}" 
end
