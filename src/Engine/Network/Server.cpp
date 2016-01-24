#include "Network/Server.h"

Server::Server() : m_Socket(m_IOService, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 27666))
{
    Network::initialize();
    ConfigFile* config = ResourceManager::Load<ConfigFile>("Config.ini");
    snapshotInterval = 1000 * config->Get<float>("Networking.SnapshotInterval", 0.05);
    pingIntervalMs = config->Get<float>("Networking.PingIntervalMs", 1000);

}

Server::~Server()
{

}

void Server::Start(World* world, EventBroker* eventBroker)
{
    m_World = world;
    m_EventBroker = eventBroker;
    // Subscribe to events
    EVENT_SUBSCRIBE_MEMBER(m_EInputCommand, &Server::OnInputCommand);
    EVENT_SUBSCRIBE_MEMBER(m_EPlayerSpawned, &Server::OnPlayerSpawned);
    EVENT_SUBSCRIBE_MEMBER(m_EEntityDeleted, &Server::OnEntityDeleted);
    EVENT_SUBSCRIBE_MEMBER(m_EComponentDeleted, &Server::OnComponentDeleted);
    for (size_t i = 0; i < m_MaxConnections; i++) {
        m_PlayerDefinitions[i].StopTime = std::clock();
    }
    LOG_INFO("I am Server. BIP BOP\n");
}

void Server::Update()
{
    readFromClients();
    m_EventBroker->Process<Server>();
    if (isReadingData) {
        Network::Update();
    }

}

void Server::readFromClients()
{
    while (m_Socket.available()) {
        try {
            bytesRead = receive(readBuffer);
            Packet packet(readBuffer, bytesRead);
            parseMessageType(packet);
        } catch (const std::exception& err) {
            //LOG_ERROR("%i: Read from client crashed %s", m_PacketID, err.what());
        }
    }
    std::clock_t currentTime = std::clock();
    // Send snapshot
    if (snapshotInterval < (1000 * (currentTime - previousSnapshotMessage) / (double)CLOCKS_PER_SEC)) {
        sendSnapshot();
        previousSnapshotMessage = currentTime;
    }

    // Send pings each 
    if (pingIntervalMs < (1000 * (currentTime - previousePingMessage) / (double)CLOCKS_PER_SEC)) {
        sendPing();
        previousePingMessage = currentTime;
    }

    // Time out logic
    if (checkTimeOutInterval < (1000 * (currentTime - timOutTimer) / (double)CLOCKS_PER_SEC)) {
        checkForTimeOuts();
        timOutTimer = currentTime;
    }
}

void Server::parseMessageType(Packet& packet)
{
    int messageType = packet.ReadPrimitive<int>(); // Read what type off message was sent from server

    // Read packet ID 
    m_PreviousPacketID = m_PacketID;    // Set previous packet id
    m_PacketID = packet.ReadPrimitive<int>(); //Read new packet id
    //identifyPacketLoss();
    switch (static_cast<MessageType>(messageType)) {
    case MessageType::Connect:
        parseConnect(packet);
        break;
    case MessageType::Ping:
        parsePing();
        break;
    case MessageType::Message:
        break;
    case MessageType::Snapshot:
        break;
    case MessageType::Disconnect:
        parseDisconnect();
        break;
    case MessageType::OnInputCommand:
        parseOnInputCommand(packet);
        break;
    case MessageType::OnPlayerDamage:
        parseOnPlayerDamage(packet);
        break;
    case MessageType::BecomePlayer:
        createPlayer();
        break;
    default:
        break;
    }
}

int Server::receive(char * data)
{
    unsigned int length = m_Socket.receive_from(
        boost::asio::buffer((void*)data
            , INPUTSIZE)
        , m_ReceiverEndpoint, 0);
    // Network Debug data
    if (isReadingData) {
        m_NetworkData.TotalDataReceived += length;
        m_NetworkData.DataReceivedThisInterval += length;
        m_NetworkData.AmountOfMessagesReceived++;
    }
    return length;
}

void Server::send(Packet& packet, UserID user)
{
    int bytesSent = m_Socket.send_to(
        boost::asio::buffer(packet.Data(), packet.Size()),
        m_ConnectedUsers[user].Endpoint,
        0);
    // Network Debug data
    if (isReadingData) {
        m_NetworkData.TotalDataSent += packet.Size();
        m_NetworkData.DataSentThisInterval += packet.Size();
        m_NetworkData.AmountOfMessagesSent++;
    }
}

