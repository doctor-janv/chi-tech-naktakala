#include "chi_lua.h"

#include "../chi_meshhandler.h"
#include "chi_runtime.h"

#include "chi_log.h"

#include <iostream>
#include "meshhandler_lua.h"
#include "console/chi_console.h"

RegisterLuaFunctionAsIs(chiMeshHandlerCreate);
RegisterLuaFunctionAsIs(chiMeshHandlerSetCurrent);
RegisterLuaFunctionAsIs(chiMeshHandlerExportMeshToObj);
RegisterLuaFunctionAsIs(chiMeshHandlerExportMeshToVTK);
RegisterLuaFunctionAsIs(chiMeshHandlerExportMeshToExodus);

//#############################################################################
/** Creates a mesh handler and sets it as "current".

\return Handle int Handle to the created mesh handler.
\ingroup LuaMeshHandler
\author Jan*/
int chiMeshHandlerCreate(lua_State *L)
{
  int index = (int) chi_mesh::PushNewHandlerAndGetIndex();
  lua_pushnumber(L,index);

  Chi::log.LogAllVerbose2()
  << "chiMeshHandlerCreate: Mesh Handler " << index << " created\n";

  return 1;
}


//#############################################################################
/** Sets the given mesh handler as "current".

\param HandlerHandler int Handle to the mesh handler previously created
       with a call to chiMeshHandlerCreate.

\ingroup LuaMeshHandler
\author Jan*/
int chiMeshHandlerSetCurrent(lua_State *L)
{
  int num_args = lua_gettop(L);
  if (num_args != 1)
    LuaPostArgAmountError("chiMeshHandlerSetCurrent",1,num_args);

  int handle = lua_tonumber(L,1);

  if ((handle < 0) or (handle >= Chi::meshhandler_stack.size()))
  {
    Chi::log.LogAllError()
      << "Invalid handle to mesh handler specified "
      << "in call to chiMeshHandlerSetCurrent";
    Chi::Exit(EXIT_FAILURE);
  }

  Chi::current_mesh_handler = handle;

  Chi::log.LogAllVerbose2()
    << "chiMeshHandlerSetCurrent: set to " << handle;

  return 0;
}