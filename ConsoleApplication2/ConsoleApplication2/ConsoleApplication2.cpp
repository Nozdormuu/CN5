
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

std::mutex mtx;

void sendMessage(SOCKET client) {
   int iResult;

   while (true) {
      std::string str;
      std::getline(std::cin, str); // Use getline to allow spaces in messages
      iResult = send(client, str.c_str(), (int)str.length(), 0);
      if (iResult == SOCKET_ERROR) {
         std::cerr << "send failed with error: " << WSAGetLastError() << std::endl;
         break;
      }
   }
}

void receiveMessage(SOCKET client) {
   char recvbuf[DEFAULT_BUFLEN];
   int iResult;

   while (true) {
      iResult = recv(client, recvbuf, DEFAULT_BUFLEN - 1, 0);
      if (iResult > 0) {
         recvbuf[iResult] = '\0'; // Null-terminate the received string
         std::lock_guard<std::mutex> lock(mtx);
         std::cout << "Received: " << recvbuf << std::endl;
      }
      else if (iResult == 0) {
         std::cout << "Connection closed" << std::endl;
         break;
      }
      else {
         std::cerr << "recv failed with error: " << WSAGetLastError() << std::endl;
         break;
      }
   }
}

int __cdecl main(int argc, char** argv) {
   WSADATA wsaData;
   SOCKET ConnectSocket = INVALID_SOCKET;
   struct addrinfo* result = NULL,
      * ptr = NULL,
      hints;
   const char* sendbuf = "this is a test";
   int iResult;

   // Initialize Winsock
   iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
   if (iResult != 0) {
      printf("WSAStartup failed with error: %d\n", iResult);
      return 1;
   }

   ZeroMemory(&hints, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_protocol = IPPROTO_TCP;

   

   iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
   if (iResult != 0) {
      printf("getaddrinfo failed with error: %d\n", iResult);
      WSACleanup();
      return 1;
   }

   // Attempt to connect to an address until one succeeds
   for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
      // Create a SOCKET for connecting to server
      ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
         ptr->ai_protocol);
      if (ConnectSocket == INVALID_SOCKET) {
         printf("socket failed with error: %ld\n", WSAGetLastError());
         WSACleanup();
         return 1;
      }

      // Connect to server.
      iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
      if (iResult == SOCKET_ERROR) {
         closesocket(ConnectSocket);
         ConnectSocket = INVALID_SOCKET;
         continue;
      }
      break;
   }

   freeaddrinfo(result);

   if (ConnectSocket == INVALID_SOCKET) {
      printf("Unable to connect to server!\n");
      WSACleanup();
      return 1;
   }

   // Start the send and receive threads
   std::thread send_thr(sendMessage, ConnectSocket);
   std::thread recv_thr(receiveMessage, ConnectSocket);

   // Join threads to the main thread
   send_thr.join();
   recv_thr.join();

   // Cleanup
   closesocket(ConnectSocket);
   WSACleanup();

   return 0;
}