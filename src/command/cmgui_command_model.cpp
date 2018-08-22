
#include "command/cmgui_command_model.h"

#include <QDebug>
#include <string.h>

#include "opencmiss/zinc/context.hpp"

#include "general/message.h"
#include "command/commands.h"

CmguiCommandModel::CmguiCommandModel(const ApplicationOptions &options)
    : executeCommand(nullptr)
    , setCommand(nullptr)
    , context(nullptr)
#if defined (USE_PERL_INTERPRETER)
    , interpreter(nullptr)
    , interpreter_argc(0)
    , interpreter_argv(nullptr)
#endif /* defined (USE_PERL_INTERPRETER) */

{
    executeCommand = QSharedPointer<ExecuteCommand>(new ExecuteCommand);
    setCommand = QSharedPointer<ExecuteCommand>(new ExecuteCommand);
    context = QSharedPointer<OpenCMISS::Zinc::Context>(new OpenCMISS::Zinc::Context("default"));

#if defined (USE_PERL_INTERPRETER)
    int status;
    interpreter_argc = 1;
    QByteArray ba = options.name.toLatin1();
    char *applicationName = strdup(ba.data());
    interpreter_argv = (char **)malloc(2 * sizeof(char*));
    interpreter_argv[0] = applicationName;
    interpreter_argv[1] = nullptr;
    create_interpreter(interpreter_argc, interpreter_argv, options.commandFile.toLatin1().data(), &interpreter, &status);
    if (status == 1) {
        interpreter_set_display_message_function(interpreter, print_message, &status);
        qDebug() << "Successfully created perl interpreter.";
    } else {
        qDebug() << "The perl interpreter did not get created properly.";
    }
#endif /* defined (USE_PERL_INTERPRETER) */
}

CmguiCommandModel::~CmguiCommandModel()
{
#if defined (USE_PERL_INTERPRETER)
    int status;
    destroy_interpreter(interpreter, &status);
    free(interpreter_argv[1]);
    free(interpreter_argv);
    interpreter_argv = nullptr;
#endif /* defined (USE_PERL_INTERPRETER) */
}

bool CmguiCommandModel::executeCommandFromString(const QString &command_string)
{
    int quit, status;
    QByteArray ba = command_string.toLatin1();
    const char *char_command_string = ba.data();
    interpret_command_(interpreter, char_command_string, (void *)this, &quit, execute_command, &status);

    return status == 1;
}

bool CmguiCommandModel::setExecuteCommandFunction(
    Execute_command_function *execute_command_function,
    void *command_function_data)
{
    bool result = false;
    if (execute_command_function != nullptr) {
        executeCommand->function = execute_command_function;
        executeCommand->data = command_function_data;
        result = true;
    }
    return result;
}

bool CmguiCommandModel::setSetCommandFunction(
    Execute_command_function *execute_command_function,
    void *command_function_data)
{
    bool result = false;
    if (execute_command_function != nullptr) {
        setCommand->function = execute_command_function;
        setCommand->data = command_function_data;
        result = true;
    }
    return result;
}

int CmguiCommandModel::printMessage(enum Message_type message_type, const char *message)
{
    QString final_message = "";

    switch (message_type) {
    case ERROR_MESSAGE:
        final_message += "ERROR: ";
        break;
    case WARNING_MESSAGE:
        final_message += "WARNING: ";
        break;
    case INFORMATION_MESSAGE:
        final_message += "";
        break;
    default:
        break;
    }

    final_message += message;
    qDebug() << "Cmgui command model" << final_message;
    return 1;
}

int CmguiCommandModel::callbackPrintMessage(const char *message,enum Message_type message_type, void *message_data)
{
    CmguiCommandModel* self = (CmguiCommandModel*) message_data;
    return self->printMessage(message_type, message);
}
