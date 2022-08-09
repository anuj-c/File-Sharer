#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#define ADDR "10.10.68.121"
using namespace std;

/* #region colors */
const char Black[] = "\033[0;30m";
const char Red[] = "\033[0;31m";
const char Green[] = "\033[0;32m";
const char Yellow[] = "\033[0;33m";
const char Blue[] = "\033[0;34m";
const char Purple[] = "\033[0;35m";
const char Cyan[] = "\033[0;36m";
const char White[] = "\033[0;37m";
const char Reset[] = "\033[0m";
void black() { cout << Black; }
void red() { cout << Red; }
void green() { cout << Green; }
void yellow() { cout << Yellow; }
void blue() { cout << Blue; }
void purple() { cout << Purple; }
void cyan() { cout << Cyan; }
void white() { cout << White; }
void reset() { cout << Reset; }
/* #endregion */
bool isClosed = false;
int mode = 1;

void parseFile(string &fname, long &size, char *file)
{
  for (int i = 0; i < strlen(file); i++)
  {
    if (file[i] == ':')
    {
      fname += '\0';
      i++;
      size = atoi(&file[i]);
      return;
    }
    fname += file[i];
  }
}

char *getSize(long size)
{
  char buf[50];
  memset(buf, 0, sizeof(buf));
  if (size <= 1024)
  {
    sprintf(buf, "%ld B", size);
  }
  else if (size <= 1024 * 1000)
  {
    sprintf(buf, "%.2f KB", (float)size / 1000);
  }
  else if (size <= 1024 * 1000 * 1000)
  {
    sprintf(buf, "%.2f MB", (float)size / (1000 * 1000));
  }
  else
  {
    sprintf(buf, "%.2f GB", (float)size / (1000 * 1000 * 1000));
  }
  return strdup(buf);
}

int receive(char *buf, int sockfd)
{
  int n;
  n = recv(sockfd, buf, 1024, 0);
  if (n == -1)
  {
    perror("recv: ");
  }
  else if (n == 0)
  {
    cout << "Client disconnected" << endl;
  }
  buf[n] = '\0';
  return n;
}

int receiveFile(char *buf, int sockfd)
{
  FILE *fp;
  int n;
  string fname;
  long size;
  parseFile(fname, size, buf);
  cout << endl;
  purple();
  printf("File name: %s\nFile size: %s\n\n", fname.c_str(), getSize(size));
  reset();

  if (mode)
    fp = fopen(fname.c_str(), "wb");
  else
    fp = fopen(fname.c_str(), "w");

  if (fp == NULL)
  {
    perror("fopen: ");
    red();
    cout << "Could not receive file. Please try again!" << endl;
    reset();
  }
  else
  {
    green();
    cout << "Receiving...\n"
         << endl;
    reset();
    char ch[1024];
    long onepart = size / 100;
    int j = 1;
    // RECEIVING THE FILE
    cout << "---------------" << endl;
    for (long i = 0; i < size;)
    {
      n = receive(ch, sockfd);
      if (n == -1 || n == 0)
      {
        remove(fname.c_str());
        return -1;
      }

      if (strcmp(ch, "EOF") == 0)
        break;

      i += n;

      printf("\r[");
      for (int it = 0; it < 50; it++)
      {
        int cur = (i * 100) / (size * 2);
        if (it <= cur)
        {
          printf("#");
        }
        else
        {
          printf("_");
        }
      }
      printf("] %ld%%", (i * 100) / (size));

      fwrite(ch, sizeof(char), n, fp);
    }
    cout << "\n---------------" << endl;
    green();
    cout << "\nFile received" << endl;
    reset();
    fclose(fp);
  }
  return 0;
}

int sendFile(string buf, int sockfd)
{
  FILE *fp;
  int n;

  // OPENING THE FILE
  if (mode)
    fp = fopen(buf.c_str(), "rb");
  else
    fp = fopen(buf.c_str(), "r");

  if (fp == NULL)
  {
    red();
    cout << "File not found" << Reset << endl;
    n = send(sockfd, "File not found", strlen("File not found"), 0);
    if (n == -1)
    {
      perror("send: ");
      return -1;
    }
  }
  else
  {
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    string msg = buf + ":" + to_string(size);
    n = send(sockfd, msg.c_str(), msg.length(), 0);
    if (n == -1)
    {
      perror("send: ");
      return -1;
    }

    purple();
    printf("\nFile name: %s\nFile size: %s\n", buf.c_str(), getSize(size));
    reset();

    sleep(0.5);
    char ch;
    string fcontent;
    // SENDING THE FILE
    long i = 1;

    cout << Green << "\nSending...\n"
         << Reset << endl;
    cout << "---------------" << endl;
    while (1)
    {
      fcontent.clear();
      for (int i = 0; i < 1024; i++)
      {
        fread(&ch, sizeof(char), 1, fp);
        if (feof(fp))
        {
          break;
        }
        fcontent += ch;
      }

      i += fcontent.length();
      printf("\r[");
      for (int it = 0; it < 50; it++)
      {
        int cur = (i * 100) / (size * 2);
        if (it <= cur)
        {
          printf("#");
        }
        else
        {
          printf("_");
        }
      }
      printf("] %ld%%", (i * 100) / (size));
      // printf("\r%ld%%", (i * 100) / size);

      if (feof(fp))
      {
        if (fcontent.length() > 0)
        {
          n = send(sockfd, fcontent.c_str(), fcontent.length(), 0);
          if (n == -1)
          {
            perror("send: ");
            return -1;
          }
        }
        break;
      }

      n = send(sockfd, fcontent.c_str(), fcontent.length(), 0);
      if (n == -1)
      {
        perror("send: ");
        return -1;
      }
    }
    cout << "\n---------------" << endl;
    sleep(0.5);
    green();
    cout << "\nFile sent" << Reset << endl;
    fclose(fp);
  }
  return 0;
}

