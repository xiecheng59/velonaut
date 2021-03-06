#ifndef NETWORKSERVER_H
#define NETWORKSERVER_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <pthread.h>
#include <time.h>
#include "common.h"
#include "RakNet/RakNetTypes.h"
#include "RakNet/MessageIdentifiers.h"
#include "RakNet/RakPeerInterface.h"
#include "RakNet/StringCompressor.h"
#include "RakNet/BitStream.h"


enum GameMessages
{
    GAME_MESSAGE = ID_USER_PACKET_ENUM+1,
    SERVER_FOUND = ID_USER_PACKET_ENUM+2,
    SERVER_RESPONSE = ID_USER_PACKET_ENUM+3
};

class timer {
    private:
        unsigned long begTime;
    public:
        void restart() {
            begTime = clock();
        }

        double elapsedTime() {
            return ( clock() - begTime) / static_cast<double>(CLOCKS_PER_SEC);
        }

        bool isTimeout(double seconds) {
            return elapsedTime() >= seconds;
        }
};


class NetworkServer
{

    struct Player {
        RakNet::RakNetGUID guid;
        bool isReady;
        bool isServer;
        bool isWinner;
        std::string name;
        std::string ipAdress;

        Player():isReady(false), isServer(false){}

    };

    struct AbstractServerState {
        virtual void update(NetworkServer* server ,timer&) = 0;
    };

    struct WaitingForPlayersState : public AbstractServerState {
        virtual void update(NetworkServer* server ,timer&);
    };

    struct WaitingForPlayerReadyState : public AbstractServerState {
        virtual void update(NetworkServer* server ,timer&);
    };

    struct CountDownState : public AbstractServerState {
        virtual void update(NetworkServer* server ,timer&);
    };

public:
    void start( int port);
    void shutdown();
    void run();
    void print();
    void setMaxIncomingConnections( int numCon );

    static NetworkServer* GetInstance();

private:
    SINGLETON(NetworkServer)
    void onClientConnect(RakNet::Packet* packet);
    void onClientDisconnect(RakNet::Packet* packet);
    void onClientAlreadyConnected(RakNet::Packet* packet);
    void onClientConnectionLost(RakNet::Packet* packet);
    void onGameMessageReceived(RakNet::Packet* packet);
    void sendToAllExcept(RakNet::BitStream* stream, RakNet::RakNetGUID except, PacketReliability reliability);
    void sendToAll(PacketReliability);
    void sendToOne(RakNet::RakNetGUID &guid, PacketReliability reliability);
    void pollPackets();
    void setServerState( AbstractServerState* state );
    void reset();

    void writeMessage( GameMessages msgType, RakNet::RakString msg );
    void writeString(RakNet::RakString str );
    void readString(RakNet::BitStream *bsIn,RakNet::RakString& str, bool ignoreMsgType = true );

    RakNet::RakString createPlayerListEvent();
    RakNet::RakString createWelcomeEvent(RakNet::RakNetGUID);
    RakNet::RakString createGameInitEvent();
    RakNet::RakString createCountDownEvent( int count);
    RakNet::RakString createGameStartEvent();
    RakNet::RakString createGameOverEvent(RakNet::RakNetGUID guid);

    static void rpcSetPlayerName(RakNet::BitStream* s, RakNet::Packet* p);
    static void rpcStartGame(RakNet::BitStream* s, RakNet::Packet* p);
    static void rpcSetPlayerReady(RakNet::BitStream* s, RakNet::Packet* p);
    static void rpcSetNumPlayers(RakNet::BitStream* s, RakNet::Packet* p);
    static void rpcSetGameOver(RakNet::BitStream* s, RakNet::Packet* p);

private:
    std::vector<RakNet::RakNetGUID> clients_;
    pthread_t thread_;
    RakNet::RakPeerInterface *server_;
    RakNet::BitStream bitSteamOut_;
    RakNet::RPC4 rpc_;
    std::map<RakNet::RakNetGUID, Player> players_;
    AbstractServerState* currentState_;
    RakNet::StringCompressor compressor_;
    int port_;
    timer countdown_;
    bool isRunning_;
    int numPlayers_;
};



#endif // NETWORKSERVER_H
