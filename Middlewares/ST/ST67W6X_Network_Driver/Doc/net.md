\page net_socket Net Socket UDP TCP

============
\tableofcontents

\section net_sock_intro NET sockets state machine

Sockets have six possible states:

| State         | Meaning                                                                                                                        |
| ------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| **RESET**     | Default state.                                                                                                                 |
| **ALLOCATED** | An available socket has been reserved.                                                                                         |
| **CONNECTED** | Connection is established; socket is ready to exchange data. It can be either a client or a server (for incoming connections). |
| **BIND**      | Socket is set to server mode and port and protocol are set, but the server is not running yet.                                 |
| **LISTENING** | Server is running, ready to accept incoming connections.                                                                       |
| **CLOSING**   | Temporary state, when close or shutdown operations are being performed, after that, state is set to RESET again.               |

\section net_sock_fsm_tcp TCP FSM

\anchor net_fig01 \image html Connectivity_FSM_TCP_sock.png "Fig.1 - TCP sockets FSM"

\section net_sock_fsm_udp UDP FSM

Since UDP is a connectionless protocol, UDP sockets use a slightly different state machine. State 'CONNECTED' is used here after the first call to 'sendto()' to signal that incoming data can be expected since communication with a server has been initiated:

\anchor net_fig02 \image html Connectivity_FSM_UDP_sock.png "Fig.2 - UDP sockets FSM"

\section net_sock_code_snippet Network socket code example

This section shows code snippets examples for TCP and UDP Client Server usage.

\subsection net_sock_code_snippet_tcp_client TCP Client

```c
int client = W6X_Net_Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

/* Supported option list in w6x_types.h*/
W6X_Net_Setsockopt(client, SOL_SOCKET, opt_name, opt_val, sizeof(opt_val))
...

uint8_t ip_addr[4] = {192, 168, 8, 1};
addr.sin_family = AF_INET;
addr.sin_port = PP_HTONS(port);
addr.sin_addr.s_addr = ATON(ip_addr);
W6X_Net_Connect(client, (struct sockaddr *)&addr, sizeof(addr));

  do /* Send the data will be done in multiple steps if > 6kB. */
  {
    bytes_sent = W6X_Net_Send(client, (void *)buffer, buffer_size, 0);
    if (bytes_sent < 0)
    {
      LogError("[%" PRIi32 "] *****> SEND ERROR <*****\n", client);
      return 1;
    }
    buffer_size -= bytes_sent;
    buffer += bytes_sent;
  } while (buffer_size > 0);

  do /* Read the data will be done in multiple steps if > 6kB. */
  {
    bytes_recvd = W6X_Net_Recv(client, (void *)buffer+ total, buffer_size-total, 0);
    if (bytes_recvd < 0)
    {
      LogError("[%" PRIi32 "] *****> RECV ERROR <*****\n", client);
      break;
    }
    total += bytes_recvd;
  } while (buffer_size > total);

W6X_Net_Close(client);
```

\subsection net_sock_code_snippet_udp_client UDP Client

```c
int client = W6X_Net_Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

/* Supported option list in w6x_types.h*/
W6X_Net_Setsockopt(client, SOL_SOCKET, opt_name, opt_val, sizeof(opt_val))
...

uint8_t ip_addr[4] = {192, 168, 8, 1};
addr.sin_family = AF_INET;
addr.sin_port = PP_HTONS(port);
addr.sin_addr.s_addr = ATON(ip_addr);
do /* Send the data will be done in multiple steps if > 6kB. */
{
  bytes_sent = W6X_Net_Sendto(client, (void *)buffer, buffer_size, (struct sockaddr *)&addr, sizeof(addr));
  if (bytes_sent < 0)
  {
    LogError("[%" PRIi32 "] *****> SEND ERROR <*****\n", client);
    return 1;
  }
  buffer_size -= bytes_sent;
  buffer += bytes_sent;
} while (buffer_size > 0);

do /* Read the data will be done in multiple steps if > 6kB. */
{
  bytes_recvd = W6X_Net_Recvfrom(client, (void *)buffer+ total, buffer_size-total, (struct sockaddr *)&addr, sizeof(addr));
  if (bytes_recvd < 0)
  {
    LogError("[%" PRIi32 "] *****> RECV ERROR <*****\n", client);
    break;
  }
  total += bytes_recvd;
} while (buffer_size > total);
W6X_Net_Close(client);
```

