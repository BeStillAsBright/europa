
def push_vals(f, sdl_const, luna_const)
    f.puts "\t// #{sdl_const}"
    f.puts "\tlua_pushliteral(L,\"#{luna_const}\");"
    f.puts "\tlua_pushinteger(L,#{sdl_const});"
    f.puts "\tlua_rawset(L,-3);"
    f.puts "\tlua_pushliteral(L,\"#{luna_const}\");"
    f.puts "\tlua_rawseti(L,-3,#{sdl_const});"
end


File.open("keymod.txt", "w") do |f|
  push_vals(f, "KMOD_NONE", "none")
  push_vals(f, "KMOD_LSHIFT", "lshift")
  push_vals(f, "KMOD_RSHIFT", "rshift")
  push_vals(f, "KMOD_LCTRL", "lctrl")
  push_vals(f, "KMOD_RCTRL", "rctrl")
  push_vals(f, "KMOD_LALT", "lalt")
  push_vals(f, "KMOD_RALT", "ralt")
  push_vals(f, "KMOD_LGUI", "lgui")
  push_vals(f, "KMOD_RGUI", "rgui")
  push_vals(f, "KMOD_NUM", "num")
  push_vals(f, "KMOD_CAPS", "caps")
  push_vals(f, "KMOD_MODE", "mode")
  push_vals(f, "KMOD_CTRL", "ctrl")
  push_vals(f, "KMOD_SHIFT", "shift")
  push_vals(f, "KMOD_ALT", "alt")
  push_vals(f, "KMOD_GUI", "gui")
end
