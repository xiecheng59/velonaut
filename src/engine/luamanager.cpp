#include <string>
#include <iostream>
#include <vector>
#include <stdarg.h>
#include <cmath>
#include <exception>
#include <assert.h>
#include "luamanager.h"
#include "app.h"
#include "graphics.h"
#include "ui.h"
#include "network.h"
#include "physics.h"

using namespace std;

LuaManager::LuaManager() {}

void LuaManager::addFunction(string func) const {

    size_t pos = func.find(".");
    if (pos != std::string::npos) {
        std::string table = func.substr (0, pos);
        std::string fname = func.substr (pos+1);

        lua_getglobal(state_, table.c_str());
        lua_getfield(state_, -1, fname.c_str());
        lua_remove(state_, -2);

    } else {
        lua_getglobal(state_, func.c_str() );
    }
}

void LuaManager::pCall(int nargs, int nresults) const {

    if (lua_pcall(state_, nargs, nresults, 0) != 0) {
        std::string str = lua_tostring(state_, lua_gettop(state_));
        lua_pop(state_, 1);
        //std::cout << str << std::endl;
        assert(false);
    }
}

void LuaManager::call(int nargs, int nres) const
{
    lua_call(state_, nargs, nres);
}

void LuaManager::addParam(int value) const {
    lua_pushnumber(state_, value);
}

void LuaManager::extractParam(int *value) const {
    *value = luaL_checknumber(state_, 1);
    lua_remove(state_, 1);
}

void LuaManager::addParam(double value) const {
    lua_pushnumber(state_, value);
}

void LuaManager::extractParam(double *value)const {
    *value = luaL_checknumber(state_, 1);
    lua_remove(state_, 1);
}

void LuaManager::addParam(bool value) const {
    lua_pushboolean(state_, value);
}

void LuaManager::addParam(string str) const {
    lua_pushstring(state_, str.c_str());
}

void LuaManager::addParam(const btVector3 &v) const
{
    lua_Number array[3] = {v.x(), v.y(), v.z()};
    addParam(array, 3);
}

void LuaManager::addParam(const btQuaternion &q) const
{
    lua_Number array[4] = {q.x(), q.y(), q.z(), q.w()};
    addParam(array, 4);
}

void LuaManager::addParam(const Ogre::Vector3& v) const
{
    lua_Number array[3] = {v.x, v.y, v.z};
    addParam(array, 3);
}

void LuaManager::addParam(const Ogre::Quaternion& q) const
{
    lua_Number array[4] = {q.x, q.y, q.z, q.w};
    addParam(array, 4);
}

void LuaManager::extractParam(string *str) const {
    *str = luaL_checkstring(state_, 1);
    lua_remove(state_, 1);
}

void LuaManager::extractParam(btVector3 *v) const {
    lua_Number numbers[3];
    extractParam(numbers, 3);
    v->setX(numbers[0]);
    v->setY(numbers[1]);
    v->setZ(numbers[2]);
}

void LuaManager::extractParam(btQuaternion *q) const {
    lua_Number numbers[4];
    extractParam(numbers, 4);
    q->setW(numbers[3]);
    q->setX(numbers[0]);
    q->setY(numbers[1]);
    q->setZ(numbers[2]);
}

void LuaManager::extractParam(Ogre::Vector3 *v) const {
    lua_Number numbers[3];
    extractParam(numbers, 3);
    v->x = numbers[0];
    v->y = numbers[1];
    v->z = numbers[2];
}

void LuaManager::extractParam(Ogre::Quaternion *q) const {
    lua_Number numbers[4];
    extractParam(numbers, 4);
    q->w = numbers[3];
    q->x = numbers[0];
    q->y = numbers[1];
    q->z = numbers[2];
}

void LuaManager::addParam(void *p) const
{
    lua_pushlightuserdata(state_, p);
}

void LuaManager::extractParam(bool *b) const
{
    luaL_checktype(state_, 1, LUA_TBOOLEAN);
    *b = lua_toboolean(state_, 1);
    lua_remove(state_, 1);
}

void LuaManager::addParam(lua_Number *array, int len) const
{
    lua_getglobal(state_, "Vector");
    lua_getfield(state_, -1, "__call");
    lua_pushvalue(state_, -2);
    lua_remove(state_, -3);

    for (int i = 0; i < len; i++) {
        lua_pushnumber(state_, array[i]);
    }
    lua_call(state_, len + 1, 1);
}

void LuaManager::extractParam(Ogre::ColourValue *c) const
{
    lua_Number numbers[4] = {0, 0, 0, 1};
    extractParam(numbers, 4);
    c->r = numbers[0];
    c->g = numbers[1];
    c->b = numbers[2];
    c->a = numbers[3];
}

