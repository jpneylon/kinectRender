#ifndef __SOCKET_CLASS_H__
#define __SOCKET_CLASS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <time.h>

#define COMMAND_ACK             3
#define MESSAGE_SIZE            32
#define MAX_DATA_PACKET_SIZE    16384
#define PORTNO_BASE             51717


class CS_Socket
{
    public:
        CS_Socket()
        {
            message_size            = MESSAGE_SIZE;
            max_data_packet_size    = MAX_DATA_PACKET_SIZE;
            portno                  = PORTNO_BASE;
        }
        ~CS_Socket(){};

        void set_port_number( unsigned int n )
        {
            portno = PORTNO_BASE + n;
        }
        unsigned int get_port_number()
        {
            return portno;
        }
        void set_maximum_data_packet_size( unsigned int n )
        {
            max_data_packet_size = n;
        }
        unsigned int get_maximum_data_packet_size()
        {
            return max_data_packet_size;
        }
        void set_message_size( unsigned int n )
        {
            message_size = n;
        }
        unsigned int get_message_size()
        {
            return message_size;
        }
        void error(const char *msg)
        {
            perror(msg);
            exit(EXIT_FAILURE);
        }


        void open_server_socket()
        {
             sockfd = socket(AF_INET, SOCK_STREAM, 0);
             if (sockfd < 0)
                error("ERROR opening socket");
             bzero(&serv_addr, sizeof(serv_addr));

             serv_addr.sin_family = AF_INET;
             serv_addr.sin_addr.s_addr = INADDR_ANY;
             serv_addr.sin_port = htons(portno);
             if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
                      error("ERROR on binding");

            if (listen(sockfd,5) < 0)
                error("ERROR upon listen");
        }
        void listen_server_socket()
        {
             printf("\n Waiting for client...\n");

             struct sockaddr_in cli_addr;
             socklen_t clilen = sizeof(cli_addr);

             newsockfd = accept(sockfd,
                         (struct sockaddr *) &cli_addr,
                         &clilen);
             if (newsockfd < 0)
             {
                printf("\n newsockfd = %d\n",newsockfd);
                error("ERROR upon accept");
             }

             time_t rawtime;
             struct tm * timeinfo;
             char buffer[64];
             time (&rawtime);
             timeinfo = localtime (&rawtime);
             strftime (buffer,32,"%m.%d.%y-%H.%M.%S",timeinfo);
             printf("\n %s : connection established \n",buffer);
        }

        bool check_for_available_data()
        {
            int bytes_avail = 0;
            ioctl( newsockfd, FIONREAD, &bytes_avail );
            if (bytes_avail > 0)
                return true;
            else
                return false;
        }

        void close_socket()
        {
             close(sockfd);
        }
        void close_new_socket()
        {
             close(newsockfd);
        }


        void send_message_server_socket( char *message )
        {
            n = write( newsockfd, message, message_size);
            if (n < 0)
                 error("ERROR writing to server socket");
            printf("\n Send To Client: %s", message);
        }
        void receive_message_server_socket( char *message )
        {
             n = read( newsockfd, message, message_size );
             if (n < 0)
                error("ERROR reading from socket");
             printf("\n Received from Client: %s", message);
        }


        template <typename T> void write_server_socket( T *data, unsigned int size_data )
        {
            int n = write( newsockfd, data, size_data );
            if (n < 0) error("ERROR writing to socket");
        }
        template <typename T> void read_server_socket( T *data, unsigned int size_data )
        {
            int n = read( newsockfd, data, size_data );
            if (n < 0) error("ERROR reading from socket");
        }

        template <typename T> void send_data_server_socket( T *data, unsigned int size_data )
        {
            T *buffer = new T[max_data_packet_size/sizeof(T)];
            unsigned int remaining_bytes = size_data * sizeof(T);
            unsigned int loops = 0;
            unsigned int bytes = 0;

            while(remaining_bytes > 0)
            {
                if (remaining_bytes > max_data_packet_size)
                {
                    memcpy(buffer, data + (bytes/sizeof(T)), max_data_packet_size);
                    n = write( newsockfd, (const void *)buffer, max_data_packet_size );
                    if (n < 0)
                        error("ERROR writing to socket");
                    remaining_bytes -= n;
                    bytes += n;
                    if (n < max_data_packet_size)
                        printf("\n %d Copy - %d bytes sent | %u bytes remaining", loops, n, remaining_bytes);
                }
                else
                {
                    memcpy(buffer, data + (bytes/sizeof(T)), remaining_bytes);
                    n = write( newsockfd, (const void *)buffer, remaining_bytes );
                    if (n < 0)
                        error("ERROR writing to socket");
                    bytes += n;
                    remaining_bytes -= n;
                    if (n < max_data_packet_size)
                        printf("\n %d Copy - %d bytes sent | %u bytes remaining", loops, n, remaining_bytes);
                }
                loops++;
            }
            printf("\n %d Copies - %u Bytes Sent.\n",loops,bytes);
            delete [] buffer;
        }
        template <typename T> void receive_data_server_socket( T *data, unsigned int size_data )
        {
            T *buffer = new T[max_data_packet_size/sizeof(T)];
            unsigned int remaining_bytes = size_data * sizeof(T);
            unsigned int loops = 0;
            unsigned int bytes = 0;

            while(remaining_bytes > 0)
            {
                if (remaining_bytes > max_data_packet_size)
                {
                    n = read( newsockfd, (void *)buffer, max_data_packet_size );
                    if (n < 0)
                        error("ERROR reading from socket");
                    memcpy(data + (bytes/sizeof(T)), buffer, max_data_packet_size);
                    remaining_bytes -= n;
                    bytes += n;
                }
                else
                {
                    n = read( newsockfd, (void *)buffer, remaining_bytes );
                    memcpy(data + (bytes/sizeof(T)), buffer, remaining_bytes);
                    if (n < 0)
                        error("ERROR reading from socket");
                    bytes += n;
                    remaining_bytes -= n;
                }
                loops++;
            }
            printf("\n %lu Copies - %lu Bytes Received.\n",loops,bytes);
            delete [] buffer;
        }


    protected:
        int n;
        int sockfd;
        int newsockfd;
        unsigned int portno;

        struct sockaddr_in cli_addr;
        socklen_t clilen;

        struct sockaddr_in serv_addr;
        struct hostent *server;

        unsigned int message_size;
        unsigned int max_data_packet_size;
};

#endif // __SOCKET_CLASS_H__
