#include <Rocket/Core.h>
#include <Rocket/Controls.h>
#include <Rocket/Debugger.h>

#include "ui.h"
#include "uirenderinterface.h"
#include "uisysteminterface.h"
#include "app.h"

using namespace std;

Ui::Ui()
{
    buildKeyMaps();
}

void Ui::initLua() {

    LuaManager::GetInstance()->requiref("engine.gui.c",[](lua_State* state) {
        luaL_Reg reg[] = {
            {"loadDocument", Ui::lLoadDocument},
            {"unloadDocument", Ui::lUnloadDocument},
            {"hideDocument", Ui::lHideDocument},
            {"showDocument", Ui::lShowDocument},
            {"loadCursor", Ui::lLoadMouseCursor},
            {"loadFont", Ui::lLoadFont},
            {"setText", Ui::lSetText},
            {"setAttribute", Ui::lSetAttribute},
            {"getAttribute", Ui::lGetAttribute},
            {"addClass", Ui::lAddClass},
            {"hasClass", Ui::lHasClass},
            {"removeClass", Ui::lRemoveClass},
            {"addEventListener", Ui::lAddEventListener},
            {NULL, NULL}
        };
        LuaManager::GetInstance()->addlib(reg);
        return 1;
    } );
}

void Ui::init() {

   // Set up the projection and view matrices.
    float z_near = -1;
    float z_far = 1;

    projection_matrix = Ogre::Matrix4::ZERO;

    // Set up matrices.
    projection_matrix[0][0] = 2.0f / 800;
    projection_matrix[0][3]= -1.0000000f;
    projection_matrix[1][1]= -2.0f / 600;
    projection_matrix[1][3]= 1.0000000f;
    projection_matrix[2][2]= -2.0f / (z_far - z_near);
    projection_matrix[3][3]= 1.0000000f;

    Ogre::ResourceGroupManager::getSingleton().createResourceGroup("Rocket");
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation("./", "FileSystem", "Rocket");

    Rocket::Core::SetRenderInterface(new UiRenderInterface(800, 600));
    Rocket::Core::SetSystemInterface(new UiSystemInterface());

    Rocket::Core::Initialise();
    Rocket::Controls::Initialise();

    context_ = Rocket::Core::CreateContext("main", Rocket::Core::Vector2i(800, 600));
    Rocket::Debugger::Initialise(context_);

    context_->ShowMouseCursor(true);
}

void Ui::shutdown() {
    context_->UnloadAllDocuments();
}

int Ui::lLoadMouseCursor(lua_State *state)
{
    // Load the mouse cursor and release the caller's reference.
    Rocket::Core::ElementDocument* cursor = Ui::GetInstance()->context_->LoadMouseCursor("data/ui/cursor.rml");
    if (cursor)
        cursor->RemoveReference();
}

int Ui::lAddEventListener(lua_State *state) {

    string id;
    string event;

    LuaManager::GetInstance()->extractParam(&id);
    LuaManager::GetInstance()->extractParam(&event);

    int r = luaL_ref(state, LUA_REGISTRYINDEX);
    Ui::GetInstance()->doc_->GetElementById(id.c_str())->AddEventListener(event.c_str(), new RocketEventListener(r), true );
}

int Ui::lAddClass(lua_State *state) {
    string id;
    string text;

    LuaManager::GetInstance()->extractParam(&id);
    LuaManager::GetInstance()->extractParam(&text);

    Ui::GetInstance()->doc_->GetElementById(id.c_str())->SetClass(text.c_str(), true);
}

int Ui::lRemoveClass(lua_State *state) {
    string id;
    string text;

    LuaManager::GetInstance()->extractParam(&id);
    LuaManager::GetInstance()->extractParam(&text);
    Ui::GetInstance()->doc_->GetElementById(id.c_str())->SetClass(text.c_str(), false);
}

int Ui::lHasClass(lua_State *state) {
    string id;
    string text;

    LuaManager::GetInstance()->extractParam(&id);
    LuaManager::GetInstance()->extractParam(&text);

    bool hasClass = Ui::GetInstance()->doc_->GetElementById(id.c_str())->IsClassSet(text.c_str());
    LuaManager::GetInstance()->addParam(hasClass);

    return 1;
}

