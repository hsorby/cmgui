
#include "command/commands.h"

#include <cstring>

#include "command/parser.h"

void execute_command(const char *command_string,void *command_data_void, int * /*quit*/, int *error)
{
    char **token;
    int i, return_code = 1;
    struct cmzn_command_data *command_data;
    struct Option_table *option_table;
    struct Parse_state *state;

    command_data = (struct cmzn_command_data *)command_data_void;
    if (command_data != nullptr) {
        state = create_Parse_state(command_string);
        if (state != nullptr) {
            i=state->number_of_tokens;
            /* check for comment */
            if (i > 0) {
                /* check for a "<" as one of the of the tokens */
                    /*???DB.  Include for backward compatability.  Remove ? */
                token=state->tokens;
                while ((i > 0) && strcmp(*token, "<"))
                {
                    i--;
                    token++;
                }
                if (i>0) {
                    /* return to tree root */
                    //return_code=set_command_prompt("",command_data);
                } else {
                    option_table = create_Option_table();
                    /* gfx */
                    //Option_table_add_entry(option_table, "gfx", NULL, command_data_void,
                    //    execute_command_gfx);
                    /* open */
                    //Option_table_add_entry(option_table, "open", NULL, command_data_void,
                    //    execute_command_open);
                    /* quit */
                    //Option_table_add_entry(option_table, "quit", NULL, command_data_void,
                    //    execute_command_quit);
                    /* list_memory */
                    //Option_table_add_entry(option_table, "list_memory", NULL, NULL,
                    //    execute_command_list_memory);
                    /* read */
                    //Option_table_add_entry(option_table, "read", NULL, command_data_void,
                    //    execute_command_read);
                    /* set */
                    //Option_table_add_entry(option_table, "set", NULL, command_data_void,
                    //    execute_command_set);
                    /* system */
                    //Option_table_add_entry(option_table, "system", NULL, command_data_void,
                    //    execute_command_system);
                    return_code=Option_table_parse(option_table, state);
                    destroy_Option_table(&option_table);
                }
                // Catching case where a fail returned code is returned but we are
                // asking for help, reseting the return code to pass if this is the case.
                if (!return_code && state->current_token &&
                   ((0==strcmp(PARSER_HELP_STRING,state->current_token)) ||
                   (0==strcmp(PARSER_RECURSIVE_HELP_STRING,state->current_token))))
                {
                    return_code = 1;
                }

            }
            destroy_Parse_state(&state);
        }
        else
        {
            print_message(ERROR_MESSAGE,
                "cmiss_execute_command.  Could not create parse state");
            return_code=0;
        }
    }
    else
    {
        print_message(ERROR_MESSAGE,
            "cmiss_execute_command.  Missing command_data");
        return_code=0;
    }

    *error = return_code;

} /* execute_command */

