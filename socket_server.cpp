// ==============================
// By Jimmy
//
// 2018/12/14
//
// 1. sort out code
// ==============================

#include "socket_server.h"

#include "opencv2/opencv.hpp"

class ServerSocket
{
    SOCKET server_sock; // server fd
    PORT server_port; // server port

    SOCKET maxfd; // record the max socket fd (server and client) for select() loop

    SELECT_SET masterfds; // store all socket fd (server and client)

    int timeout; // readfds sock timeout, shutdown after timeout millis.
    
    int quality; // jpeg compression [1..100], not use in server

    // shutdown server
    bool _release()
    {
        if (server_sock != INVALID_SOCKET)
            shutdown(server_sock, 2); // disable receive or send data, like close()
        server_sock = (INVALID_SOCKET);
        return false;
    }

    // open server 
    //
    // @port: server port
    //
    bool _open(int port)
    {
        // initial server fd
        server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        SOCKADDR_IN address; // server address information
        address.sin_addr.s_addr = INADDR_ANY; // get all host ip to server address
        address.sin_family = AF_INET; // tcp
        address.sin_port = htons(port); // convert port number to network format

        int optval = 1; // set SO_REUSEADDR(1) is true
        setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval); // allow reuse port for some binding error

        // bind server with port
        if (bind(server_sock, (SOCKADDR*)&address, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
        {
            std::cerr << "error : couldn't bind sock " << server_sock << " to port " << port << "!" << std::endl;
            return _release();
        }

        // listen for clinet
        if (listen(server_sock, 10) == SOCKET_ERROR)
        {
            std::cerr << "error : couldn't listen on sock " << server_sock << " on port " << port << " !" << std::endl;
            return _release();
        }

        // initial select fd set
        FD_ZERO(&masterfds);

        // put server fd in select set
        FD_SET(server_sock, &masterfds);

        // set maxfd for loop all socket fd
        maxfd = server_sock;

        return true;
    }

    // ===============================================================================================================================
    // ===================================================== Write Function ==========================================================
    // ===============================================================================================================================

    // send message to client
    //
    // @send_sock: client fd
    // @message: string of message
    // @message_len: message length
    // 
    // @return: message length server already send
    //
    int _write(int send_sock, std::string message, int message_len)
    {
        int send_len = 0;

        send_len = send(send_sock, message.c_str(), message_len, 0);

        return send_len;
    }

    // send frame to client
    //
    // @send_sock: client fd
    // @frame: frame data
    // @frame_len: frame data length
    // 
    // @return: frame data length server already send
    //
    int _write_frame(int &send_sock, std::vector<unsigned char> frame, int frame_len)
    {
        int send_len = 0;

        send_len = send(send_sock, frame.data(), frame_len, 0);

        return send_len;
    }

    // send data length to client, then client known how many data it need to receive
    //
    // @send_sock: client fd
    // @data_len: data length server need to send after
    // 
    // @return: data_len size (int is 4 bytes)
    //
    int _write_len(int send_sock, int data_len)
    {
        int send_len = 0;  

        send_len = send(send_sock, &data_len, sizeof(int), 0);

        return send_len;
    }

    // ===============================================================================================================================
    // ===================================================== Read Function ===========================================================
    // ===============================================================================================================================

    // receive message from client
    //
    // @receive_sock: client fd
    // @message_len: message length
    //
    // @return: message
    //
    std::string _read(int receive_sock, int message_len)
    {
        std::string message; // declare message string
        message.clear(); // initial message string

        int receive_len = 0; // message length already receive
        int rest_len = message_len; // message server need to receive

        // receive message buffer, "......\0"
        // +1 is for big data (4096), because it needs '\0' to decide string end
        char buffer[BUFFER_MAX+1];
        memset(buffer, '\0', sizeof(buffer)); // initial message buffer with '\0'

        //std::cout<<"should receive: "<<message_len<<std::endl;

        // BUFFER_MAX is define in the socket_header.h
        if(message_len <= BUFFER_MAX)
        {
            // if message length is not larger than buffer size, it can just recevie one time
            
            // receive message into message buffer
            receive_len = recv(receive_sock, buffer, message_len, 0);
            // add message
            message += buffer;
        }
        else
        {
            // if message length is larger than buffer size, it need to recevie many time until less than buffer size

            // if message length server need to receive larger than zero, server need to receive again and again
            while(rest_len > 0)
            {
                if(rest_len <= BUFFER_MAX)
                {
                    // if message length is not larger than buffer size

                    // receive message into message buffer
                    receive_len += recv(receive_sock, buffer, rest_len, 0);
                    // update message length server need to receive
                    rest_len = message_len - receive_len;
                }
                else
                {
                    // if message length is larger than buffer size

                    // receive message into message buffer
                    receive_len += recv(receive_sock, buffer, BUFFER_MAX, 0);
                    // update message length server need to receive
                    rest_len = message_len - receive_len;
                }

                // add message
                message += buffer;
                // clear message buffer for receive new message
                memset(buffer, '\0', sizeof(buffer));

                //std::cout<<rest_len<<std::endl;
            }
        }

        //std::cout<<message<<" ("<<message.size()<<")"<<std::endl;

        return message;
    }

    // receive frame data form client
    //
    // @receive_sock: client fd
    // @frame_len: frame data length
    //
    // @return: frame data
    //
    std::vector<unsigned char> _read_frame(int receive_sock, int frame_len)
    {
        std::vector<unsigned char> frame; // declare frame data vector
        frame.clear(); // initial frame data vector

        int receive_len = 0; // frame data length already receive
        int rest_len = frame_len; // frame data length server need to receive

        // receive frame data buffer, "......\0"
        // +1 is for big data (4096), because it needs '\0' to decide string end
        unsigned char buffer[BUFFER_MAX+1]; 
        memset(buffer, '\0', sizeof(buffer)); // initial frame data buffer with '\0'

        //std::cout<<"should receive: "<<frame_len<<std::endl;

        // BUFFER_MAX is define in the socket_header.h
        if(frame_len <= BUFFER_MAX)
        {
            // if frame data length is not larger than buffer size, it can just recevie one time
            
            // receive frame in to frame data buffer
            receive_len = recv(receive_sock, buffer, frame_len, 0);
            // put all frame data in to frame data vector
            frame.insert(frame.end(), buffer, buffer + frame_len);
        }
        else
        {
            // if frame data length is larger than buffer size, it need to recevie many time until less than buffer size

            // if frame data length server need to receive larger than zero, server need to receive again and again
            while(rest_len > 0)
            {
                if(rest_len <= BUFFER_MAX)
                {
                    // if frame data length is not larger than buffer size

                    // receive frame into frame data buffer
                    receive_len = recv(receive_sock, buffer, rest_len, 0);
                    // update frame data length server need to receive
                    rest_len -= receive_len;
                }
                else
                {
                    // if frame data length is larger than buffer size

                    // receive frame into frame data buffer
                    receive_len = recv(receive_sock, buffer, BUFFER_MAX, 0);
                    // update frame data length server need to receive
                    rest_len -= receive_len;
                }

                // put all frame data in to frame data vector
                frame.insert(frame.end(), buffer, buffer + receive_len);
                // clear frame data  buffer for receive new frame data
                memset(buffer, '\0', sizeof(buffer));

                //std::cout<<frame.size()<<std::endl;
                //std::cout<<rest_len<<std::endl;
                //std::cout<<receive_len<<std::endl;
            }
        }

        //std::cout<<message<<" ("<<message.size()<<")"<<std::endl;

        // return 
        return frame;
    }

    // receive data length from client, then server known how many data it need to receive 
    // 
    // @receive_sock: client fd
    //
    // @return: data length
    //
    int _read_len(int receive_sock)
    {
        int data_len = 0;
        recv(receive_sock, &data_len, sizeof(int), 0);

        return data_len;
    }

public:

    // ServerSocket constructor
    //
    // @_port: port number
    // @_timeout: timeout for select() timeval
    // @_quality: jpeg compression [1..100] for cvimencode, not use in here
    //
    ServerSocket(int _port = 0, int _timeout = 200000, int _quality = 30)
        : server_port(_port)
        , server_sock(INVALID_SOCKET)
        , timeout(_timeout)
        , quality(_quality)
    {
        signal(SIGPIPE, SIG_IGN); // ignore ISGPIP to avoid client crash lead server force to stop
        FD_ZERO(&masterfds); // initial readfds, set readfds all zero

        if(server_port) _open(server_port); // if port > 0, then create a server with port
    }

    // ServerSocket destructor
    ~ServerSocket()
    {
        _release();
    }

    // check whether server is opened or not
    bool isOpened()
    {
        return server_sock != INVALID_SOCKET;
    }

    // ===============================================================================================================================
    // ===================================================== Start Function ==========================================================
    // ===============================================================================================================================

    // start to receive frame data
    //
    // @frame: put all frame data into frame vector
    //
    bool start_frame(std::vector<unsigned char> &frame)
    {
        //std::cout<<"waiting..."<<std::endl;

        SELECT_SET readfds = masterfds; // copy masterfds to readfds

        struct timeval tv = { 0, timeout }; // set timeout

        // on client want to access
        if (select(maxfd + 1, &readfds, NULL, NULL, &tv) <= 0) return true;
        
        // select() loop
        for (int sock = 0; sock <= maxfd; sock++)
        {
            // check whether socket fd is active or not
            if (!FD_ISSET(sock, &readfds))
                continue;
            
            // if socket fd is equal to server socket fd, server known some client want to connect
            if (sock == server_sock)
            {
                SOCKADDR_IN address = {0}; // client address information
                SOCKLEN_T addrlen = sizeof(SOCKADDR); // addrlen size

                SOCKET client_sock = accept(server_sock, (SOCKADDR*)&address, &addrlen); // accept client connection

                // check whether client connection is successful or not
                if (client_sock == SOCKET_ERROR)
                {
                    std::cerr << "error : couldn't accept connection on server sock " << server_sock << " !" << std::endl;
                    return false;
                }

                std::string client_ip = inet_ntoa(address.sin_addr); // get client ip address

                maxfd = ( maxfd > client_sock ? maxfd : client_sock ); // reset maxfd with the larger one

                FD_SET(client_sock, &masterfds); // insert client fd to masterfds

                std::cerr << "new client " << client_sock << " : " << client_ip << std::endl;
            }
            else // s is client
            {
                // receive frame data length for receive all frame data completely
                int frame_len = 0;
                frame_len = _read_len(sock);

                //std::cout<<sock<<" receive: "<<frame_len<<std::endl;

                // check whether frame data length is zero or not
                if(frame_len > 0)
                {
                    // frame data length is not zero, begin to receive frame data
                    frame = _read_frame(sock, frame_len);

                    //std::cout<<frame.size()<<std::endl;
                }
                else
                {
                    // frame data length is zero, have to check whether client is closed or not
                    // send some data (any thing is ok) to client
                    int n = _write_len(sock, frame_len);

                    // if n is equal to -1 (less than zero), it is mean client is closed
                    if (n < 0)
                    {
                        std::cerr << "kill client " << sock << std::endl;

                        // shutdown client and remove client fd from masterfds
                        shutdown(sock, 2);
                        FD_CLR(sock, &masterfds);
                    }
                }
            }
        }
        return true;
    }

    // start to get frame data and frame stamp from client
    //
    // @frame : put frame data which receive from client
    // @frame_stamp : put frame stamp which receive from client
    //
    bool start_frame_with_stamp(std::vector<unsigned char> &frame, int &frame_stamp)
    {
        //std::cout<<"waiting..."<<std::endl;

        SELECT_SET readfds = masterfds; // copy masterfds to readfds

        struct timeval tv = { 0, timeout }; // set timeout

        // on client want to access
        if (select(maxfd + 1, &readfds, NULL, NULL, &tv) <= 0) return true;
        
        // select() loop
        for (int sock = 0; sock <= maxfd; sock++)
        {
            // check whether socket fd is active or not
            if (!FD_ISSET(sock, &readfds))
                continue;
            
            // if socket fd is equal to server socket fd, server known some client want to connect
            if (sock == server_sock)
            {
                SOCKADDR_IN address = {0}; // client address information
                SOCKLEN_T addrlen = sizeof(SOCKADDR); // addrlen size

                SOCKET client_sock = accept(server_sock, (SOCKADDR*)&address, &addrlen); // accept client connection

                // check whether client connection is successful or not
                if (client_sock == SOCKET_ERROR)
                {
                    std::cerr << "error : couldn't accept connection on server sock " << server_sock << " !" << std::endl;
                    return false;
                }

                std::string client_ip = inet_ntoa(address.sin_addr); // get client ip address

                maxfd = (maxfd > client_sock ? maxfd : client_sock); // reset maxfd with the larger one

                FD_SET(client_sock, &masterfds); // insert client fd to masterfds

                std::cerr << "new client " << client_sock << " : " << client_ip << std::endl;
            }
            else // s is client
            {
                // receive frame stamp
                frame_stamp = _read_len(sock);

                // receive frame data length for receive all frame data completely
                int frame_len = 0;
                frame_len = _read_len(sock);

                //std::cout<<"frame_stamp : "<<frame_stamp<<std::endl;
                //std::cout<<sock<<" receive: "<<frame_len<<std::endl;

                // check whether frame data length is zero or not
                if(frame_len > 0)
                {
                    // frame data length is not zero, begin to receive frame data
                    frame = _read_frame(sock, frame_len);

                    //std::cout<<frame.size()<<std::endl;
                }
                else
                {
                    // frame data length is zero, have to check whether client is closed or not
                    // send some data (any thing is ok) to client
                    int n = _write_len(sock, frame_len);

                    // if n is equal to -1 (less than zero), it is mean client is closed
                    if (n < 0)
                    {
                        std::cerr << "kill client " << sock << std::endl;

                        // shutdown client and remove client fd from masterfds
                        shutdown(sock, 2);
                        FD_CLR(sock, &masterfds);
                    }
                }
            }
        }
        return true;
    }
};


// ===============================================================================================================================
// =============================================== Function Call for Server ======================================================
// ===============================================================================================================================


// receive frame data
//
// @port : server port number
// @timeout : for server to use select
// @quality : jpeg quality, not use in here
//
// @reture : message from client
//
/*std::string receive_message(int port, int timeout, int quality)
{
    static ServerSocket server_socket(port, timeout, quality);

    server_socket.start();

    return "";
}*/

// receive frame data and frame stamp
//
// @frame : put frame data which receive from client
// @port : server port number
// @timeout : for server to use select
// @quality : jpeg quality, not use in here
//
// @reture : frame stamp
//
int receive_frame_with_stamp(std::vector<unsigned char> &frame, int port, int timeout, int quality)
{
    static ServerSocket server_socket(port, timeout, quality);

    int framp_stamp = 0;

    server_socket.start_frame_with_stamp(frame, framp_stamp);

    return framp_stamp;
}

// receive frame data
//
// @port : server port number
// @timeout : for server to use select
// @quality : jpeg quality, not use in here
//
// @reture : frame data from client
//
std::vector<unsigned char> receive_frame(int port, int timeout, int quality)
{
    static ServerSocket server_socket(port, timeout, quality);

    std::vector<unsigned char> frame;

    server_socket.start_frame(frame);

    return frame;
}