void *reading(void *args)
{
  // GETTING SOCKFD
  int sockfd = *(int *)args;

  // RECEIVING WHILE LOOP
  char buf[1024];
  while (1)
  {
    memset(buf, 0, 1024);

    // RECV FUNCTION
    int n = recv(sockfd, buf, 1024, 0);
    if (n == -1)
    {
      perror("recv: ");
      break;
    }
    else if (n == 0)
    {
      cout << "Client disconnected" << endl;
      close(sockfd);
      isClosed = true;
      break;
    }
    buf[n] = '\0';

    // IF FILE FOUND THEN RECEIVE ALL THE FILES
    if (strncmp(buf, "File found", strlen("File found")) == 0)
    {
      n = receiveFile(buf + strlen("File found:"), sockfd);
      if (n == -1)
      {
        red();
        cout << "Error occured while receiving file. Please try after reconnecting!" << Reset << endl;
        isClosed = true;
        break;
      }
    }

    // FOR SENDING FILE
    else if (strncmp(buf, "Ready to receive", strlen("ready to receive")) == 0)
    {
      n = sendFile(buf + strlen("ready to receive "), sockfd);
      if (n == -1)
      {
        red();
        cout << "Error occured while sending file. Please try after reconnecting!" << Reset << endl;
        isClosed = true;
        break;
      }
    }

    // FOR CLOSING CONNECTION
    else if (strcmp(buf, "Closing connection") == 0)
    {
      break;
    }

    // CHANGING MODE
    else if (strncmp(buf, "Mode changed to", strlen("Mode changed to")) == 0)
    {
      printf("\n%sServer :%s %s\n", Blue, Reset, buf);
      if (strcmp(buf + strlen("Mode changed to "), "binary") == 0)
      {
        mode = 1;
      }
      else
      {
        mode = 0;
      }
    }

    else
    {
      // PRINTING RECEIVED MESSAGE
      printf("\n%sServer :%s %s\n", Blue, Reset, buf);
    }

    // PROMPT
    cyan();
    cout << "\n------------------------" << endl;
    yellow();
    cout << "Enter the command:" << Reset << endl;
  }
  pthread_exit(NULL);
  return NULL;
}

int main()
{
  // VARIABLE DECLARATION
  struct sockaddr_in client;
  client.sin_family = AF_INET;
  client.sin_port = htons(3003);
  client.sin_addr.s_addr = inet_addr(ADDR);
  int sockfd, n;

  // SOCKET FD
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1)
  {
    cout << "Error in creating socket" << endl;
    return -1;
  }

  // CONNECTING SOCKET TO ADDRESS
  n = connect(sockfd, (struct sockaddr *)&client, sizeof(client));
  if (n == -1)
  {
    cout << "Error in connecting" << endl;
    return -1;
  }
  green();
  printf("Connected to server...\n");
  reset();

  char buff[1024];

  // VERIFYING CREDENTIALS
  while (1)
  {
    string s;
    memset(buff, 0, 1024);
    yellow();
    cout << "Signup or Login(S/L): " << Reset;
    cin >> s;
    strcpy(buff, s.c_str());
    yellow();
    cout << "Enter username: " << Reset;
    cin >> s;
    strcat(buff, (' ' + s).c_str());
    yellow();
    cout << "Enter password: " << Reset;
    cin >> s;
    strcat(buff, (' ' + s).c_str());
    n = send(sockfd, buff, strlen(buff), 0);
    if (n == -1)
    {
      perror("send: ");
      close(sockfd);
      return -1;
    }
    memset(buff, 0, 1024);
    n = recv(sockfd, buff, 1024, 0);
    if (n == -1)
    {
      perror("recv: ");
      close(sockfd);
      return -1;
    }
    else if (n == 0)
    {
      cout << "Client disconnected" << endl;
      close(sockfd);
      return -1;
    }
    buff[n] = '\0';

    if (strncmp(buff, "Successfully Logged in", strlen("Successfully Logged in")) == 0)
    {
      blue();
      printf("\nServer :%s %s\n", Green, buff);
      reset();
      cout << "\n-------\n";
      break;
    }
    if (strncmp(buff, "New User Created", strlen("New User Created")) == 0)
    {
      blue();
      printf("\nServer :%s %s\n", Green, buff);
      reset();
      break;
    }

    blue();
    printf("\nServer : %s\n", buff);
    reset();
  }

  // READING THREAD
  pthread_t thread_id;
  pthread_create(&thread_id, NULL, &reading, &sockfd);

  // SENDING WHILE LOOP
  while (1)
  {
    memset(buff, 0, 1024);
    // scanf till \n
    scanf(" %[^\n]s", buff);
    if (isClosed)
    {
      break;
    }
    if (strcmp(buff, "clear") == 0)
    {
      cout << "\033[2J\033[1;1H";
    }
    n = send(sockfd, buff, strlen(buff), 0);
    if (n == -1)
    {
      perror("Error in sending");
      close(sockfd);
      return -1;
    }

    if (strcmp(buff, "exit") == 0)
    {
      red();
      cout << "Closing connection" << endl;
      reset();
      break;
    }
  }

  // CLOSING SOCKET FD
  close(sockfd);
  return 0;
}
