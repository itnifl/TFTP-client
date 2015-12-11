#include "tftp.h"

//Original code from https://github.com/lanrat/tftp - thank you lanrat!
//Functions here are rewritten to avoid use of hard coded hostnames or addresses - Atle Holm (atle@team-holm.net) - September 2015
//Rewrote the getFile function so that it will not empty a file that already exists on local disk when the retrieval of it fails - Atle Holm (atle@team-holm.net)- December 2015

//returns a struct for the server information
struct sockaddr_in getServerStruct(char *host, int port)
{
  struct sockaddr_in serv_addr;

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;

  serv_addr.sin_addr.s_addr = inet_addr(host);
  serv_addr.sin_port = htons(port);

  return serv_addr;
}


//called when we retrieve a file from the server
void getFile(char *host, int port, char *filename)
{
  int sockfd;
  struct sockaddr_in serv_addr;
  FILE * file;
  char *buffer;  
  if (fileExists(filename)) {
	  printf("Information: The file %s exists already, reading it into memory to preserve its state in case retreival of new file fails.\n", filename);
	  file = fopen(filename, "rb");	  
	  if (file != NULL)
	  {
		  fseek(file, 0L, SEEK_END);
		  long s = ftell(file);
		  rewind(file);
		  buffer = malloc(s);
		  if (buffer != NULL)
		  {	  //Now after this, the buffer contains our file contents, and we will use it later if we need to:
			  fread(buffer, s, 1, file);
		  }
	  }
	  fclose(file);
  }

  if (strchr(filename,'/') != NULL )
  {
    printf("We do not support file transfer out of the current working directory\n");
    return;
  }

  sockfd = createUDPSocketAndBind(0);
  serv_addr = getServerStruct(host, port);

  if(sockfd < 0)
  {
    printf("Couldn't open socket\n");
    return;
  }

  if(!send_RRQ(sockfd, (struct sockaddr *) &serv_addr, filename, TFTP_SUPORTED_MODE))
  {
    printf("Error: couldn't send RRQ\n");
    return;
  }
  
  file = fopen(filename, "wb");

  if (file == NULL)
  {
	  perror(filename);
	  return;
  }

  if(!recvFile(sockfd, (struct sockaddr *) &serv_addr, file,filename))
  {
    printf("Error: didn't receive file\n");
	fclose(file);

	printf("Information: Writing preserved file contents of file %s to disk, since we were not able to retrieve a new file from the server.\n", filename);
	file = fopen(filename, "w");
	if (file == NULL)
	{
		printf("Error opening file!\n");
		exit(1);
	}
	//We did not receive a new file, so lets put all the text that was in, it back into it:
	fprintf(file, buffer);


	fclose(file);
    return;
  }
  fclose(file);
  return;
}

int fileExists(char *fname)
{
	FILE *file;
	if (file = fopen(fname, "r"))
	{
		fclose(file);
		return 1;
	}
	return 0;
}

//used to upload files to the server
void putFile(char *host, int port, char *filename)
{
  int sockfd;
  struct sockaddr_in serv_addr;
  PACKET packet;
  int result;
  FILE * fileh;
  int timeout_counter = 0;
  
  if (strchr(filename,'/') != NULL )
  {
    printf("We do not support file transfer out of the current working directory\n");
    return;
  }

  
  fileh = fopen(filename, "rb");

  if(fileh == NULL)
  {
    perror(filename);
    return;
  }

  sockfd = createUDPSocketAndBind(0);
  serv_addr = getServerStruct(host, port);

  if(sockfd < 0)
  {
    printf("Couldn't open socket\n");
    return;
  }

  if(!send_WRQ(sockfd, (struct sockaddr *) &serv_addr, filename, TFTP_SUPORTED_MODE))
  {
    printf("Error: couldn't send WRQ to server\n");
    return;
  }
  while (timeout_counter < MAX_TFTP_TIMEOUTS)
  {
    result = waitForPacket(sockfd, (struct sockaddr *) &serv_addr, TFTP_OPTCODE_ACK, &packet);
    if (result < 0)
    {
      printf("Error: Timeout sending packet to server\n");
      if(!send_WRQ(sockfd, (struct sockaddr *) &serv_addr, filename, TFTP_SUPORTED_MODE))
      {
        printf("Error: couldn't send WRQ to server\n");
        return;
      }
      timeout_counter++;
    }else
    {
      break;
    }
  }
  if (result < 0)
  {
    //we still timed out
    printf("Timed out after %d tries, is the server running\n",MAX_TFTP_TIMEOUTS);
    fclose(fileh);
    return;
  }
  if (packet.optcode == TFTP_OPTCODE_ERR)
  {
    //we recieved an error, print it
    printError(&packet);
  }else
  {
    if (!sendFile(sockfd, (struct sockaddr *) &serv_addr, fileh))
    {
      printf("Unable to send file to server\n");
    }
  }
  fclose(fileh);
  return;
}


//main client, checks for args and starts an operation if no errors detected
int main(int argc, char *argv[])
{
  int port = SERV_UDP_PORT;
  char *host = SERV_HOST_ADDR;
  bool writeFile = false;
  bool readFile = false;
  int argOffset = 1;
  char* filename;
  int x = 1;

  while (x < argc) {	  
	  if (argv[x][0] == '-') {
		  //printf("Entered at Checking: %c\n", argv[x][1]);
		  //printf("Variable is: %s\n", argv[x + 1]);
		  switch (argv[x][1]) {
		  case 'p':
			  port = atoi(argv[x + 1]);
			  break;
		  case 'h':
			  host = argv[x + 1];
			  break;
		  case 'w':
			  writeFile = true;
			  filename = argv[x + 1];
			  break;
		  case 'r':
			  readFile = true;
			  filename = argv[x + 1];
			  break;
		  default:
			  break;
		  }
	  }	  
	  x++;
  }
  //printf("After loop\n");
  //printf("Host: %s\n", host);
  //printf("Port: %i\n", port);
  //printf("Filename: %s\n", filename);
  if (writeFile == true) {
	  putFile(host, port, filename);
  }
  if (readFile == true) {
	  getFile(host, port, filename);
  }
  if (writeFile == false && readFile == false) {
	  printf("Usage: %s [-h host] [-p port]  (-w putfile || -r getFile)\n", argv[0]);
  }
}
