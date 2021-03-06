// ==============================
// By Jimmy
//
// 2018/12/14
//
// 1. sort out code
// ==============================

#include "mjpeg_streaming.h"

class MJPGWriter
{
    SOCKET sock;
    SOCKET maxfd; // record the max value of sock for select() loop
    PORT port;

    fd_set masterfds;
    int timeout; // readfds sock timeout, shutdown after timeout millis.
    int quality; // jpeg compression [1..100]

    // sock: client socket
    // s: constant pointer to a constant char
    // len: length of s
    int _write(int sock, char const * const s, int len)
    {
        if (len < 1)
        {
            len = strlen(s);
        }
        return ::send(sock, s, len, 0);
    }

public:

    // constructor
    // _port: port number
    // _timeout: timeout for select() timeval
    // _quality: jpeg compression [1..100] for cv::imencode (the higher is the better)
    MJPGWriter(int _port = 0, int _timeout = 200000, int _quality = 30)
        : port(_port)
        , sock(INVALID_SOCKET)
        , timeout(_timeout)
        , quality(_quality)
    {
        signal(SIGPIPE, SIG_IGN); // ignore ISGPIP to avoid client crash and server is forcde to stop
        FD_ZERO(&masterfds); // set readfds all zero

        if(port) open(port); // if port > 0, then create a server with port
    }

    // destructor
    ~MJPGWriter()
    {
        release();
    }

    bool release()
    {
        if (sock != INVALID_SOCKET)
            ::shutdown(sock, 2); // disable receive or send data, like close()
        sock = (INVALID_SOCKET);
        return false;
    }


    bool open(int port)
    {
        sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        SOCKADDR_IN address; // server address information
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_family = AF_INET;
        address.sin_port = htons(port);

        int optval = 1; // set SO_REUSEADDR(1) is true
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

        if (::bind(sock, (SOCKADDR*)&address, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
        {
            std::cerr << "error : couldn't bind sock " << sock << " to port " << port << "!" << std::endl;
            return release();
        }

        if (::listen(sock, 10) == SOCKET_ERROR)
        {
            std::cerr << "error : couldn't listen on sock " << sock << " on port " << port << " !" << std::endl;
            return release();
        }

        FD_ZERO(&masterfds);
        FD_SET(sock, &masterfds);

        maxfd = sock;

        return true;
    }

    bool isOpened()
    {
        return sock != INVALID_SOCKET;
    }

    bool write(const cv::Mat & frame)
    {
        fd_set readfds = masterfds;

        struct timeval tv = { 0, timeout };

        // nothing broken, there's just noone listening
        if (::select(maxfd + 1, &readfds, NULL, NULL, &tv) <= 0) return true;

        std::vector<uchar> outbuf;
        
        std::vector<int> compression_params;
        compression_params.push_back(cv::IMWRITE_JPEG_QUALITY); // default quality value is 95
        compression_params.push_back(quality);

        cv::imencode(".jpg", frame, outbuf, compression_params); // encodes an image into a memory buffer
        
        size_t outlen = outbuf.size();

        for (int s = 0; s <= maxfd; s++)
        {
            SOCKLEN_T addrlen = sizeof(SOCKADDR);

            // check whether s is active
            if (!FD_ISSET(s, &readfds))
                continue;
            
            // if s is equal to sock, s is server socket which need to accept and send http header
            if (s == sock)
            {
                SOCKADDR_IN address = {0}; // client address information
                

                SOCKET client = ::accept(sock, (SOCKADDR*)&address, &addrlen);

                if (client == SOCKET_ERROR)
                {
                    std::cerr << "error : couldn't accept connection on sock " << sock << " !" << std::endl;
                    return false;
                }

                char *ip = inet_ntoa(address.sin_addr);

                maxfd = ( maxfd > client ? maxfd : client ); // reset maxfd with the larger one

                FD_SET(client, &masterfds); // insert client fd to masterfds

                // send http header
                _write(client, "HTTP/2.0 200 OK\r\n", 0);
                _write(client,
                "Server: C++\r\n"
                "Accept-Range: bytes\r\n"
                "Connection: close\r\n"
                "Max-Age: 0\r\n"
                "Expires: 0\r\n"
                "Cache-Control: no-cache, private\r\n"
                "Pragma: no-cache\r\n"
                "Content-Type: multipart/x-mixed-replace; boundary=mjpegstream\r\n"
                "\r\n", 0);

                std::cerr << "new client " << client << " : " << ip << std::endl;
            }
            else // s is client
            {
                char head[400];

                sprintf(head, "--mjpegstream\r\nContent-Type: image/jpeg\r\nContent-Length: %zu\r\n\r\n", outlen);

                _write(s, head, 0);

                int n = _write(s, (char*)(&outbuf[0]), outlen);

                // std::cerr << " MJPEG-stream sent to ";
                std::cerr << "known client " << s << " " << n << std::endl;

                if (n < 0) // client close or crash
                {
                    std::cerr << "kill client " << s << std::endl;
                    ::shutdown(s, 2);
                    FD_CLR(s, &masterfds);
                }
            }
        }
        return true;
    }
};

void send_mjpeg(cv::Mat im, int port, int timeout, int quality) {
    
    // create only one MJPGWriter object
    static MJPGWriter mjpeg_writer(port, timeout, quality);

    cv::Mat mat;

    //cv::resize(image_to_mat(im), mat, cv::Size(1352, 1013));

    mjpeg_writer.write(im);
}

void send_mjpeg(image im, int port, int timeout, int quality) {
    
    // create only one MJPGWriter object
    static MJPGWriter mjpeg_writer(port, timeout, quality);

    cv::Mat mat;

    cv::resize(image_to_mat(im), mat, cv::Size(1352, 1013));

    mjpeg_writer.write(mat);
}
