#ifndef LUAMANAGER_H
#define LUAMANAGER_H

#include <lua/lua.hpp>
#include <Ogre.h>
#include "common.h"

class LuaManager
{
public:
    void init();
    void start();
    void update(float dt);
    void shutdown();

    void doFile(std::string file) const;
    void addlib(luaL_Reg *reg);
    void requiref(std::string name, lua_CFunction func);
    void pCall(int nargs = 0, int nresults = 0) const;
    void registerFunction(const char* name, lua_CFunction func) const;
    
    void dumpStack() const;

    void addFunction(std::string str) const;

    void addParam(int value) const;
    void addParam(double value) const;
    void addParam(std::string str) const;
    void addParam(Ogre::Vector3 v) const;
    void addParam(Ogre::Matrix3 m) const;
    void addParam(void *) const;
    
    void extractParam(int *value) const;
    void extractParam(double *value) const;
    void extractParam(std::string *str) const;
    void extractParam(Ogre::Vector3 *v) const;
    void extractParam(Ogre::Matrix3 *m) const;
    void extractParam(Ogre::ColourValue *c) const;
    void extractParam(void**) const;

    lua_State *state() const;
    
    static LuaManager* GetInstance();
private:
    void pushMatrix(lua_Number*, int numElements)  const;
    void pushLuaMatrix()  const;
    void pushVector(lua_Number*, int numElements)  const;
    void removeVector(lua_Number*, int index)  const;
    void removeMatrix(lua_Number*, int index)  const;

    lua_State* state_;

    friend class App;
    SINGLETON(LuaManager)
    
};

#endif // LUAMANAGER_H