void LuaManager::dumpStack() const {

    int i;
    int top = lua_gettop(state_);
    std::cout << "==== STACKDUMP len:" << top << " ===" << std::endl;
    for (i = top; i >= 1; --i) {
        int t = lua_type(state_,i);
        switch(t) {
        case LUA_TSTRING:
        {
            std::cout << lua_tostring(state_, i) << std::endl;
            break;
        }
        case LUA_TBOOLEAN:
        {
            std::cout << (lua_toboolean(state_,i) ? "true" : "false") << std::endl;
            break;
        }
        case LUA_TNUMBER:
        {
            std::cout << lua_tonumber(state_, i) << std::endl;
            break;
        }
        default:
        {
            std::cout << lua_typename(state_,t) << std::endl;
        }
        }
    }
    std::cout << "========================" << std::endl;
}


void LuaManager::registerFunction(const char *name, lua_CFunction func) const {
    lua_register(state_, name, func);
}

lua_State *LuaManager::state() const
{
    return state_;
}

LuaManager *LuaManager::GetInstance()
{
    return App::GetApp()->GetLuaManager();
}

void LuaManager::init()
{
    state_ = luaL_newstate();
    assert(state_);
    luaL_openlibs(state_);

    Graphics::GetInstance()->initLua();
    Physics::GetInstance()->initLua();
    Network::GetInstance()->initLua();
    Ui::GetInstance()->initLua();
}

void LuaManager::start()
{
    doFile("./data/scripts/engine/app.lua");
    AddDictionary();
}

void LuaManager::update(float dt)
{
    addFunction("App.update");
    addParam((double)dt);
    pCall(1);
}

void LuaManager::shutdown()
{
    lua_close(state_);
}

void LuaManager::addlib(luaL_Reg* reg)
{
    luaL_newlib(state_, reg);
}

void LuaManager::requiref(string name, lua_CFunction func)
{
    luaL_requiref(state_, name.c_str(), func, false);
}

void LuaManager::doFile(string file) const {
    int error = luaL_dofile(state_, file.c_str());
    if (error)
    {
        std::string str = lua_tostring(state_, lua_gettop(state_));
        lua_pop(state_, 1);
        //std::cout << str << std::endl;
        assert(false);
    }
}

void LuaManager::extractParam(void **p) const
{
    luaL_checktype(state_, 1, LUA_TLIGHTUSERDATA);
    *p = lua_touserdata(state_, 1); 
    lua_remove(state_, 1);
}

void LuaManager::addParamReg(const void *p) const
{
    lua_rawgetp(state_, LUA_REGISTRYINDEX, p);
}

void LuaManager::setReg(const void *p) const
{
    lua_pushvalue(state_, -1);
    lua_rawsetp(state_, LUA_REGISTRYINDEX, p);
}

void LuaManager::extractParam(lua_Number *array, int len) const
{
    luaL_checktype(state_, 1, LUA_TTABLE);
    int tablelen = luaL_len(state_, 1);
    for (int i = 1; i <= std::min(tablelen, len); i++) {
        lua_pushnumber(state_, i);
        lua_gettable(state_, 1);
        lua_Number num = luaL_checknumber(state_, -1);
        array[i-1] = num;
        lua_pop(state_, 1);
    }
    lua_remove(state_, 1);
}

void LuaManager::onMouseDown( SDL_Event e )
{
    addFunction("Input.onMouseDown");

    int modifierState = 0;
    addParam( Input::GetInstance()->dictionary_[e.button.button] );
    addParam(modifierState);

    pCall(2);
}

void LuaManager::onMouseUp( SDL_Event e)
{
    addFunction("Input.onMouseUp");

    int modifierState = 0;
    addParam( Input::GetInstance()->dictionary_[e.button.button] );
    addParam(modifierState);

    pCall(2);
}

void LuaManager::onMouseMove( SDL_Event e )
{

    addFunction("Input.onMouseMove");

    int modifierState = 0;
    addParam( e.motion.x );
    addParam( e.motion.y );
    addParam(modifierState);

    pCall(3);
}

void LuaManager::onKeyDown( SDL_Event e )
{
    addFunction("Input.onKeyDown");
    addParam( Input::GetInstance()->dictionary_[e.key.keysym.sym] );
    pCall(1);
}

void LuaManager::onKeyUp( SDL_Event e )
{
    addFunction("Input.onKeyUp");
    addParam( Input::GetInstance()->dictionary_[e.key.keysym.sym] );
    pCall(1);
}

void LuaManager::AddDictionary()
{
    std::map<uint, std::string>::iterator it;
    int count = 0;

    addFunction("Input.createDictionary");
    for (it = Input::GetInstance()->dictionary_.begin(); it !=Input::GetInstance()->dictionary_.end(); it++)
    {
        addParam( it->second );
        count++;
    }
    pCall(count);
}