int Ui::lSetText(lua_State *state) {
    string id;
    string text;

    LuaManager::GetInstance()->extractParam(&id);
    LuaManager::GetInstance()->extractParam(&text);

    Ui::GetInstance()->doc_->GetElementById(id.c_str())->SetInnerRML(text.c_str());
}

int Ui::lSetAttribute(lua_State *state) {
    string id;
    string attr;
    string val;

    LuaManager::GetInstance()->extractParam(&id);
    LuaManager::GetInstance()->extractParam(&attr);
    LuaManager::GetInstance()->extractParam(&val);

    Ui::GetInstance()->doc_->GetElementById(id.c_str())->SetAttribute(attr.c_str(), val.c_str());
}

int Ui::lGetAttribute(lua_State *state) {
    string id;
    string attr;

    LuaManager::GetInstance()->extractParam(&id);
    LuaManager::GetInstance()->extractParam(&attr);

    Rocket::Core::String s = Ui::GetInstance()->doc_->GetElementById(id.c_str())->GetAttribute<Rocket::Core::String>(attr.c_str(), "none" );
    LuaManager::GetInstance()->addParam(string(s.CString()));

    return 1;
}

int Ui::lLoadDocument(lua_State *state)
{
    string docString;
    LuaManager::GetInstance()->extractParam(&docString);

    Ui* ui = Ui::GetInstance();    
    Rocket::Core::ElementDocument* doc;

    doc = ui->context_->LoadDocument( docString.c_str() );
    if (doc)
    {
        doc->RemoveReference();
    }
    ui->docs_.push_back(doc);
    int i = ui->docs_.size()-1;
    LuaManager::GetInstance()->addParam(i);

    return 1;
}

int Ui::lUnloadDocument(lua_State *state) {
    Ui* ui = Ui::GetInstance();
    ui->context_->UnloadDocument( ui->doc_ );
    return 0;
}

int Ui::lHideDocument(lua_State *state) {
    Ui* ui = Ui::GetInstance();
    int docIndex;
    LuaManager::GetInstance()->extractParam(&docIndex);
    ui->docs_[docIndex]->Hide();
    return 0;
}

int Ui::lShowDocument(lua_State *state) {
    Ui* ui = Ui::GetInstance();
    int docIndex;
    LuaManager::GetInstance()->extractParam(&docIndex);
    ui->doc_ = ui->docs_[docIndex];
    ui->doc_->Show();
    return 0;
}

// TODO: Add RemoveEventListener function

Ui *Ui::GetInstance()
{
    return App::GetApp()->GetUi();
}

int Ui::lLoadFont(lua_State *state)
{
    string s;
    LuaManager::GetInstance()->extractParam(&s);
    Rocket::Core::FontDatabase::LoadFontFace(s.c_str());
}

void Ui::onMouseDown(SDL_Event e) {
    context_->ProcessMouseButtonDown(0, 0);
}

void Ui::onMouseMove(SDL_Event e) {
    context_->ProcessMouseMove(e.motion.x, e.motion.y, 0);
}

void Ui::onMouseUp(SDL_Event e ) {
    context_->ProcessMouseButtonUp(0, 0);
}

void Ui::onKeyDown( SDL_Event e )
{
    if (key_identifiers[e.key.keysym.sym] != 0) {
        context_->ProcessKeyDown(key_identifiers[e.key.keysym.sym], 0);
        if (key_identifiers[e.key.keysym.sym] != Rocket::Core::Input::KI_BACK &&
            key_identifiers[e.key.keysym.sym] != Rocket::Core::Input::KI_UP &&
            key_identifiers[e.key.keysym.sym] != Rocket::Core::Input::KI_DOWN &&
            key_identifiers[e.key.keysym.sym] != Rocket::Core::Input::KI_RIGHT &&
            key_identifiers[e.key.keysym.sym] != Rocket::Core::Input::KI_LEFT
           )
            context_->ProcessTextInput(e.key.keysym.sym);
    }
}

void Ui::onKeyUp( SDL_Event e )
{
    if (key_identifiers[e.key.keysym.sym] != 0) {
        context_->ProcessKeyUp(key_identifiers[e.key.keysym.sym], 0);
    }
}

