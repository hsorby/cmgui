
#pragma once

#include <QSharedPointer>

#include "command/execute_command.h"

class QString;

namespace OpenCMISS {
namespace Zinc {
class Context;
}
}

#include "cmgui.h"

#if defined (USE_PERL_INTERPRETER)
#include "perl_interpreter.h"
#else
#include "general/message.h"
#endif /* defined (USE_PERL_INTERPRETER) */

class CmguiCommandModel
{
public:
    CmguiCommandModel(const ApplicationOptions& options);
    ~CmguiCommandModel();

    static int callbackPrintMessage(const char *message, enum Message_type message_type, void *message_data);
    int printMessage(enum Message_type message_type, const char *message);


   bool executeCommandFromString(const QString& command_string);

   bool setExecuteCommandFunction(
       Execute_command_function *execute_command_function,
       void *command_function_data);

   bool setSetCommandFunction(
       Execute_command_function *execute_command_function,
       void *command_function_data);

private:
   QSharedPointer<ExecuteCommand> executeCommand;
   QSharedPointer<ExecuteCommand> setCommand;
   QSharedPointer<OpenCMISS::Zinc::Context> context;

#if defined (USE_PERL_INTERPRETER)
    struct Interpreter *interpreter;
    int interpreter_argc;
    char **interpreter_argv;
#endif /* defined (USE_PERL_INTERPRETER) */
};
