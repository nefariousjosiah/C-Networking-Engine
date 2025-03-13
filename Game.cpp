#include <SFML/Graphics.hpp>
#include <enet/enet.h>
#include <iostream>
#include <cstring>

/* Check for collision
bool checkCollision(const sf::RectangleShape& rect1, const sf::RectangleShape& rect2) {
    return rect1.getGlobalBounds().intersects(rect2.getGlobalBounds());
}
*/

void runServer(sf::RenderWindow& window, sf::RectangleShape& clientBlock, sf::RectangleShape& serverBlock, sf::Text& connectionText) {
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 17011;

    ENetHost* server = enet_host_create(&address, 2, 2, 0, 0);
    if (!server) {
        std::cerr << "Failed to create ENet server.\n";
        return;
    }

    std::cout << "Server started on port 17011.\n";

    sf::Clock clock;
    sf::Clock countdownClock;
    const float moveSpeed = 200.0f;
    ENetPeer* clientPeer = nullptr;

    bool gameStarted = false;
    bool gameEnded = false;
    std::string winnerText;

    // Set up countdown
    int countdown = 3;

    while (window.isOpen()) {
        sf::Time deltaTime = clock.restart();
        float dt = deltaTime.asSeconds();

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        if (!gameStarted && clientPeer) {
            // Countdown logic
            if (countdownClock.getElapsedTime().asSeconds() > 1.0f) {
                countdown--;
                countdownClock.restart();
            }
            if (countdown <= 0) {
                gameStarted = true;
                countdown = 0;
                connectionText.setString("Go!");
            }
            else {
                connectionText.setString(std::to_string(countdown));
            }
        }

        // Handle server block movement
        if (gameStarted && !gameEnded) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) serverBlock.move(0.0f, -moveSpeed * dt);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) serverBlock.move(-moveSpeed * dt, 0.0f);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) serverBlock.move(0.0f, moveSpeed * dt);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) serverBlock.move(moveSpeed * dt, 0.0f);
        }

        // Networking
        ENetEvent netEvent;
        while (enet_host_service(server, &netEvent, 0) > 0) {
            switch (netEvent.type) {
            case ENET_EVENT_TYPE_CONNECT:
                std::cout << "A client connected.\n";
                connectionText.setString("Client Connected!");
                clientPeer = netEvent.peer;

                // Set initial positions
                serverBlock.setPosition(50.0f, 300.0f);
                clientBlock.setPosition(700.0f, 300.0f);
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                // Update client block position from client data
                if (netEvent.packet->dataLength == sizeof(float) * 2) {
                    float x, y;
                    std::memcpy(&x, netEvent.packet->data, sizeof(float));
                    std::memcpy(&y, netEvent.packet->data + sizeof(float), sizeof(float));
                    clientBlock.setPosition(x, y);
                }
                enet_packet_destroy(netEvent.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << "Client disconnected.\n";
                connectionText.setString("Client Disconnected!");
                clientPeer = nullptr;
                break;

            default:
                break;
            }
        }

        /* Check for collision
        if (gameStarted && !gameEnded && checkCollision(serverBlock, clientBlock)) {
            gameEnded = true;
            winnerText = "Server Wins!";
            connectionText.setString(winnerText);
        }
        */

        // Send server block position to client
        if (clientPeer) {
            float posData[2] = { serverBlock.getPosition().x, serverBlock.getPosition().y };
            ENetPacket* packet = enet_packet_create(posData, sizeof(posData), ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(clientPeer, 0, packet);
        }

        // Render
        window.clear(sf::Color::Blue);
        window.draw(serverBlock);
        window.draw(clientBlock);
        window.draw(connectionText); // Draw the connection text
        window.display();
    }

    enet_host_destroy(server);
}

int main() {
    if (enet_initialize() != 0) {
        std::cerr << "Failed to initialize ENet.\n";
        return EXIT_FAILURE;
    }

    sf::RenderWindow window(sf::VideoMode(800, 600), "Multiplayer Game");
    sf::RectangleShape serverBlock(sf::Vector2f(50.0f, 50.0f)); // Server block (Red)
    sf::RectangleShape clientBlock(sf::Vector2f(50.0f, 50.0f)); // Client block (Green)

    serverBlock.setFillColor(sf::Color::Red);
    clientBlock.setFillColor(sf::Color::Green);

    // Load font
    sf::Font font;
    if (!font.loadFromFile("pixel.ttf")) {
        std::cerr << "Failed to load font.\n";
        return EXIT_FAILURE;
    }

    // Setup text for connection messages
    sf::Text connectionText;
    connectionText.setFont(font);
    connectionText.setString("Waiting for client...");
    connectionText.setCharacterSize(24);
    connectionText.setFillColor(sf::Color::White);
    connectionText.setPosition(10.0f, 10.0f);

    runServer(window, clientBlock, serverBlock, connectionText);

    enet_deinitialize();
    return 0;
}
