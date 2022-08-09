#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#define ADDR "10.10.68.121"
using namespace std;

struct client
{
  int sockfd;
  string uname;
};
vector<struct client> clients;

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

int verifyCred(char *buf, int sockfd, string &uname)
{
  int n;
  string method;
  istringstream ss(buf);
  string word;
  ss >> word;
  method = word;
  ss >> word;
  uname = word;
  ss >> word;
  string pass = word;
  if (pass.length() == 0)
  {
    red();
    cout << "Password not entered" << endl;
    reset();
    n = send(sockfd, "Password not entered", strlen("Password not entered"), 0);
    if (n == -1)
    {
      perror("send: ");
      return -1;
    }
    return 0;
  }

  FILE *fp;
  string path = uname + "/credentials.txt";
  fp = fopen(path.c_str(), "r");

  if (fp && method == "L")
  {
    char line[1024];
    while (fgets(line, sizeof(line), fp))
    {
      if (strcmp(line, pass.c_str()) == 0)
      {
        fclose(fp);
        green();
        cout << "\nlogged in\n"
             << endl;
        reset();
        string msg = "Successfully Logged in";
        n = send(sockfd, msg.c_str(), msg.length(), 0);
        if (n == -1)
        {
          perror("send: ");
          return -1;
        }
        return 1;
      }
      else
      {
        fclose(fp);
        red();
        cout << "\nPassword wrong\n"
             << endl;
        reset();
        string msg = Red;
        msg += "Password wrong";
        msg += Reset;
        n = send(sockfd, msg.c_str(), msg.length(), 0);
        if (n == -1)
        {
          perror("send: ");
          return -1;
        }
        return 0;
      }
    }
  }
  else if (!fp && method == "S")
  {
    int check = mkdir(uname.c_str(), 0777);
    if (check == -1)
    {
      perror("mkdir: ");
      return -1;
    }
    else
    {
      fp = fopen(path.c_str(), "w");
      fprintf(fp, "%s", pass.c_str());
      fclose(fp);
      green();
      cout << "\nnew user created\n"
           << endl;
      reset();
      string msg = "New User Created";
      n = send(sockfd, msg.c_str(), msg.length(), 0);
      if (n == -1)
      {
        perror("send: ");
        return -1;
      }
      return 2;
    }
  }
  else if (fp && method == "S")
  {
    fclose(fp);
    red();
    cout << "\nUser already exists\n"
         << endl;
    reset();
    string msg = Red;
    msg += "User already exists. Pick another username";
    msg += Reset;
    n = send(sockfd, msg.c_str(), msg.length(), 0);
    if (n == -1)
    {
      perror("send: ");
      return -1;
    }
  }
  else if (!fp && method == "L")
  {
    red();
    cout << "\nUser does not exist\n"
         << endl;
    reset();
    string msg = Green;
    msg += "User does not exists. Please sign up first.";
    msg += Reset;
    n = send(sockfd, msg.c_str(), msg.length(), 0);
    if (n == -1)
    {
      perror("send: ");
      return -1;
    }
  }
  else
  {
    red();
    cout << "\nInvalid method\n"
         << endl;
    reset();
    string msg = Green;
    msg += "Please use S for signup and L for login";
    msg += Reset;
    n = send(sockfd, msg.c_str(), msg.length(), 0);
    if (n == -1)
    {
      perror("send: ");
      return -1;
    }
  }
  return 0;
}

int deleteFile(int sockfd, string uname, char *buf)
{
  int n;
  string path = uname + "/" + buf;
  if (remove(path.c_str()) != 0)
  {
    perror("remove: ");
    n = send(sockfd, "Could not delete file. Please try again!", strlen("Could not delete file. Please try again!"), 0);
    if (n == -1)
    {
      perror("send: ");
      return -1;
    }
  }
  else
  {
    green();
    cout << "File deleted" << endl;
    reset();
    n = send(sockfd, "File deleted", strlen("File deleted"), 0);
    if (n == -1)
    {
      perror("send: ");
      return -1;
    }
  }
  return 0;
}

int sendFile(string buf, int sockfd, string uname, int mode)
{
  FILE *fp;
  int n;

  // OPENING THE FILE
  if (mode)
    fp = fopen((uname + "/" + buf).c_str(), "rb");
  else
    fp = fopen((uname + "/" + buf).c_str(), "r");

  if (fp == NULL)
  {
    cout << "File not found" << endl;
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

    string msg = "File found:" + buf + ":" + to_string(size);
    yellow();
    cout << msg << endl;
    reset();
    n = send(sockfd, msg.c_str(), msg.length(), 0);
    if (n == -1)
    {
      perror("send: ");
      return -1;
    }
    sleep(0.5);
    char ch;
    string fcontent;
    // SENDING THE FILE
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
    sleep(0.5);
    green();
    cout << "\nFile sent" << endl;
    reset();
    fclose(fp);
  }
  return 0;
}

