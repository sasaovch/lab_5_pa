#include "ipc.h"
#include "pa2345.h"
#include "child_work.h"
#include "priority_queue.h"
#include "pipes_const.h"
#include "time_work.h"
#include "banking.h"

int request_cs(const void *self) {
    Info* pipe_info = (Info *) self;
    local_id pipe_id = pipe_info->fork_id;
    int N = pipe_info->N;

    pipe_info->local_time++;
    
    PriorityQueueElement element;
    element.pipe_id = pipe_id;
    element.timestamp = get_lamport_time();
               
    // push(element);

    Message request_msg;
    request_msg.s_header.s_magic = MESSAGE_MAGIC;
    request_msg.s_header.s_type = CS_REQUEST;
    request_msg.s_header.s_local_time = get_lamport_time();
    request_msg.s_header.s_payload_len = 0;


    send_multicast(pipe_info, &request_msg);

    int replies_left = N - 2;
    while (1) {
        if (replies_left == 0) {
            break;
        }

        Message received_msg;
        PriorityQueueElement received_element;
        received_element.pipe_id = 0;
        received_element.timestamp = 0;
        local_id from_id = receive_any(pipe_info, &received_msg);
        sync_lamport_time(pipe_info, received_msg.s_header.s_local_time);

        switch (received_msg.s_header.s_type) {
            case CS_REPLY:
                replies_left--;
                break;

            case CS_REQUEST:
                received_element.pipe_id = from_id;
                received_element.timestamp = received_msg.s_header.s_local_time;

                if (
                    element.timestamp > received_element.timestamp
                    || (
                        element.timestamp == received_element.timestamp
                        && element.pipe_id > received_element.pipe_id
                    )
                ) {
                    Message reply_msg;
                    pipe_info->local_time++;

                    reply_msg.s_header.s_magic = MESSAGE_MAGIC;
                    reply_msg.s_header.s_payload_len = 0;
                    reply_msg.s_header.s_type = CS_REPLY;
                    reply_msg.s_header.s_local_time = get_lamport_time();                      
                    send(pipe_info, from_id, &reply_msg);
                } else {
                    push(received_element);
                }
         
                break;

            // case CS_RELEASE:               
                // pop();
                // break;

            case DONE:
                // fprintf(elf, "%d: done child %d from %d with type %d got message: %s\n", get_lamport_time(), pipe_id, from_id, received_msg.s_header.s_type, received_msg.s_payload);
                // fflush(elf);

                // fprintf(stdout, "%d: done child %d from %d with type %d got message: %s\n", get_lamport_time(), pipe_id, from_id, received_msg.s_header.s_type, received_msg.s_payload);
                // fflush(stdout);              
                pipe_info->received_done_msg++;
                break;
            }
        }
    fprintf(elf, "%d: receive permission\n", pipe_id);
    fflush(elf);

    // fprintf(stdout, "%d: receive permission\n", pipe_id);
    // fflush(stdout);    
    return 0;
}

int release_cs(const void *self) {
    Info* pipe_info = (Info *) self;
    while (not_empty()) {
        PriorityQueueElement send_element = pop();

        Message release_msg;
        pipe_info->local_time++;

        release_msg.s_header.s_magic = MESSAGE_MAGIC;
        release_msg.s_header.s_payload_len = 0;
        release_msg.s_header.s_type = CS_REPLY;
        release_msg.s_header.s_local_time = get_lamport_time();                      
        send(pipe_info, send_element.pipe_id, &release_msg);        
    }

    return 0;
}
