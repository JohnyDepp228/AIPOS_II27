#include <stdio.h>
#include <iostream>
#include <winsock2.h> 
#include <windows.h>
#include <ws2tcpip.h> 


#pragma comment(lib, "ws2_32.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define MY_PORT    666
#define PRINTNUSERS if (nclients)\
  printf("%d user on-line\n",nclients);\
  else printf("No User on line\n");
#define INET_ADDRSTRLEN 1000
DWORD WINAPI WorkWithClient(LPVOID client_socket);

// глобальна€ переменна€ Ц количество
// активных пользователей 
int nclients = 0;

int main()
{
    setlocale(LC_ALL, "RU");
    char buff[1024];    // Ѕуфер дл€ различных нужд
    std::cout << "TCP server\n";

    if (WSAStartup(0x0202, (WSADATA*)&buff[0]))
    {
        // ќшибка!
        std::cout << "Error WSAStartup %d\n" << WSAGetLastError();
        return -1;
    }


    SOCKET mysocket;

    if ((mysocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        // ќшибка!
        std::cout << "Error socket %d\n" << WSAGetLastError() << std::endl;
        WSACleanup();

        return -1;
    }

    // -------------------------------------------
    // Ўаг 3 св€зывание сокета с локальным адресом
    // -------------------------------------------
    sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(MY_PORT);
    // не забываем о сетевом пор€дке!!!
    local_addr.sin_addr.s_addr = 0;
    // сервер принимает подключени€
    // на все IP-адреса

// вызываем bind дл€ св€зывани€
    if (bind(mysocket, (sockaddr*)&local_addr,
        sizeof(local_addr)))
    {
        // ќшибка
        std::cout << "Error bind %d\n" << WSAGetLastError() << std::endl;
        closesocket(mysocket);  // закрываем сокет!
        WSACleanup();
        return -1;
    }

    // -------------------------------------------
    // Ўаг 4 ожидание подключений
    // -------------------------------------------
    // размер очереди Ц 0x100
    if (listen(mysocket, 0x100))
    {
        // ќшибка
        std::cout << "Error listen %d\n" << WSAGetLastError() << std::endl;
        closesocket(mysocket);
        WSACleanup();
        return -1;
    }

    std::cout << "ќжидание подключений\n";

    // -------------------------------------------
    // Ўаг 5 извлекаем сообщение из очереди
    // -------------------------------------------
    SOCKET client_socket;    // сокет дл€ клиента
    sockaddr_in client_addr;    // адрес клиента
    // (заполн€етс€ системой)

// функции accept необходимо передать размер
// структуры
    int client_addr_size = sizeof(client_addr);

    // цикл извлечени€ запросов на подключение из
    // очереди
    while ((client_socket = accept(mysocket, (sockaddr*)
        &client_addr, &client_addr_size)))
    {
        nclients++;
        HOSTENT* hst;
        char host[NI_MAXHOST];
        if (getnameinfo((sockaddr*)&client_addr, sizeof(client_addr), host, NI_MAXHOST, NULL, 0, 0) == 0) {
            std::cout << "Host: " << host << "\n";
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
        std::cout << "new connect! IP: " << ip_str << "\n";
        PRINTNUSERS

            DWORD thID;
        CreateThread(NULL, NULL, WorkWithClient,
            &client_socket, NULL, &thID);
    }
    return 0;
}


DWORD WINAPI WorkWithClient(LPVOID client_socket)
{
    SOCKET my_sock;
    my_sock = ((SOCKET*)client_socket)[0];
    char buff[20 * 1024];
#define sHELLO "Hello, Student!\r\n"

    // отправл€ем клиенту приветствие 
    send(my_sock, sHELLO, sizeof(sHELLO), 0);

    // цикл эхо-сервера: прием строки от клиента и
    // возвращение ее клиенту
    int bytes_recv = 0;
    while ((bytes_recv = recv(my_sock, &buff[0], sizeof(buff), 0)) && bytes_recv != SOCKET_ERROR)
        send(my_sock, &buff[0], bytes_recv, 0);
    // если мы здесь, то произошел выход из цикла по
    // причине возращени€ функцией recv ошибки Ц
    // соединение клиентом разорвано
    nclients--; // уменьшаем счетчик активных клиентов
    printf("-disconnect\n");
    PRINTNUSERS

        // закрываем сокет
        closesocket(my_sock);
    return 0;
}
