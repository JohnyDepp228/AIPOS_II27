Министерство образования Республики Беларусь
Учреждение образования
«Брестский Государственный технический университет»
Кафедра ИИТ




Лабораторная работа №1
По дисциплине «АиПОС»
Тема: «Организация TCP»



















Выполнил:
Студент 3 курса
Группы ИИ-27/24
Масюк А.Д.
Проверил: 
Кулик А.Д.






Брест 2026
# Вариант 11

# Цель работы: 

Изучить принципы работы TCP по средствам разработки двух программ, а именно, сервер и клиент.

# Задание 1:

Реализовать программно сервер. 

# Код программы:
```
#include <iostream>
#include <string>
#include <atomic>
#include <vector>
#include <winsock2.h> 
#include <windows.h>
#include <ws2tcpip.h> 
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

class TcpServer {
private:
    // Вспомогательная структура для безопасной передачи параметров в поток
    struct ThreadParam {
        TcpServer* serverPointer;
        SOCKET clientSocket;
    };

public:
    // Конструктор инициализирует Winsock и создает слушающий сокет
    TcpServer(int port) : m_port(port), m_listenSocket(INVALID_SOCKET), m_nclients(0) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed. Error: " + std::to_string(WSAGetLastError()));
        }
    }

    // Деструктор гарантирует закрытие ресурсов при уничтожении объекта класса
    ~TcpServer() {
        if (m_listenSocket != INVALID_SOCKET) {
            closesocket(m_listenSocket);
        }
        WSACleanup();
    }

    // Запуск сервера: привязка, прослушивание и запуск основного цикла
    void start() {
        m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_listenSocket == INVALID_SOCKET) {
            throw std::runtime_error("Socket creation failed. Error: " + std::to_string(WSAGetLastError()));
        }

        sockaddr_in localAddr{};
        localAddr.sin_family = AF_INET;
        localAddr.sin_port = htons(m_port);
        localAddr.sin_addr.s_addr = INADDR_ANY; // Принимаем подключения на все IP-адреса

        if (bind(m_listenSocket, reinterpret_cast<sockaddr*>(&localAddr), sizeof(localAddr)) == SOCKET_ERROR) {
            throw std::runtime_error("Bind failed. Error: " + std::to_string(WSAGetLastError()));
        }

        if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            throw std::runtime_error("Listen failed. Error: " + std::to_string(WSAGetLastError()));
        }

        std::cout << "TCP Server started on port " << m_port << "\n";
        std::cout << "Ожидание подключений...\n";

        run();
    }

private:
    int m_port;
    SOCKET m_listenSocket;
    std::atomic<int> m_nclients; // Потокобезопасный счетчик клиентов

    // Вывод количества пользователей в консоль
    void printUsersCount() const {
        int currentClients = m_nclients.load();
        if (currentClients > 0) {
            std::cout << currentClients << " user(s) on-line\n";
        }
        else {
            std::cout << "No User on line\n";
        }
    }

    // Основной цикл обработки входящих подключений
    void run() {
        while (true) {
            sockaddr_in clientAddr{};
            int clientAddrSize = sizeof(clientAddr);

            SOCKET clientSocket = accept(m_listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "Accept failed. Error: " << WSAGetLastError() << "\n";
                continue;
            }

            m_nclients++;

            // Определение имени хоста и IP-адреса клиента
            char host[NI_MAXHOST] = "";
            char ipStr[INET_ADDRSTRLEN] = "";

            getnameinfo(reinterpret_cast<sockaddr*>(&clientAddr), clientAddrSize, host, NI_MAXHOST, nullptr, 0, 0);
            inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);

            std::cout << "+ Host: " << host << " [" << ipStr << "] new connect!\n";
            printUsersCount();

            // Создаем структуру с параметрами в динамической памяти, чтобы поток забрал её атомарно
            ThreadParam* pParam = new ThreadParam{ this, clientSocket };

            // Создаем отдельный поток для обслуживания клиента
            HANDLE thID = CreateThread(nullptr, 0, clientThreadProxy, pParam, 0, nullptr);
            if (thID) {
                CloseHandle(thID); // Закрываем дескриптор потока, он нам больше не нужен в основном цикле
            }
            else {
                std::cerr << "Failed to create thread\n";
                closesocket(clientSocket);
                delete pParam; // Очищаем память, если поток не запустился
                m_nclients--;
            }
        }
    }

    // Статический прокси-метод для CreateThread
    static DWORD WINAPI clientThreadProxy(LPVOID lpParam) {
        if (!lpParam) return -1;

        // Извлекаем параметры из переданной структуры
        ThreadParam* pParam = reinterpret_cast<ThreadParam*>(lpParam);
        TcpServer* server = pParam->serverPointer;
        SOCKET clientSock = pParam->clientSocket;

        // Удаляем структуру из памяти, так как мы уже скопировали данные
        delete pParam;

        // Вызываем метод класса для обработки клиента
        server->handleClient(clientSock);
        return 0;
    }

    void ProccedCommand(const std::string& command, std::string& fileName, std::string& str) {
        size_t start = command.find('<');
        if (start == std::string::npos) {
            str = " ";
        }

        size_t end = command.find('>', start + 1);
        if (end == std::string::npos) {
            str = " ";
        }

        // 3. Вырезаем текст между ними
        str = command.substr(start + 1, end - start - 1);

        start = end;

        start = command.find('<', start + 1);
        if (start == std::string::npos) {
            fileName = " ";
        }

        // 2. Ищем вторую кавычку, начиная со следующего символа
        end = command.find('>', start + 1);
        if (end == std::string::npos) {
            fileName = " ";
        }

        fileName = command.substr(start + 1, end - start - 1);
    }

    bool CheckForCommand(const std::string& command) {
        std::string filename;
        std::string str;
        if (command.find("find") != std::string::npos) {
            ProccedCommand(command, filename, str);
            std::cout << "Filename: " << filename << std::endl;
            std::cout << "Str: " << str << std::endl;
            std::cout << "Find: " << FindInFile(filename, str) << std::endl;
            return true;
        }
        else {
            return false;
        }
    }

    int FindInFile(std::string filename, std::string str) {
        int numOfStr = 0;
        std::ifstream file(filename);


        if (!file.is_open()) {
            std::cerr << "Не удалось открыть файл!" << std::endl;
            return numOfStr;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.find(str) != std::string::npos) {
                numOfStr++;
            }
        }


        file.close();

        return numOfStr;
    }

    // Метод непосредственного общения с клиентом (Эхо-режим)
    void handleClient(SOCKET clientSocket) {
        const std::string helloMessage = "Hello, Student!\r\n";
        send(clientSocket, helloMessage.c_str(), static_cast<int>(helloMessage.size()), 0);

        std::vector<char> buffer(20 * 1024);
        int bytesRecv = 0;


        auto ShowVec = [](const std::vector<char> vec) {
            std::cout << "S<=C: ";
            for (const auto& n : vec) {
                std::cout << n;
            }
            std::cout << std::endl;
            return 0;
            };

        auto SetString = [](const std::vector<char> vec, std::string& command) {

            for (const auto& n : vec) {
                command += n;
            }
            return 0;
            };
        std::string command;
        // Цикл эхо-ответа
        while ((bytesRecv = recv(clientSocket, buffer.data(), static_cast<int>(buffer.size()), 0)) > 0) {

            send(clientSocket, buffer.data(), bytesRecv, 0);


            std::string command(buffer.data(), bytesRecv);


            std::cout << "S<=C: " << command << std::endl;


            if (!CheckForCommand(command)) {
                std::cout << "No command" << std::endl;
            }
        }

        // Клиент отключился или произошла ошибка
        m_nclients--;
        std::cout << "-disconnect\n";
        printUsersCount();

        closesocket(clientSocket);
    }
};

int main() {
    setlocale(LC_ALL, "RU");

    try {
        // Создаем экземпляр сервера на порту 666 и запускаем его
        TcpServer server(666);
        server.start();
    }
    catch (const std::exception& ex) {
        std::cerr << "Критическая ошибка: " << ex.what() << "\n";
        return -1;
    }

    return 0;
}

```
# Задание 2:

