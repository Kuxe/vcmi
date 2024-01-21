/*
 * CServerHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/CStopWatch.h"

#include "../lib/network/NetworkInterface.h"
#include "../lib/StartInfo.h"
#include "../lib/CondSh.h"

VCMI_LIB_NAMESPACE_BEGIN

class CConnection;
class PlayerColor;
struct StartInfo;
struct TurnTimerInfo;

class CMapInfo;
class CGameState;
struct ClientPlayer;
struct CPack;
struct CPackForLobby;
struct CPackForClient;

template<typename T> class CApplier;

VCMI_LIB_NAMESPACE_END

class CClient;
class CBaseForLobbyApply;
class GlobalLobbyClient;

class HighScoreCalculation;
class HighScoreParameter;

enum class ESelectionScreen : ui8;
enum class ELoadMode : ui8;

// TODO: Add mutex so we can't set CONNECTION_CANCELLED if client already connected, but thread not setup yet
enum class EClientState : ui8
{
	NONE = 0,
	CONNECTING, // Trying to connect to server
	CONNECTION_CANCELLED, // Connection cancelled by player, stop attempts to connect
	LOBBY, // Client is connected to lobby
	LOBBY_CAMPAIGN, // Client is on scenario bonus selection screen
	STARTING, // Gameplay interfaces being created, we pause netpacks retrieving
	GAMEPLAY, // In-game, used by some UI
	DISCONNECTING, // We disconnecting, drop all netpacks
	CONNECTION_FAILED // We could not connect to server
};

class IServerAPI
{
protected:
	virtual void sendLobbyPack(const CPackForLobby & pack) const = 0;

public:
	virtual ~IServerAPI() {}

	virtual void sendClientConnecting() const = 0;
	virtual void sendClientDisconnecting() = 0;
	virtual void setCampaignState(std::shared_ptr<CampaignState> newCampaign) = 0;
	virtual void setCampaignMap(CampaignScenarioID mapId) const = 0;
	virtual void setCampaignBonus(int bonusId) const = 0;
	virtual void setMapInfo(std::shared_ptr<CMapInfo> to, std::shared_ptr<CMapGenOptions> mapGenOpts = {}) const = 0;
	virtual void setPlayer(PlayerColor color) const = 0;
	virtual void setPlayerName(PlayerColor color, const std::string & name) const = 0;
	virtual void setPlayerOption(ui8 what, int32_t value, PlayerColor player) const = 0;
	virtual void setDifficulty(int to) const = 0;
	virtual void setTurnTimerInfo(const TurnTimerInfo &) const = 0;
	virtual void setSimturnsInfo(const SimturnsInfo &) const = 0;
	virtual void setExtraOptionsInfo(const ExtraOptionsInfo & info) const = 0;
	virtual void sendMessage(const std::string & txt) const = 0;
	virtual void sendGuiAction(ui8 action) const = 0; // TODO: possibly get rid of it?
	virtual void sendStartGame(bool allowOnlyAI = false) const = 0;
	virtual void sendRestartGame() const = 0;
};

/// structure to handle running server and connecting to it
class CServerHandler final : public IServerAPI, public LobbyInfo, public INetworkClientListener, public INetworkTimerListener, boost::noncopyable
{
	friend class ApplyOnLobbyHandlerNetPackVisitor;

	std::shared_ptr<INetworkConnection> networkConnection;
	std::unique_ptr<GlobalLobbyClient> lobbyClient;
	std::unique_ptr<CApplier<CBaseForLobbyApply>> applier;
	std::shared_ptr<CMapInfo> mapToStart;
	std::vector<std::string> myNames;
	std::shared_ptr<HighScoreCalculation> highScoreCalc;

	void threadRunNetwork();
	void threadRunServer(bool connectToLobby);

	/// temporary helper member that exists while game in lobby mode
	/// required to correctly deserialize gamestate using client-side game callback
	std::unique_ptr<CClient> nextClient;

	void onServerFinished();
	void sendLobbyPack(const CPackForLobby & pack) const override;

	void onPacketReceived(const std::shared_ptr<INetworkConnection> &, const std::vector<uint8_t> & message) override;
	void onConnectionFailed(const std::string & errorMessage) override;
	void onConnectionEstablished(const std::shared_ptr<INetworkConnection> &) override;
	void onDisconnected(const std::shared_ptr<INetworkConnection> &) override;
	void onTimer() override;

	void applyPackOnLobbyScreen(CPackForLobby & pack);

	std::string serverHostname;
	ui16 serverPort;

	bool isServerLocal() const;

public:
	std::unique_ptr<INetworkHandler> networkHandler;

	std::shared_ptr<CConnection> c;

	std::atomic<EClientState> state;
	////////////////////
	// FIXME: Bunch of crutches to glue it all together

	// For starting non-custom campaign and continue to next mission
	std::shared_ptr<CampaignState> campaignStateToSend;

	ESelectionScreen screenType; // To create lobby UI only after server is setup
	ELoadMode loadMode; // For saves filtering in SelectionTab
	////////////////////

	std::unique_ptr<CStopWatch> th;
	std::unique_ptr<boost::thread> threadRunLocalServer;
	std::unique_ptr<boost::thread> threadNetwork;

	std::unique_ptr<CClient> client;

	CondSh<bool> campaignServerRestartLock;

	CServerHandler();
	~CServerHandler();
	
	void resetStateForLobby(EStartMode mode, ESelectionScreen screen, const std::vector<std::string> & names);
	void startLocalServerAndConnect(bool connectToLobby);
	void connectToServer(const std::string & addr, const ui16 port);

	GlobalLobbyClient & getGlobalLobby();

	// Helpers for lobby state access
	std::set<PlayerColor> getHumanColors();
	PlayerColor myFirstColor() const;
	bool isMyColor(PlayerColor color) const;
	ui8 myFirstId() const; // Used by chat only!

	bool isHost() const;
	bool isGuest() const;

	const std::string & getCurrentHostname() const;
	const std::string & getLocalHostname() const;
	const std::string & getRemoteHostname() const;

	ui16 getCurrentPort() const;
	ui16 getLocalPort() const;
	ui16 getRemotePort() const;

	// Lobby server API for UI
	void sendClientConnecting() const override;
	void sendClientDisconnecting() override;
	void setCampaignState(std::shared_ptr<CampaignState> newCampaign) override;
	void setCampaignMap(CampaignScenarioID mapId) const override;
	void setCampaignBonus(int bonusId) const override;
	void setMapInfo(std::shared_ptr<CMapInfo> to, std::shared_ptr<CMapGenOptions> mapGenOpts = {}) const override;
	void setPlayer(PlayerColor color) const override;
	void setPlayerName(PlayerColor color, const std::string & name) const override;
	void setPlayerOption(ui8 what, int32_t value, PlayerColor player) const override;
	void setDifficulty(int to) const override;
	void setTurnTimerInfo(const TurnTimerInfo &) const override;
	void setSimturnsInfo(const SimturnsInfo &) const override;
	void setExtraOptionsInfo(const ExtraOptionsInfo &) const override;
	void sendMessage(const std::string & txt) const override;
	void sendGuiAction(ui8 action) const override;
	void sendRestartGame() const override;
	void sendStartGame(bool allowOnlyAI = false) const override;

	void startMapAfterConnection(std::shared_ptr<CMapInfo> to);
	bool validateGameStart(bool allowOnlyAI = false) const;
	void debugStartTest(std::string filename, bool save = false);

	void startGameplay(VCMI_LIB_WRAP_NAMESPACE(CGameState) * gameState = nullptr);
	void endGameplay(bool closeConnection = true, bool restart = false);
	void startCampaignScenario(HighScoreParameter param, std::shared_ptr<CampaignState> cs = {});
	void showServerError(const std::string & txt) const;

	// TODO: LobbyState must be updated within game so we should always know how many player interfaces our client handle
	int howManyPlayerInterfaces();
	ELoadMode getLoadMode();

	void visitForLobby(CPackForLobby & lobbyPack);
	void visitForClient(CPackForClient & clientPack);
};

extern CServerHandler * CSH;