void Server::send(PlayerID player, Packet& packet)
{
    int bytesSent = m_Socket.send_to(
        boost::asio::buffer(packet.Data(), packet.Size()),
        m_PlayerDefinitions[player].Endpoint,
        0);
    // Network Debug data
    if (isReadingData) {
        m_NetworkData.TotalDataSent += packet.Size();
        m_NetworkData.DataSentThisInterval += packet.Size();
        m_NetworkData.AmountOfMessagesSent++;
    }
}

void Server::send(Packet & packet)
{
    m_Socket.send_to(
        boost::asio::buffer(
            packet.Data(),
            packet.Size()),
        m_ReceiverEndpoint,
        0);
    if (isReadingData) {
        // Network Debug data
        m_NetworkData.TotalDataSent += packet.Size();
        m_NetworkData.DataSentThisInterval += packet.Size();
    }
}

void Server::broadcast(Packet& packet)
{
    for (int i = 0; i < m_ConnectedUsers.size(); i++) {
        if (m_ConnectedUsers[i].Endpoint.address() != boost::asio::ip::address()) {
            packet.ChangePacketID(m_ConnectedUsers[i].PacketID);
            send(packet, i);
        }
    }
}

// Send snapshot fields
void Server::sendSnapshot()
{
    Packet packet(MessageType::Snapshot);
    addChildrenToPacket(packet, EntityID_Invalid);
    broadcast(packet);
}

void Server::addChildrenToPacket(Packet & packet, EntityID entityID)
{
    auto itPair = m_World->GetChildren(entityID);
    std::unordered_map<std::string, ComponentPool*> worldComponentPools = m_World->GetComponentPools();
    // Loop through every child
    for (auto it = itPair.first; it != itPair.second; it++) {
        EntityID childEntityID = it->second;
        // Write EntityID and parentsID and Entity name
        packet.WritePrimitive(childEntityID);
        packet.WritePrimitive(entityID);
        packet.WriteString(m_World->GetName(childEntityID));
        // Write components to child
        int numberOfComponents = 0;
        for (auto& i : worldComponentPools) {
            if (i.second->KnowsEntity(childEntityID)) {
                numberOfComponents++;
            }
        }
        // Write how many components should be read
        packet.WritePrimitive(numberOfComponents);
        for (auto& i : worldComponentPools) {
            // If the entity exist in the pool
            if (i.second->KnowsEntity(childEntityID)) {
                ComponentWrapper componentWrapper = i.second->GetByEntity(childEntityID);
                // ComponentType
                packet.WriteString(componentWrapper.Info.Name);
                // Loop through fields
                for (auto& componentField : componentWrapper.Info.FieldsInOrder) {
                    ComponentInfo::Field_t fieldInfo = componentWrapper.Info.Fields.at(componentField);
                    if (fieldInfo.Type == "string") {
                        std::string& value = componentWrapper[componentField];
                        packet.WriteString(value);
                    } else {
                        packet.WriteData(componentWrapper.Data + fieldInfo.Offset, fieldInfo.Stride);
                    }
                }
            }
        }
        // Go to to your children
        addChildrenToPacket(packet, childEntityID);
    }
}

void Server::sendPing()
{
    // Prints connected players ping
    for (int i = 0; i < m_ConnectedUsers.size(); i++) {
        if (m_ConnectedUsers[i].Endpoint.address() != boost::asio::ip::address()) {
            int ping = 1000 * (m_ConnectedUsers[i].StopTime - m_StartPingTime) / static_cast<double>(CLOCKS_PER_SEC);
            LOG_INFO("Last packetID received %i: User %i's ping: %i", m_ConnectedUsers[i].PacketID, i, std::abs(ping));
        }
    }
    // Create ping message
    Packet packet(MessageType::Ping);
    packet.WriteString("Ping from server");
    // Time message
    m_StartPingTime = std::clock();
    // Send message
    broadcast(packet);
}

