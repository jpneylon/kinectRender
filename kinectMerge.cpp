/*
kinectMerge
UCLA Radiation Oncology
Yugang Min
ymin@mednet.ucla.edu
August-October, 2014

It is a cleanup version of multiKinectTracker_PCL_2013, without openFrameworks.
1. Run multiple cameras
2. Add depth smoothing processing
3. Add outlier removal
4. Add keyboard callback
5. Test OpenCV support
*/

#define COMMAND_ACK  3

#define DEPTHHEIGHT 424//398 //380 //398 //424
#define DEPTHWIDTH 512

#include <helper_cuda.h>
#include <helper_math.h>

#include <sys/socket.h>
#include <netinet/in.h>

#define portnum 51717
#define buffersize DEPTHHEIGHT*DEPTHWIDTH*3
#define imagesize DEPTHHEIGHT*DEPTHWIDTH*3


void error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int kinectSocket()
{
    bool continueOperation = true;
    int tcp_instruction[2];		// [0]: instruction type [1]: data size
    int datasize;

	int sockfd, newsockfd, portno, length;
	socklen_t clilen;
	float *bufferptr;
	float *buffer = (float *)malloc(buffersize*sizeof(float));
	struct sockaddr_in server_addr, client_addr;
	int n;
	unsigned char *rgbbufferptr;
	unsigned char *rgbbuffer = (unsigned char *)malloc(buffersize);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");


	length = sizeof(server_addr);
	bzero(&server_addr, length);
	portno = portnum; //atoi(argv[1]);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &server_addr, length) < 0)
		error("ERROR on binding");


    listen(sockfd,5);
    clilen = sizeof(client_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");
    else
        printf("Camera is connected.\n");

	bufferptr = buffer;
	rgbbufferptr = rgbbuffer;

	float *s;
	unsigned char *t;

	float dataPack[buffersize];
	unsigned char dataPackColor[buffersize];

    /* Loop only a specified number of times for this example.       */
    while(continueOperation)
    {

        tcp_instruction[0] = COMMAND_ACK;
        tcp_instruction[1] = 0;
        n = write(newsockfd,tcp_instruction,sizeof(int)*2);
        if (n < 0) error("ERROR writing to socket");

        // TCP
        // receive depth data
        n = read(newsockfd, tcp_instruction, sizeof(int)*2);
        datasize = tcp_instruction[1];
        bzero(bufferptr,datasize*sizeof(float));
        int bytes_read = 0;
        while (bytes_read < datasize*sizeof(float) &&(n = read(newsockfd,bufferptr+bytes_read/sizeof(float), datasize*sizeof(float)-bytes_read)) > 0)
        {
            bytes_read += n;
        }
        if (n <= 0)
        {
            close(sockfd);
            close(newsockfd);
            error("ERROR reading from socket");
        }
        //else
        //    std::cout << "received " << bytes_read << " bytes." << std::endl;

        // receive rgb data
        n = read(newsockfd, tcp_instruction, sizeof(int)*2);
        datasize = tcp_instruction[1];
        bzero(rgbbufferptr,datasize);
        bytes_read = 0;
        while (bytes_read < datasize &&(n = read(newsockfd,rgbbufferptr+bytes_read, datasize-bytes_read)) > 0)
        {
            bytes_read += n;
        }
        if (n <= 0)
        {
            close(sockfd);
            close(newsockfd);
            error("ERROR reading from socket");
        }

        s = reinterpret_cast<float *>(bufferptr);
        t = reinterpret_cast<unsigned char *>(rgbbufferptr);

        for (int i = 0; i < datasize; i++)
        {
                dataPack[i] = *(s+i);
                dataPackColor[i] = *(t+i);
        }

        int numItem = datasize / 3;

        for (int idx = 0; idx < numItem; idx++)
        {
            float3 pt;
            pt.x = dataPack[idx*3];
            pt.y = dataPack[idx*3+1];
            pt.z = dataPack[idx*3+2];

            float3 color;
            color.x = dataPackColor[idx*3+2];
            color.y = dataPackColor[idx*3+1];
            color.z = dataPackColor[idx*3];
        }
	}	/* End of WHILE LOOP */

	// clean up socket
	close(sockfd);
    close(newsockfd);

	return 0;
}

