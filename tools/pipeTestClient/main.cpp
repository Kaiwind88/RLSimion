#include <stdio.h>
#include <string>
#include <thread>
#include "../WindowsUtils/NamedPipe.h"

#ifdef _DEBUG
#pragma comment(lib,"../../Debug/WindowsUtils.lib")
#else
#pragma comment(lib,"../../Release/WindowsUtils.lib")
#endif


void main(char** argv, int argc)
{
	char message[100] = "my message";
	CNamedPipeClient client;

	

	printf("CLIENT: attempting connection to server\n");
	bool bConnected= client.connectToServer("myPipe");
	if (bConnected)
	{
		printf("CLIENT: Client connected\n");
		int numBytes = client.writeBuffer(message, strlen(message) + 1);
		printf("CLIENT: %d bytes written: %s\n", numBytes, message);

		printf("CLIENT: closing connection");
		client.closeConnection();
	}
	else
	{
		printf("CLIENT: failed connection");
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}