void Server::checkForTimeOuts()
{
    int startPing = 1000 * m_StartPingTime
        / static_cast<double>(CLOCKS_PER_SEC);

    for (int i = 0; i < m_ConnectedUsers.size(); i++) {
        if (m_ConnectedUsers[i].Endpoint.address() != boost::asio::ip::address()) {
            int stopPing = 1000 * m_ConnectedUsers[i].StopTime /
                static_cast<double>(CLOCKS_PER_SEC);
            if (startPing > stopPing + m_TimeoutMs) {
                LOG_INFO("User %i timed out!", i);
                disconnect(i);
            }
        }
    }
}

void Server::disconnect(UserID user)
{
    //broadcast("A player disconnected");
    LOG_INFO("User %s disconnected/timed out", m_PlayerDefinitions[user].Name.c_str());
    // Remove enteties and stuff (When we can remove entity, remove it and tell clients to remove the copy they have)
    Events::PlayerDisconnected e;
    e.Entity = m_PlayerDefinitions[user].EntityID;
    e.PlayerID = user;
    m_EventBroker->Publish(e);

    m_PlayerDefinitions[user].Endpoint = boost::asio::ip::udp::endpoint();
    m_PlayerDefinitions[user].EntityID = -1;
    m_PlayerDefinitions[user].Name = "";
    m_PlayerDefinitions[user].PacketID = 0;
    m_ConnectedUsers.erase(m_ConnectedUsers.begin() + user);
}

void Server::parseOnInputCommand(Packet& packet)
{
    PlayerID player = -1;
    // Check which player it was who sent the message
    for (int i = 0; i < m_MaxConnections; i++) {
        // if the player is connected set playerID to the correct PlayerID
        if (m_PlayerDefinitions[i].Endpoint.address() == m_ReceiverEndpoint.address()
            && m_PlayerDefinitions[i].Endpoint.port() == m_ReceiverEndpoint.port()) {
            player = i;
            break;
        }
    }
    if (player != -1) {
        while (packet.DataReadSize() < packet.Size()) {
            Events::InputCommand e;
            e.Command = packet.ReadString();
            e.PlayerID = player; // Set correct player id
            e.Value = packet.ReadPrimitive<float>();
            m_EventBroker->Publish(e);
            //LOG_INFO("Server::parseOnInputCommand: Command is %s. Value is %f. PlayerID is %i.", e.Command.c_str(), e.Value, e.PlayerID);
        }
    }
}

void Server::parseOnPlayerDamage(Packet & packet)
{
    Events::PlayerDamage e;
    e.DamageAmount = packet.ReadPrimitive<double>();
    e.PlayerDamagedID = packet.ReadPrimitive<EntityID>();
    m_EventBroker->Publish(e);
    //LOG_DEBUG("Server::parseOnPlayerDamage: Command is %s. Value is %f. PlayerID is %i.", e.DamageAmount, e.PlayerDamagedID, e.TypeOfDamage.c_str());
}

void Server::parseConnect(Packet& packet)
{
    LOG_INFO("Parsing connections");
    // Check if player is already connected
    if (GetPlayerIDFromEndpoint(m_ReceiverEndpoint) != -1) {
        return;
    }
    for (int i = 0; i < m_ConnectedUsers.size(); i++) {
        if (m_ConnectedUsers[i].Endpoint.address() == m_ReceiverEndpoint.address() &&
            m_ConnectedUsers[i].Endpoint.port() == m_ReceiverEndpoint.port()) {
            // Already connected 
            return;
        }
    }
    // Create a new player
    PlayerDefinition pd;
    pd.EntityID = 0; // Overlook this
    pd.Endpoint = m_ReceiverEndpoint;
    pd.Name = packet.ReadString();
    pd.PacketID = 0;
    pd.StopTime = std::clock();
    m_ConnectedUsers.push_back(pd);
    LOG_INFO("Spectator \"%s\" connected on IP: %s", pd.Name.c_str(), pd.Endpoint.address().to_string().c_str());

    // Send a message to the player that connected
    Packet connnectPacket(MessageType::Connect, m_ConnectedUsers[m_ConnectedUsers.size() - 1].PacketID);
    send(connnectPacket);

    // Send notification that a player has connected
    Packet notificationPacket(MessageType::PlayerConnected);
    broadcast(notificationPacket);
}

void Server::parseDisconnect()
{
    LOG_INFO("%i: Parsing disconnect", m_PacketID);

    for (int i = 0; i < m_ConnectedUsers.size(); i++) {
        if (m_ConnectedUsers[i].Endpoint.address() == m_ReceiverEndpoint.address()) {
            disconnect(i);
            break;
        }
    }
}

