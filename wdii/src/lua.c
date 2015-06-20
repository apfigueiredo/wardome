#include <lua.h>
#include <lauxlib.h>
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "interpreter.h"


#define SCRIPT_DIR "lua/"

extern int luaopen_wardome(lua_State* L); 

ACMD(do_lua) 
{
  lua_State *L;
  char lbuf[1024], scriptf[1024];

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char("Give me a script file to read...\r\n", ch);
    return;
  }

  L=luaL_newstate();
  luaopen_base(L);
  luaopen_wardome(L);
  snprintf(scriptf, 1023, "%s%s", SCRIPT_DIR, argument);
  if (luaL_loadfile(L, scriptf)==0) {
    lua_pcall(L,0,0,0);
  } else {
    snprintf(lbuf, 1023, "Unable to load <%s>\r\n", scriptf);
    send_to_char(lbuf, ch);
  }
  lua_close(L);
}
