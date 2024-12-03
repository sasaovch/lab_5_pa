#include "ipc.h"
#include "pipes_const.h"

#include <unistd.h>
#include <errno.h>
#include <stdio.h>

int send_multicast(void *__info, const Message *msg) {
    Info *info = (Info *)__info;
    
    local_id iterator = 0;
    while (iterator < info->N) {
        if (iterator != info->fork_id) {
            send(__info, iterator, msg);
        }
        
        iterator++;
    }
    return 0;
}

int send(void *__info, local_id pipe_id, const Message *msg) {
    Info *info = (Info *)__info;

    size_t message_size = sizeof(MessageHeader) + msg->s_header.s_payload_len;
    write(info->pm[info->fork_id][pipe_id][1], msg, message_size);
    return 0;
}

int receive(void *__info, local_id from, Message *msg) {
  Info *info = (Info *)__info;
  int fd = info->pm[from][info->fork_id][0];

  if (fd == -1) {
    return -1;
  }
  
  if (read(fd, &msg->s_header, sizeof(MessageHeader)) == -1) {
    return -1;
  } else {
    if (msg->s_header.s_payload_len > 0 && read(fd, &msg->s_payload, msg->s_header.s_payload_len) == -1) {
        return -1;
    }
  }
  return 0;
}

int receive_any(void * self, Message * msg) {
    Info *info = (Info *) self;
    local_id process_id = info->fork_id;

    while(1) {
        for (int from = 0; from < info->N; from++) {
            if (from != process_id) {
                if (receive(self, from, msg) == 0) {
                    return from;
                }
            }
        }
    }
}
