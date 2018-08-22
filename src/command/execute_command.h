
#pragma once

typedef int (Execute_command_function)(const char *command,void *user_data);

class ExecuteCommand
{
public:
    Execute_command_function *function;
    void *data;
};