int receiveFile(char *buf, int sockfd, string uname, int mode)
{
  FILE *fp;
  int n;
  string fname;
  long size;
  char buff[1024];

  string path = uname + "/" + buf;

  if (mode)
    fp = fopen(path.c_str(), "wb");
  else
    fp = fopen(path.c_str(), "w");

  if (fp == NULL)
  {
    perror("fopen: ");
    n = send(sockfd, "Could not receive file. Please try again!", strlen("Could not receive file. Please try again!"), 0);
    if (n == -1)
    {
      perror("send: ");
      return -1;
    }
  }
  else
  {
    string msg = "Ready to receive ";
    msg += buf;
    cout << msg << endl;
    n = send(sockfd, msg.c_str(), msg.length(), 0);
    if (n == -1)
    {
      perror("send: ");
      return -1;
    }

    n = receive(buff, sockfd);
    if (n == -1 || n == 0)
    {
      return -1;
    }

    if (strcmp(buff, "File not found") == 0)
    {
      if (!remove(path.c_str()))
      {
        cout << "File still there" << endl;
      }
      red();
      cout << "File not found" << Reset << endl;
      return 0;
    }

    parseFile(fname, size, buff);
    purple();
    printf("File name: %s\nFile size: %ld\n", fname.c_str(), size);
    reset();

    char ch[1024];
    // RECEIVING THE FILE
    for (long i = 0; i < size;)
    {
      n = receive(ch, sockfd);
      if (n == -1 || n == 0)
      {
        remove(path.c_str());
        return -1;
      }
      fwrite(ch, sizeof(char), n, fp);
      i += n;
    }
    green();
    cout << "File received" << Reset << endl;
    fclose(fp);
  }
  return 0;
}

int listFiles(int sockfd, string uname)
{
  int n;
  DIR *dir;
  struct dirent *ent;
  string path = uname + "/";
  dir = opendir(path.c_str());
  if (dir == NULL)
  {
    perror("opendir: ");
    return -1;
  }
  string msg = Purple;
  msg += "\n\nFiles in directory:";
  msg += Reset;
  while ((ent = readdir(dir)) != NULL)
  {
    if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0 && strcmp(ent->d_name, "credentials.txt") != 0)
    {
      msg += "\n* ";
      msg += ent->d_name;
    }
  }
  closedir(dir);
  cout << msg << endl;
  n = send(sockfd, msg.c_str(), msg.length(), 0);
  if (n == -1)
  {
    perror("send: ");
    return -1;
  }
  return 0;
}