// Called from Ogre before a queue group is rendered.
void Ui::renderQueueStarted(Ogre::uint8 queueGroupId, const Ogre::String& invocation, bool& ROCKET_UNUSED(skipThisInvocation))
{
    if (queueGroupId == Ogre::RENDER_QUEUE_OVERLAY && Ogre::Root::getSingleton().getRenderSystem()->_getViewport()->getOverlaysEnabled())
    {
        context_->Update();
        configureRenderSystem();
        context_->Render();
    }
}

void Ui::configureRenderSystem()
{
    Ogre::RenderSystem* render_system = Ogre::Root::getSingleton().getRenderSystem();

    render_system->_setProjectionMatrix(projection_matrix);
    render_system->_setViewMatrix(Ogre::Matrix4::IDENTITY);

    // Disable lighting, as all of Rocket's geometry is unlit.
    render_system->setLightingEnabled(false);
    // Disable depth-buffering; all of the geometry is already depth-sorted.
    render_system->_setDepthBufferParams(false, false);
    // Rocket generates anti-clockwise geometry, so enable clockwise-culling.
    render_system->_setCullingMode(Ogre::CULL_CLOCKWISE);
    // Disable fogging.
    render_system->_setFog(Ogre::FOG_NONE);
    // Enable writing to all four channels.
    render_system->_setColourBufferWriteEnabled(true, true, true, true);
    // Unbind any vertex or fragment programs bound previously by the application.
    render_system->unbindGpuProgram(Ogre::GPT_FRAGMENT_PROGRAM);
    render_system->unbindGpuProgram(Ogre::GPT_VERTEX_PROGRAM);

    // Set texture settings to clamp along both axes.
    Ogre::TextureUnitState::UVWAddressingMode addressing_mode;
    addressing_mode.u = Ogre::TextureUnitState::TAM_CLAMP;
    addressing_mode.v = Ogre::TextureUnitState::TAM_CLAMP;
    addressing_mode.w = Ogre::TextureUnitState::TAM_CLAMP;
    render_system->_setTextureAddressingMode(0, addressing_mode);

    // Set the texture coordinates for unit 0 to be read from unit 0.
    render_system->_setTextureCoordSet(0, 0);
    // Disable texture coordinate calculation.
    render_system->_setTextureCoordCalculation(0, Ogre::TEXCALC_NONE);
    // Enable linear filtering; images should be rendering 1 texel == 1 pixel, so point filtering could be used
    // except in the case of scaling tiled decorators.
    render_system->_setTextureUnitFiltering(0, Ogre::FO_LINEAR, Ogre::FO_LINEAR, Ogre::FO_POINT);
    // Disable texture coordinate transforms.
    render_system->_setTextureMatrix(0, Ogre::Matrix4::IDENTITY);
    // Reject pixels with an alpha of 0.
    render_system->_setAlphaRejectSettings(Ogre::CMPF_GREATER, 0, false);
    // Disable all texture units but the first.
    render_system->_disableTextureUnitsFrom(1);

    // Enable simple alpha blending.
    render_system->_setSceneBlending(Ogre::SBF_SOURCE_ALPHA, Ogre::SBF_ONE_MINUS_SOURCE_ALPHA);

    // Disable depth bias.
    render_system->_setDepthBias(0, 0);
}
void Ui::buildKeyMaps()
{
    key_identifiers[SDLK_UNKNOWN] = Rocket::Core::Input::KI_UNKNOWN;
    key_identifiers[SDLK_ESCAPE] = Rocket::Core::Input::KI_ESCAPE;
    key_identifiers[SDLK_1] = Rocket::Core::Input::KI_1;
    key_identifiers[SDLK_2] = Rocket::Core::Input::KI_2;
    key_identifiers[SDLK_3] = Rocket::Core::Input::KI_3;
    key_identifiers[SDLK_4] = Rocket::Core::Input::KI_4;
    key_identifiers[SDLK_5] = Rocket::Core::Input::KI_5;
    key_identifiers[SDLK_6] = Rocket::Core::Input::KI_6;
    key_identifiers[SDLK_7] = Rocket::Core::Input::KI_7;
    key_identifiers[SDLK_8] = Rocket::Core::Input::KI_8;
    key_identifiers[SDLK_9] = Rocket::Core::Input::KI_9;
    key_identifiers[SDLK_0] = Rocket::Core::Input::KI_0;
    key_identifiers[SDLK_MINUS] = Rocket::Core::Input::KI_OEM_MINUS;
    key_identifiers[SDLK_EQUALS] = Rocket::Core::Input::KI_OEM_PLUS;
    key_identifiers[SDLK_BACKSPACE] = Rocket::Core::Input::KI_BACK;
    key_identifiers[SDLK_TAB] = Rocket::Core::Input::KI_TAB;
    key_identifiers[SDLK_q] = Rocket::Core::Input::KI_Q;
    key_identifiers[SDLK_w] = Rocket::Core::Input::KI_W;
    key_identifiers[SDLK_e] = Rocket::Core::Input::KI_E;
    key_identifiers[SDLK_r] = Rocket::Core::Input::KI_R;
    key_identifiers[SDLK_t] = Rocket::Core::Input::KI_T;
    key_identifiers[SDLK_y] = Rocket::Core::Input::KI_Y;
    key_identifiers[SDLK_u] = Rocket::Core::Input::KI_U;
    key_identifiers[SDLK_i] = Rocket::Core::Input::KI_I;
    key_identifiers[SDLK_o] = Rocket::Core::Input::KI_O;
    key_identifiers[SDLK_p] = Rocket::Core::Input::KI_P;
    key_identifiers[SDLK_LEFTBRACKET] = Rocket::Core::Input::KI_OEM_4;
    key_identifiers[SDLK_RIGHTBRACKET] = Rocket::Core::Input::KI_OEM_6;
    key_identifiers[SDLK_RETURN] = Rocket::Core::Input::KI_RETURN;
   // key_identifiers[SDLK_] = Rocket::Core::Input::KI_LCONTROL;
    key_identifiers[SDLK_a] = Rocket::Core::Input::KI_A;
    key_identifiers[SDLK_s] = Rocket::Core::Input::KI_S;
    key_identifiers[SDLK_d] = Rocket::Core::Input::KI_D;
    key_identifiers[SDLK_f] = Rocket::Core::Input::KI_F;
    key_identifiers[SDLK_g] = Rocket::Core::Input::KI_G;
    key_identifiers[SDLK_h] = Rocket::Core::Input::KI_H;
    key_identifiers[SDLK_j] = Rocket::Core::Input::KI_J;
    key_identifiers[SDLK_k] = Rocket::Core::Input::KI_K;
    key_identifiers[SDLK_l] = Rocket::Core::Input::KI_L;
    key_identifiers[SDLK_SEMICOLON] = Rocket::Core::Input::KI_OEM_1;
    key_identifiers[SDLK_QUOTE] = Rocket::Core::Input::KI_OEM_7;
    //key_identifiers[OIS::KC_GRAVE] = Rocket::Core::Input::KI_OEM_3;
    //key_identifiers[SDLK_SH] = Rocket::Core::Input::KI_LSHIFT;
    key_identifiers[SDLK_BACKSLASH] = Rocket::Core::Input::KI_OEM_5;
    key_identifiers[SDLK_z] = Rocket::Core::Input::KI_Z;
    key_identifiers[SDLK_x] = Rocket::Core::Input::KI_X;
    key_identifiers[SDLK_c] = Rocket::Core::Input::KI_C;
    key_identifiers[SDLK_v] = Rocket::Core::Input::KI_V;
    key_identifiers[SDLK_b] = Rocket::Core::Input::KI_B;
    key_identifiers[SDLK_n] = Rocket::Core::Input::KI_N;
    key_identifiers[SDLK_m] = Rocket::Core::Input::KI_M;
    key_identifiers[SDLK_COMMA] = Rocket::Core::Input::KI_OEM_COMMA;
    key_identifiers[SDLK_PERIOD] = Rocket::Core::Input::KI_OEM_PERIOD;
    key_identifiers[SDLK_SLASH] = Rocket::Core::Input::KI_OEM_2;
    //key_identifiers[SDLK_R] = Rocket::Core::Input::KI_RSHIFT;
    key_identifiers[SDLK_ASTERISK] = Rocket::Core::Input::KI_MULTIPLY;
    //key_identifiers[OIS::KC_LMENU] = Rocket::Core::Input::KI_LMENU;
    key_identifiers[SDLK_SPACE] = Rocket::Core::Input::KI_SPACE;
    //key_identifiers[SDLK_CA] = Rocket::Core::Input::KI_CAPITAL;
    key_identifiers[SDLK_F1] = Rocket::Core::Input::KI_F1;
    key_identifiers[SDLK_F2] = Rocket::Core::Input::KI_F2;
    key_identifiers[SDLK_F3] = Rocket::Core::Input::KI_F3;
    key_identifiers[SDLK_F4] = Rocket::Core::Input::KI_F4;
    key_identifiers[SDLK_F5] = Rocket::Core::Input::KI_F5;
    key_identifiers[SDLK_F6] = Rocket::Core::Input::KI_F6;
    key_identifiers[SDLK_F7] = Rocket::Core::Input::KI_F7;
    key_identifiers[SDLK_F8] = Rocket::Core::Input::KI_F8;
    key_identifiers[SDLK_F9] = Rocket::Core::Input::KI_F9;
    key_identifiers[SDLK_F10] = Rocket::Core::Input::KI_F10;
    key_identifiers[SDLK_NUMLOCKCLEAR] = Rocket::Core::Input::KI_NUMLOCK;
    key_identifiers[SDLK_SCROLLLOCK] = Rocket::Core::Input::KI_SCROLL;
    key_identifiers[SDLK_7] = Rocket::Core::Input::KI_7;
    key_identifiers[SDLK_8] = Rocket::Core::Input::KI_8;
    key_identifiers[SDLK_9] = Rocket::Core::Input::KI_9;
    //key_identifiers[SDLK_su] = Rocket::Core::Input::KI_SUBTRACT;
    key_identifiers[SDLK_4] = Rocket::Core::Input::KI_4;
    key_identifiers[SDLK_5] = Rocket::Core::Input::KI_5;
    key_identifiers[SDLK_6] = Rocket::Core::Input::KI_6;
    //key_identifiers[OIS::KC_ADD] = Rocket::Core::Input::KI_ADD;
    key_identifiers[SDLK_1] = Rocket::Core::Input::KI_1;
    key_identifiers[SDLK_2] = Rocket::Core::Input::KI_2;
    key_identifiers[SDLK_3] = Rocket::Core::Input::KI_3;
    key_identifiers[SDLK_0] = Rocket::Core::Input::KI_0;
    //key_identifiers[OIS::KC_DECIMAL] = Rocket::Core::Input::KI_DECIMAL;
    //key_identifiers[OIS::KC_OEM_102] = Rocket::Core::Input::KI_OEM_102;
    key_identifiers[SDLK_F11] = Rocket::Core::Input::KI_F11;
    key_identifiers[SDLK_F12] = Rocket::Core::Input::KI_F12;
    key_identifiers[SDLK_F13] = Rocket::Core::Input::KI_F13;
    key_identifiers[SDLK_F14] = Rocket::Core::Input::KI_F14;
    key_identifiers[SDLK_F15] = Rocket::Core::Input::KI_F15;

    key_identifiers[SDLK_UP] = Rocket::Core::Input::KI_UP;
    key_identifiers[SDLK_LEFT] = Rocket::Core::Input::KI_LEFT;
    key_identifiers[SDLK_RIGHT] = Rocket::Core::Input::KI_RIGHT;
    key_identifiers[SDLK_DOWN] = Rocket::Core::Input::KI_DOWN;

    /*
    //key_identifiers[OIS::KC_KANA] = Rocket::Core::Input::KI_KANA;
    key_identifiers[OIS::KC_ABNT_C1] = Rocket::Core::Input::KI_UNKNOWN;
    key_identifiers[OIS::KC_CONVERT] = Rocket::Core::Input::KI_CONVERT;
    key_identifiers[OIS::KC_NOCONVERT] = Rocket::Core::Input::KI_NONCONVERT;
    key_identifiers[OIS::KC_YEN] = Rocket::Core::Input::KI_UNKNOWN;
    key_identifiers[OIS::KC_ABNT_C2] = Rocket::Core::Input::KI_UNKNOWN;
    key_identifiers[OIS::KC_NUMPADEQUALS] = Rocket::Core::Input::KI_OEM_NEC_EQUAL;
    key_identifiers[OIS::KC_PREVTRACK] = Rocket::Core::Input::KI_MEDIA_PREV_TRACK;
    key_identifiers[OIS::KC_AT] = Rocket::Core::Input::KI_UNKNOWN;
    key_identifiers[OIS::KC_COLON] = Rocket::Core::Input::KI_OEM_1;
    key_identifiers[OIS::KC_UNDERLINE] = Rocket::Core::Input::KI_OEM_MINUS;
    key_identifiers[OIS::KC_KANJI] = Rocket::Core::Input::KI_KANJI;
    key_identifiers[OIS::KC_STOP] = Rocket::Core::Input::KI_UNKNOWN;
    key_identifiers[OIS::KC_AX] = Rocket::Core::Input::KI_OEM_AX;
    key_identifiers[OIS::KC_UNLABELED] = Rocket::Core::Input::KI_UNKNOWN;
    key_identifiers[OIS::KC_NEXTTRACK] = Rocket::Core::Input::KI_MEDIA_NEXT_TRACK;
    key_identifiers[OIS::KC_NUMPADENTER] = Rocket::Core::Input::KI_NUMPADENTER;
    key_identifiers[OIS::KC_RCONTROL] = Rocket::Core::Input::KI_RCONTROL;
    key_identifiers[OIS::KC_MUTE] = Rocket::Core::Input::KI_VOLUME_MUTE;
    key_identifiers[OIS::KC_CALCULATOR] = Rocket::Core::Input::KI_UNKNOWN;
    key_identifiers[OIS::KC_PLAYPAUSE] = Rocket::Core::Input::KI_MEDIA_PLAY_PAUSE;
    key_identifiers[OIS::KC_MEDIASTOP] = Rocket::Core::Input::KI_MEDIA_STOP;
    key_identifiers[OIS::KC_VOLUMEDOWN] = Rocket::Core::Input::KI_VOLUME_DOWN;
    key_identifiers[OIS::KC_VOLUMEUP] = Rocket::Core::Input::KI_VOLUME_UP;
    key_identifiers[OIS::KC_WEBHOME] = Rocket::Core::Input::KI_BROWSER_HOME;
    key_identifiers[OIS::KC_NUMPADCOMMA] = Rocket::Core::Input::KI_SEPARATOR;
    key_identifiers[OIS::KC_DIVIDE] = Rocket::Core::Input::KI_DIVIDE;
    key_identifiers[OIS::KC_SYSRQ] = Rocket::Core::Input::KI_SNAPSHOT;
    key_identifiers[OIS::KC_RMENU] = Rocket::Core::Input::KI_RMENU;
    key_identifiers[OIS::KC_PAUSE] = Rocket::Core::Input::KI_PAUSE;
    key_identifiers[OIS::KC_HOME] = Rocket::Core::Input::KI_HOME;
    key_identifiers[OIS::KC_UP] = Rocket::Core::Input::KI_UP;
    key_identifiers[OIS::KC_PGUP] = Rocket::Core::Input::KI_PRIOR;
    key_identifiers[OIS::KC_LEFT] = Rocket::Core::Input::KI_LEFT;
    key_identifiers[OIS::KC_RIGHT] = Rocket::Core::Input::KI_RIGHT;
    key_identifiers[OIS::KC_END] = Rocket::Core::Input::KI_END;
    key_identifiers[OIS::KC_DOWN] = Rocket::Core::Input::KI_DOWN;
    key_identifiers[OIS::KC_PGDOWN] = Rocket::Core::Input::KI_NEXT;
    key_identifiers[OIS::KC_INSERT] = Rocket::Core::Input::KI_INSERT;
    key_identifiers[OIS::KC_DELETE] = Rocket::Core::Input::KI_DELETE;
    */
}
