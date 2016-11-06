#include "DpaTask.h"
#include "DpaMessage.h"
#include "MqMessaging.h"
#include "IqrfLogging.h"
#include "PlatformDep.h"
#include <climits>
#include <ctime>
#include <ratio>
#include <chrono>

const unsigned IQRF_MQ_BUFFER_SIZE = 1024;

void MqMessaging::sendDpaMessageToMq(const DpaMessage&  dpaMessage)
{
  ustring message(dpaMessage.DpaPacketData(), dpaMessage.Length());
  TRC_DBG(FORM_HEX(message.data(), message.size()));
  
  m_toMqMessageQueue->pushToQueue(message);
}

void MqMessaging::setDaemon(IDaemon* daemon)
{
  m_daemon = daemon;
  m_daemon->registerMessaging(*this);
}

int MqMessaging::handleMessageFromMq(const ustring& mqMessage)
{
  TRC_DBG("==================================" << std::endl <<
    "Received from MQ: " << std::endl << FORM_HEX(mqMessage.data(), mqMessage.size()));

  m_transaction->setMessage(mqMessage);
  m_daemon->executeDpaTransaction(*m_transaction);

  return 0;
}

MqMessaging::MqMessaging()
  :m_daemon(nullptr)
  , m_mqChannel(nullptr)
  , m_toMqMessageQueue(nullptr)
{
  m_localMqName = "iqrf-daemon-110";
  m_remoteMqName = "iqrf-daemon-100";
}

MqMessaging::~MqMessaging()
{
}

void MqMessaging::start()
{
  TRC_ENTER("");

  m_mqChannel = ant_new MqChannel(m_remoteMqName, m_localMqName, IQRF_MQ_BUFFER_SIZE);

  m_toMqMessageQueue = ant_new TaskQueue<ustring>([&](const ustring& msg) {
    m_mqChannel->sendTo(msg);
  });

  m_mqChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
    return handleMessageFromMq(msg); });

  m_transaction = ant_new MqMessagingTransaction(this);

  std::cout << "daemon-MQ-protocol started" << std::endl;

  TRC_LEAVE("");
}

void MqMessaging::stop()
{
  TRC_ENTER("");
  std::cout << "udp-daemon-protocol stops" << std::endl;
  delete m_transaction;
  delete m_mqChannel;
  delete m_toMqMessageQueue;
  TRC_LEAVE("");
}

////////////////////////////////////
MqMessagingTransaction::MqMessagingTransaction(MqMessaging* mqMessaging)
  :m_mqMessaging(mqMessaging)
{
}

MqMessagingTransaction::~MqMessagingTransaction()
{
}

const DpaMessage& MqMessagingTransaction::getMessage() const
{
  return m_message;
}

void MqMessagingTransaction::processConfirmationMessage(const DpaMessage& confirmation)
{
  m_mqMessaging->sendDpaMessageToMq(confirmation);
}

void MqMessagingTransaction::processResponseMessage(const DpaMessage& response)
{
  m_mqMessaging->sendDpaMessageToMq(response);
}

void MqMessagingTransaction::processFinished(bool success)
{
  m_success = success;
}

void MqMessagingTransaction::setMessage(ustring message)
{
  m_message.DataToBuffer(message.data(), message.length());
}
///////////////////////////////////////////////

#if 0
//https://www.softprayog.in/programming/interprocess-communication-using-posix-message-queues-in-linux
/* * server.c: Server program
* to demonstrate interprocess commnuication
* with POSIX message queues */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#define SERVER_QUEUE_NAME "/sp-example-server"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE 256
#define MSG_BUFFER_SIZE MAX_MSG_SIZE + 10

int main(int argc, char **argv)
{
  mqd_t qd_server, qd_client;  // queue descriptors
  long token_number = 1; // next token to be given to client

  printf("Server: Hello, World!\n");

  struct mq_attr attr;

  attr.mq_flags = 0;
  attr.mq_maxmsg = MAX_MESSAGES;
  attr.mq_msgsize = MAX_MSG_SIZE;
  attr.mq_curmsgs = 0;

  if ((qd_server = mq_open(SERVER_QUEUE_NAME, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
    perror("Server: mq_open (server)");
    exit(1);
  }

  char in_buffer[MSG_BUFFER_SIZE];
  char out_buffer[MSG_BUFFER_SIZE];

  while (1) {
    // get the oldest message with highest priority
    if (mq_receive(qd_server, in_buffer, MSG_BUFFER_SIZE, NULL) == -1) {
      perror("Server: mq_receive");
      exit(1);
    }

    printf("Server: message received.\n");

    // send reply message to client
    if ((qd_client = mq_open(in_buffer, O_WRONLY)) == 1) {
      perror("Server: Not able to open client queue");
      continue;
    }

    sprintf(out_buffer, "%ld", token_number);

    if (mq_send(qd_client, out_buffer, strlen(out_buffer), 0) == -1) {
      perror("Server: Not able to send message to client");
      continue;
    }

    printf("Server: response sent to client.\n");
    token_number++;
  }
}

/* * client.c: Client program
* to demonstrate interprocess commnuication
* with POSIX message queues */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#define SERVER_QUEUE_NAME "/sp-example-server"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE 256
#define MSG_BUFFER_SIZE MAX_MSG_SIZE + 10

int main(int argc, char **argv) {

  char client_queue_name[64];
  mqd_t qd_server, qd_client; // queue descriptors

  // create the client queue for receiving messages from server
  sprintf(client_queue_name, "/sp-example-client-%d", getpid());

  struct mq_attr attr;

  attr.mq_flags = 0;
  attr.mq_maxmsg = MAX_MESSAGES;
  attr.mq_msgsize = MAX_MSG_SIZE;
  attr.mq_curmsgs = 0;

  if ((qd_client = mq_open(client_queue_name, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
    perror("Client: mq_open (client)");
    exit(1);
  }

  if ((qd_server = mq_open(SERVER_QUEUE_NAME, O_WRONLY)) == -1) {
    perror("Client: mq_open (server)");
    exit(1);
  }

  char in_buffer[MSG_BUFFER_SIZE];

  printf("Ask for a token (Press <ENTER>): ");

  char temp_buf[10];

  while (fgets(temp_buf, 2, stdin)) {

    // send message to server
    if (mq_send(qd_server, client_queue_name, strlen(client_queue_name), 0) == -1) {
      perror("Client: Not able to send message to server");
      continue;
    }

    // receive response from server

    if (mq_receive(qd_client, in_buffer, MSG_BUFFER_SIZE, NULL) == -1) {
      perror("Client: mq_receive");
      exit(1);
    }
    // display token received from server
    printf("Client: Token received from server: %s\n\n", in_buffer);

    printf("Ask for a token (Press ): ");
  }


  if (mq_close(qd_client) == -1) {
    perror("Client: mq_close");
    exit(1);
  }

  if (mq_unlink(client_queue_name) == -1) {
    perror("Client: mq_unlink");
    exit(1);
  }
  printf("Client: bye\n");

  exit(0);
}
#endif