void *handleClient(void *args)
{
  // GETTING SOCKFD
  int sockfd = *(int *)args;
  int n;
  char buf[1024];
  FILE *fp;

  int mode = 1;

  string uname;

  // LOGIN CREDENTIALS
  while (1)
  {
    memset(buf, 0, 1024);
    if ((n = receive(buf, sockfd)) <= 0)
      pthread_exit(NULL);

    cout << buf << endl;
    n = verifyCred(buf, sockfd, uname);
    if (n == -1)
    {
      pthread_exit(NULL);
    }
    if (n > 0)
    {
      break;
    }
  }

  clients.push_back({sockfd, uname});

  sleep(0.5);
  // WELCOME MSG
  string msg = "Welcome " + uname + "!";
  msg += "\n\n\
  \033[0;35mget <file name>\033[0m - To download/get a file from server\n\
  \033[0;35mput <file name>\033[0m - To upload/put a file to server\n\
  \033[0;35mexit\033[0m - To close the connection\n\
  \033[0;35mclear\033[0m - To clear the screen\n\
  \033[0;35mlist\033[0m - To list all files in the server directory\n\
  \033[0;35mdelete <file name>\033[0m - To delete a file from server\n\
  \033[0;35mmode b/c\033[0m - b for binary mode, c for character mode";
  n = send(sockfd, msg.c_str(), msg.length(), 0);
  if (n == -1)
  {
    perror("send:");
    pthread_exit(NULL);
  }

  // WHILE LOOP
  while (1)
  {
    // RECEIVING
    memset(buf, 0, 1024);
    n = recv(sockfd, buf, 1024, 0);
    if (n == -1)
    {
      perror("recv: ");
      break;
    }
    else if (n == 0)
    {
      cout << "Client disconnected" << endl;
      break;
    }
    buf[n] = '\0';

    // PRINTING THE FILE NAME
    printf("%s: %s\n", uname.c_str(), buf);

    // SENDING THE FILE
    if (strncmp(buf, "get", strlen("get")) == 0)
    {
      if (sendFile(buf + 4, sockfd, uname, mode) == -1)
      {
        red();
        cout << "Error occured while sending file" << Reset << endl;
        close(sockfd);
        pthread_exit(NULL);
      }
    }

    // GETTING THE FILE
    else if (strncmp(buf, "put", strlen("put")) == 0)
    {
      if ((n = receiveFile(buf + 4, sockfd, uname, mode)) == -1)
      {
        red();
        cout << "Error occured while receiving file" << Reset << endl;
        close(sockfd);
        pthread_exit(NULL);
      }
    }

    // LIST ALL THE FILES
    else if (strcmp(buf, "list") == 0)
    {
      n = listFiles(sockfd, uname);
      if (n == -1)
      {
        red();
        cout << "Error occured while listing files" << Reset << endl;
        close(sockfd);
        break;
      }
    }

    // DELETE FILE
    else if (strncmp(buf, "delete", strlen("delete")) == 0)
    {
      n = deleteFile(sockfd, uname, buf + 7);
      if (n == -1)
      {
        red();
        cout << "Error occured while deleting file" << Reset << endl;
        close(sockfd);
        break;
      }
    }

    // EXIT COMMAND
    else if (strcmp(buf, "exit") == 0)
    {
      n = send(sockfd, "Closing connection", strlen("Closing connection"), 0);
      if (n == -1)
      {
        perror("send: ");
        close(sockfd);
        pthread_exit(NULL);
      }
      close(sockfd);
      red();
      cout << "Client disconnected" << Reset << endl;
      break;
    }

    // CLEAR SCREEN
    else if (strcmp(buf, "clear") == 0)
    {
      string msg = "Welcome " + uname + "!";
      msg += "\n\n\
      \033[0;35mget <file name>\033[0m - To download/get a file from server\n\
      \033[0;35mput <file name>\033[0m - To upload/put a file to server\n\
      \033[0;35mexit\033[0m - To close the connection\n\
      \033[0;35mclear\033[0m - To clear the screen\n\
      \033[0;35mlist\033[0m - To list all files in the server directory\n\
      \033[0;35mdelete <file name>\033[0m - To delete a file from server\n\
      \033[0;35mmode b/c\033[0m - b for binary mode, c for character mode";
      n = send(sockfd, msg.c_str(), msg.length(), 0);
      if (n == -1)
      {
        perror("send:");
        close(sockfd);
        break;
      }
    }

    // MODE CHANGE
    else if (strncmp(buf, "mode", strlen("mode")) == 0)
    {
      if (strcmp(buf + 5, "b") == 0)
      {
        mode = 1;
        n = send(sockfd, "Mode changed to binary", strlen("Mode changed to binary"), 0);
        if (n == -1)
        {
          perror("send:");
          close(sockfd);
          break;
        }
      }
      else if (strcmp(buf + 5, "c") == 0)
      {
        mode = 0;
        n = send(sockfd, "Mode changed to character", strlen("Mode changed to character"), 0);
        if (n == -1)
        {
          perror("send:");
          close(sockfd);
          break;
        }
      }
      else
      {
        n = send(sockfd, "Invalid mode", strlen("Invalid mode"), 0);
        if (n == -1)
        {
          perror("send:");
          close(sockfd);
          break;
        }
      }
    }

    // INVALID COMMAND
    else
    {
      n = send(sockfd, "Invalid command", strlen("Invalid command"), 0);
      if (n == -1)
      {
        perror("send: ");
        close(sockfd);
        pthread_exit(NULL);
      }
    }
  }
  pthread_exit(NULL);
  return NULL;
}

// MAIN FUNCTION
int main()
{
  // DECLARING VARIABLES
  struct sockaddr_in server, client;
  socklen_t caddr_size;
  server.sin_family = AF_INET;
  server.sin_port = htons(3003);
  server.sin_addr.s_addr = inet_addr(ADDR);
  int sockfd, n;
  char buff[1024];

  // CREATING SOCKET
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1)
  {
    cout << "Error in creating socket" << endl;
    return -1;
  }

  // SETTING SOCKET OPTIONS
  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
  {
    perror("setsockopt");
    return 1;
  }

  // BINDING SOCKET
  n = bind(sockfd, (struct sockaddr *)&server, sizeof(server));
  if (n == -1)
  {
    perror("bind:");
    cout << "Error in binding" << endl;
    return -1;
  }

  // LISTENING
  n = listen(sockfd, 10);
  if (n == -1)
  {
    cout << "Error in listening" << endl;
    return -1;
  }

  printf("Waiting for connection\n");

  // ACCEPTING CONNECTION
  while (1)
  {
    int sockfd1 = accept(sockfd, (struct sockaddr *)&client, &caddr_size);
    if (sockfd1 == -1)
    {
      cout << "Error in accepting" << endl;
      return -1;
    }

    printf("Connection accepted, socket: %d\n", sockfd1);

    // CREATING THREAD
    pthread_t thread;
    pthread_create(&thread, NULL, handleClient, &sockfd1);
  }
  close(sockfd);
  return 0;
}
