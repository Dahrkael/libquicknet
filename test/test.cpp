
#include "quicknet_peer.h"
#include "quicknet_messagetypes.h"
#include "quicknet_time.h"
#include "quicknet_log.h"

class TestServer : public quicknet::Peer
{
public:
	TestServer()
		: Peer(true, 8)
	{
	}

	void OnConnection(uint8_t playerID) override
	{
		quicknet::Log::Info("New player connected!");
	}

	void OnDisconnection(uint8_t playerID) override
	{
		quicknet::Log::Info("Player disconnected!");
	}

	void OnGameMessage(const quicknet::Message* const message) override
	{
		quicknet::Log::Info("New message from player!");
	}
};

class TestClient : public quicknet::Peer
{
public:
	TestClient()
		: Peer(false, 1)
	{
	}

	void OnConnection(uint8_t playerID) override
	{
		quicknet::Log::Info("Connected to server!");
	}

	void OnDisconnection(uint8_t playerID) override
	{
		quicknet::Log::Info("Disconnected from server!");
	}

	void OnGameMessage(const quicknet::Message* const message) override
	{
		quicknet::Log::Info("New message from server!");
	}
};

int main()
{
	// create both instances
	TestServer server;
	TestClient client;

	// ask the client to connect to the server
	client.ConnectTo(quicknet::Address("127.0.0.1", 8000));
	while (true)
	{
		// update both states
		server.UpdateNetwork();
		client.UpdateNetwork();

		// once connected send a message every frame (20fps internally)
		if (client.NetworkState() == quicknet::NetPeerState::Connected)
		{
			std::unique_ptr<quicknet::MessageTest> message = quicknet::MessageTest::Create();
			client.SendTo((uint8_t)0, std::move(message));
		}
		// sleep a bit to avoid cpu waste
		quicknet::Utils::SleepMilliseconds(50);
	}
    return 0;
}


