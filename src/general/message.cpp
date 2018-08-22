
#include "general/message.h"


#define MESSAGE_STRING_SIZE 1000
static char message_string[MESSAGE_STRING_SIZE];

typedef int (Print_message_function)(const char *,enum Message_type, void *);

static Print_message_function
    *print_any_message_function=(Print_message_function *)NULL;
static void	*print_message_data = (void *)NULL;

int print_message_string(enum Message_type message_type,
    const char *the_string)
{
    int return_code = 0;
    if (strlen(the_string)) {
        if (print_any_message_function)
        {
            return_code = (*print_any_message_function)(the_string, message_type,
                print_message_data);
        } else {
            char prefixed_message_string[MESSAGE_STRING_SIZE + 10];
            switch (message_type) {
            case ERROR_MESSAGE:
                sprintf(prefixed_message_string, "ERROR: %s", the_string);
                break;
            case WARNING_MESSAGE:
                sprintf(prefixed_message_string, "WARNING: %s", the_string);
                break;
            case INFORMATION_MESSAGE:
                sprintf(prefixed_message_string, "%s", the_string);
                break;
            default:
                break;
            }
            printf("%s\n", prefixed_message_string);
        }
    }


    return return_code;
}

int print_message(enum Message_type message_type,const char *format, ... )
/*******************************************************************************
LAST MODIFIED : 15 September 2008

DESCRIPTION :
A function for displaying a message of the specified <message_type>.  The printf
form of arguments is used.
==============================================================================*/
{
    int return_code;
    va_list ap;

    va_start(ap, format);
    message_string[MESSAGE_STRING_SIZE-1] = '\0';
    return_code = vsnprintf(message_string, MESSAGE_STRING_SIZE-1, format, ap);
    if (return_code >= (MESSAGE_STRING_SIZE-1))
    {
        char error_string[100];
        sprintf(error_string,"Overflow of message_string.  "
            "Following is truncated to %d characters:",MESSAGE_STRING_SIZE-1);
        print_message_string(ERROR_MESSAGE, error_string);
    }
    va_end(ap);

    print_message_string(message_type, message_string);

    return (return_code);
} /* display_message */
