
#pragma once

#ifndef MESSAGE_TYPE_ENUM
#define MESSAGE_TYPE_ENUM
enum Message_type
{
    ERROR_MESSAGE = 0,
    WARNING_MESSAGE = 1,
    INFORMATION_MESSAGE = 2
}; /* enum Message_type */
#endif

int print_message(enum Message_type message_type,const char *format, ... );