Реализовать программно клиента.

Код программы:
```
#include <iostream>
#include <string>
#include <vector>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h> 
#include <chrono>
#include <iomanip> 

#pragma comment(lib, "ws2_32.lib")

class TcpClient {
public:
    // Конструктор инициализирует Winsock
    TcpClient(const std::string& serverIp, int port)
        : m_serverIp(serverIp), m_port(port), m_socket(INVALID_SOCKET)
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed. Error: " + std::to_string(WSAGetLastError()));
        }
    }

    // Деструктор автоматически закрывает ресурсы
    ~TcpClient() {
        disconnect();
        WSACleanup();
    }

    // Подключение к серверу
    void connectToServer() {
        m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_socket == INVALID_SOCKET) {
            throw std::runtime_error("Socket creation failed. Error: " + std::to_string(WSAGetLastError()));
        }

        sockaddr_in destAddr{};
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(m_port);

        // Преобразование IP адреса (проверка строки на валидность адреса / имени хоста)
        if (inet_pton(AF_INET, m_serverIp.c_str(), &destAddr.sin_addr) != 1) {
            addrinfo hints{}, * res = nullptr;
            hints.ai_family = AF_INET;

            if (getaddrinfo(m_serverIp.c_str(), nullptr, &hints, &res) == 0 && res != nullptr) {
                destAddr.sin_addr = reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr;
                freeaddrinfo(res);
            }
            else {
                closesocket(m_socket);
                m_socket = INVALID_SOCKET;
                throw std::runtime_error("Invalid address or host not found: " + m_serverIp);
            }
        }

        // Установка соединения
        if (connect(m_socket, reinterpret_cast<sockaddr*>(&destAddr), sizeof(destAddr)) == SOCKET_ERROR) {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            throw std::runtime_error("Connect failed. Error: " + std::to_string(WSAGetLastError()));
        }

        std::cout << "Соединение с " << m_serverIp << " успешно установлено\n";
        std::cout << "Type quit for quit\n\n";
    }

    // Основной цикл обмена сообщениями
    void runCommunicationLoop() {
        if (m_socket == INVALID_SOCKET) {
            std::cerr << "Нет активного соединения с сервером.\n";
            return;
        }
        else {
            auto now = std::chrono::system_clock::now();

            std::time_t currentTime = std::chrono::system_clock::to_time_t(now);


            std::tm localTime;
            localtime_s(&localTime, &currentTime);


            std::cout << "Start time: "
                << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S")
                << std::endl;
        }

        std::vector<char> buffer(1024);
        int bytesRecv = 0;

        // Цикл чтения сообщений от сервера
        while ((bytesRecv = recv(m_socket, buffer.data(), static_cast<int>(buffer.size() - 1), 0)) > 0) {
            buffer[bytesRecv] = '\0';


            auto now = std::chrono::system_clock::now();

            std::time_t currentTime = std::chrono::system_clock::to_time_t(now);


            std::tm localTime;
            localtime_s(&localTime, &currentTime);

            std::cout << "Recieved msg from server at "
                << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "\t" << buffer.data();


            std::cout << "Send to server:  ";
            std::string userInput;
            std::getline(std::cin, userInput);


            if (userInput == "quit") {
                std::cout << "Exit...\n";
                break;
            }

            // Добавляем символы перевода строки, чтобы сервер читал это как пакет данных
            userInput += "\r\n";

            // Передаем строку клиента серверу с точным расчетом длины пользовательского ввода
            if (send(m_socket, userInput.c_str(), static_cast<int>(userInput.size()), 0) == SOCKET_ERROR) {
                std::cerr << "Send failed. Error: " << WSAGetLastError() << "\n";
                break;
            }
        }

        if (bytesRecv == SOCKET_ERROR) {
            std::cerr << "Recv error: " << WSAGetLastError() << "\n";
        }

        disconnect();
    }

    // Принудительное закрытие сокета
    void disconnect() {
        if (m_socket != INVALID_SOCKET) {
            closesocket(m_socket);
            auto now = std::chrono::system_clock::now();

            std::time_t currentTime = std::chrono::system_clock::to_time_t(now);


            std::tm localTime;
            localtime_s(&localTime, &currentTime);


            std::cout << "End time: "
                << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S")
                << std::endl;
            m_socket = INVALID_SOCKET;
        }
    }

private:
    std::string m_serverIp;
    int m_port;
    SOCKET m_socket;
};

int main() {
    setlocale(LC_ALL, "RU");
    std::cout << "TCP CLIENT\n";

    try {
        // Создаем экземпляр клиента (IP сервера: 127.0.0.1, Порт: 666)
        TcpClient client("127.0.0.1", 666);

        // Подключаемся и запускаем цикл обмена сообщениями
        client.connectToServer();
        client.runCommunicationLoop();
    }
    catch (const std::exception& ex) {
        std::cerr << "Критическая ошибка: " << ex.what() << "\n";
        return -1;
    }

    return 0;
}

```

# Результат работы:
 

 

# Вывод:
Изучили принцип работы TCP.