\subsection net_sock_code_snippet_tcp_server TCP Server

```c
int server = W6X_Net_Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


/* Supported option list in w6x_types.h*/
W6X_Net_Setsockopt(server, SOL_SOCKET, opt_name, opt_val, sizeof(opt_val))


struct sockaddr_in addr = {0};
addr.sin_port = PP_HTONS(port);
if ( W6X_Net_Bind(server, (struct sockaddr *)&addr, sizeof(addr)) != 0)
{
  LogError("Failed to bind");
  return;
}

if (W6X_Net_Listen(server, 5) != 0)
{
  LogError("Failed to start the server");
  return;
}

while(!finish)
{
  int client_socket = W6X_Net_Accept(server, (struct sockaddr *)&remote_addr, &addr_len);
  if (client_socket < 0)
  {
    LogError("Unable to accept connection");
    goto deinit;
  }

  // Receive and send data might be done in separate task
  do /* Read the data will be done in multiple steps if > 6kB. */
  {
    bytes_recvd = W6X_Net_Recv(client_socket, (void *)buffer+ total, buffer_size-total, 0);
    if (bytes_recvd < 0)
    {
      LogError("[%" PRIi32 "] *****> RECV ERROR <*****\n", client_socket);
      break;
    }
    total += bytes_recvd;
  } while (buffer_size > total);

  do /* Send the data will be done in multiple steps if > 6kB. */
  {
    bytes_sent = W6X_Net_Send(client_socket, (void *)buffer, buffer_size, 0);
    if (bytes_sent < 0)
    {
      LogError("[%" PRIi32 "] *****> SEND ERROR <*****\n", client_socket);
      return 1;
    }
    buffer_size -= bytes_sent;
    buffer += bytes_sent;
  } while (buffer_size > 0);
  W6X_Net_Close(client_socket);
}
W6X_Net_Shutdown(server, 0);
```

\subsection net_sock_code_snippet_udp_server UDP Server

```c
int server = W6X_Net_Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

/* Supported option list in w6x_types.h*/
W6X_Net_Setsockopt(server, SOL_SOCKET, opt_name, opt_val, sizeof(opt_val))

struct sockaddr_in addr = {0};
addr.sin_port = PP_HTONS(port);
if ( W6X_Net_Bind(server, (struct sockaddr *)&addr, sizeof(addr)) != 0)
{
  LogError("Failed to bind");
  return;
}

while(!finish)
{
  do /* Read the data will be done in multiple steps if > 6kB. */
  {
    bytes_recvd = W6X_Net_Recvfrom(client, (void *)buffer+ total, buffer_size-total, (struct sockaddr *)&addr, sizeof(addr));
    if (bytes_recvd < 0)
    {
      LogError("[%" PRIi32 "] *****> RECV ERROR <*****\n", client);
      break;
    }
    total += bytes_recvd;
  } while (buffer_size > total);
  do /* Send the data will be done in multiple steps if > 6kB. */
  {
    bytes_sent = W6X_Net_Sendto(client, (void *)buffer, buffer_size, (struct sockaddr *)&addr, sizeof(addr));
    if (bytes_sent < 0)
    {
      LogError("[%" PRIi32 "] *****> SEND ERROR <*****\n", client);
      return 1;
    }
    buffer_size -= bytes_sent;
    buffer += bytes_sent;
  } while (buffer_size > 0);

}
W6X_Net_Shutdown(server, 1);
```
