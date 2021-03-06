#ifndef UI_H
#define UI_H

#include <OGRE/Ogre.h>
#include <Rocket/Core.h>
#include <lua/lua.hpp>
#include "common.h"
#include "inputlistener.h"
#include "luamanager.h"

class Ui : public Ogre::RenderQueueListener, InputListener
{
public:
    void init();
    void initLua();
    void shutdown();

    virtual void renderQueueStarted(Ogre::uint8 queueGroupId, const Ogre::String& invocation, bool& skipThisInvocation);

    virtual void onMouseDown(SDL_Event e );
    virtual void onMouseUp(SDL_Event e );
    virtual void onMouseMove(SDL_Event e );

    virtual void onKeyDown( SDL_Event e );
    virtual void onKeyUp( SDL_Event e );

    static Ui* GetInstance();



private:
    SINGLETON(Ui)
    friend class App;
    void configureRenderSystem();
    void buildKeyMaps();
    static int lLoadDocument(lua_State* state);
    static int lUnloadDocument(lua_State* state);
    static int lHideDocument(lua_State* state);
    static int lShowDocument(lua_State* state);
    static int lLoadMouseCursor(lua_State* state);
    static int lLoadFont(lua_State* state);
    static int lSetText(lua_State* state);
    static int lSetAttribute(lua_State* state);
    static int lGetAttribute(lua_State* state);
    static int lAddClass(lua_State* state);
    static int lHasClass(lua_State* state);
    static int lRemoveClass(lua_State* state);
    static int lAddEventListener(lua_State* state);

private:

    typedef std::map< SDL_Keycode, Rocket::Core::Input::KeyIdentifier > KeyIdentifierMap;

    Rocket::Core::Context* context_;
    Ogre::Matrix4 projection_matrix;
    KeyIdentifierMap key_identifiers;
    std::vector<Rocket::Core::ElementDocument*> docs_;
    Rocket::Core::ElementDocument *doc_;


class RocketEventListener : public Rocket::Core::EventListener
{
public:
    int r;
    RocketEventListener(int r):r(r) {}


protected:
    virtual void ProcessEvent(Rocket::Core::Event& event) {
        lua_rawgeti(LuaManager::GetInstance()->state(), LUA_REGISTRYINDEX, r);
        LuaManager::GetInstance()->pCall();
    }
};

};

#endif // UI_H
