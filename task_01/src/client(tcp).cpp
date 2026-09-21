#include <stdio.h>
#include <iostream>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h> 


#pragma comment(lib, "ws2_32.lib")


#define PORT 666
#define SERVERADDR "127.0.0.1"

int main()
{
    setlocale(LC_ALL, "RU");
    char buff[1024];
    printf("TCP CLIENT\n");

    // Ўаг 1 - инициализаци€ библиотеки Winsock
    if (WSAStartup(0x202, (WSADATA*)&buff[0]))
    {
        printf("WSAStart error %d\n", WSAGetLastError());
        return -1;
    }

    // Ўаг 2 - создание сокета
    SOCKET my_sock;
    my_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (my_sock < 0)
    {
        printf("Socket() error %d\n", WSAGetLastError());
        return -1;
    }

    // Ўаг 3 - установка соединени€

    // заполнение структуры sockaddr_in
    // указание адреса и порта сервера
    sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    HOSTENT* hst;

    // преобразование IP адреса из символьного в
    // сетевой формат
    if (inet_pton(AF_INET, SERVERADDR, &dest_addr.sin_addr) != 1) {
        addrinfo hints = { 0 }, * res = nullptr;
        hints.ai_family = AF_INET;
        if (getaddrinfo(SERVERADDR, nullptr, &hints, &res) == 0 && res != nullptr) {
            dest_addr.sin_addr = ((sockaddr_in*)res->ai_addr)->sin_addr;
            freeaddrinfo(res);
        }
        else {
            printf("Invalid address %s\n", SERVERADDR); closesocket(my_sock); WSACleanup(); return -1;
        }
    }

    // адрес сервера получен Ц пытаемс€ установить
    // соединение 
    if (connect(my_sock, (sockaddr*)&dest_addr,
        sizeof(dest_addr)))
    {
        printf("Connect error %d\n", WSAGetLastError());
        return -1;
    }

    printf("—оединение с %s успешно установлено\n\
    Type quit for quit\n\n", SERVERADDR);

    // Ўаг 4 - чтение и передача сообщений
    int nsize;
    while ((nsize = recv(my_sock, &buff[0],
        sizeof(buff) - 1, 0))
        != SOCKET_ERROR)
    {
        // ставим завершающий ноль в конце строки 
        buff[nsize] = 0;

        // выводим на экран 
        printf("S=>C:%s", buff);

        // читаем пользовательский ввод с клавиатуры
        printf("S<=C:"); fgets(&buff[0], sizeof(buff) - 1,
            stdin);

        // проверка на "quit"
        if (!strcmp(&buff[0], "quit\n"))
        {
            //  орректный выход
            printf("Exit...");
            closesocket(my_sock);
            WSACleanup();
            return 0;
        }

        // передаем строку клиента серверу
        send(my_sock, &buff[0], nsize, 0);
    }

    printf("Recv error %d\n", WSAGetLastError());
    closesocket(my_sock);
    WSACleanup();
    return -1;
}