void Server::parseClientPing()
{
    LOG_INFO("%i: Parsing ping", m_PacketID);
    PlayerID player = GetPlayerIDFromEndpoint(m_ReceiverEndpoint);
    if (player == -1) {
        return;
    }
    // Return ping
    Packet packet(MessageType::Ping, m_PlayerDefinitions[player].PacketID);
    packet.WriteString("Ping received");
    send(packet);
}

void Server::parsePing()
{
    for (int i = 0; i < m_ConnectedUsers.size(); i++) {
        if (m_ConnectedUsers[i].Endpoint.address() == m_ReceiverEndpoint.address()) {
            m_ConnectedUsers[i].StopTime = std::clock();
            break;
        }
    }
}

void Server::identifyPacketLoss()
{
    // if no packets lost, difference should be equal to 1
    int difference = m_PacketID - m_PreviousPacketID;
    if (difference != 1) {
        LOG_INFO("%i Packet(s) were lost...", difference);
    }
}

void Server::createPlayer()
{
    if (GetPlayerIDFromEndpoint(m_ReceiverEndpoint) != -1) {
        // Already connected as player
        LOG_WARNING("Already connected!");
        return;
    }
    UserID userIndex;
    for (userIndex = 0; userIndex < m_ConnectedUsers.size(); userIndex++) {
        if (m_ConnectedUsers[userIndex].Endpoint.address() == m_ReceiverEndpoint.address() &&
            m_ConnectedUsers[userIndex].Endpoint.port() == m_ReceiverEndpoint.port()) {
            // Found user
            break;
        }
    }
    if (userIndex == m_ConnectedUsers.size()) {
        LOG_WARNING("Not a recognized user!");
        return;
    }
    for (PlayerID playerIndex = 0; playerIndex < m_MaxConnections; playerIndex++) {
        if (m_PlayerDefinitions[playerIndex].Endpoint.address() == boost::asio::ip::address()) {
            m_PlayerDefinitions[playerIndex] = m_ConnectedUsers[userIndex];
            return;
        }
    }
    LOG_WARNING("Server is full!");

}

void Server::kick(PlayerID player)
{
    disconnect(player);
    Packet packet = Packet(MessageType::Kick);
    send(packet);
}

PlayerID Server::GetPlayerIDFromEndpoint(boost::asio::ip::udp::endpoint endpoint)
{
    for (int i = 0; i < m_MaxConnections; i++) {
        if (m_PlayerDefinitions[i].Endpoint.address() == endpoint.address() &&
            m_PlayerDefinitions[i].Endpoint.port() == endpoint.port()) {
            return i;
        }
    }
    return -1;
}

bool Server::OnInputCommand(const Events::InputCommand & e)
{
    //LOG_DEBUG("Server::OnInputCommand: Command is %s. Value is %f. PlayerID is %i.", e.Command.c_str(), e.Value, e.PlayerID);
    if (e.Command == "LogNetworkBandwidth" && e.Value > 0) {
        if (isReadingData) {
            saveToFile();
        }
        isReadingData = !isReadingData;
        m_SaveDataTimer = std::clock();
    }
    if (e.Command == "KickPlayer" && e.Value > 0) {
        kick(0);
    }

    return true;
}

bool Server::OnPlayerSpawned(const Events::PlayerSpawned & e)
{
    Packet packet = Packet(MessageType::OnPlayerSpawned);
    packet.WritePrimitive<EntityID>(e.Player.ID);
    packet.WritePrimitive<EntityID>(e.Spawner.ID);
    send(e.PlayerID, packet);
    return false;
}

bool Server::OnEntityDeleted(const Events::EntityDeleted & e)
{
    if (!e.Cascaded) {
        Packet packet = Packet(MessageType::EntityDeleted);
        packet.WritePrimitive<EntityID>(e.DeletedEntity);
        broadcast(packet);
    }
    return false;
}

bool Server::OnComponentDeleted(const Events::ComponentDeleted & e)
{
    if (!e.Cascaded) {
        Packet packet = Packet(MessageType::ComponentDeleted);
        packet.WritePrimitive<EntityID>(e.Entity);
        packet.WriteString(e.ComponentType);
        broadcast(packet);
    }
    return false;
}
