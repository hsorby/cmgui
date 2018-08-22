/*******************************************************************************
FILE : cmgui.cpp

LAST MODIFIED : 7 January 2003

DESCRIPTION :
==============================================================================*/
/* OpenCMISS-Cmgui Application
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*SAB I have concatenated the correct version file for each version
  externally in the shell with cat #include "version.h"*/

#include "cmgui.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QLabel>
#include <QDebug>
#include <QTextStream>

#include "configure/cmgui_configure.h"
#include "configure/cmgui_version.h"

#include "command/cmgui_command_window.h"
#include "command/cmgui_command_model.h"

/*
Global functions
----------------
*/

#if defined (WX_USER_INTERFACE)

void wxCmguiApp::OnIdle(wxIdleEvent& event)
{
	if (event_dispatcher)
	{
		if (Event_dispatcher_process_idle_event(event_dispatcher))
		{
			event.RequestMore();
		}
	}
}

void wxCmguiApp::SetEventDispatcher(Event_dispatcher *event_dispatcher_in)
{
	event_dispatcher = event_dispatcher_in;
}

#endif /*defined (WX_USER_INTERFACE)*/

const char applicationName[] = "Cmgui";

enum CommandLineParseResult
{
    CommandLineOk,
    CommandLineError,
    CommandLineVersionRequested,
    CommandLineHelpRequested
};

CommandLineParseResult parseCommandLine(QCommandLineParser &parser, const QStringList &arguments, QString *errorMessage)
{
    QString applicationDescription;
    QTextStream stream(&applicationDescription);
    stream << applicationName << " version " << CMGUI_VERSION_STRING << endl;
    stream << CMGUI_COPYRIGHT_STRING << endl;
    stream << "Build information: " << CMGUI_BUILD_STRING << " " << CMGUI_REVISION_STRING;

    parser.setApplicationDescription(applicationDescription);
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();

    if (!parser.parse(arguments)) {
        *errorMessage = parser.errorText();
        return CommandLineError;
    }

    if (parser.isSet(versionOption))
        return CommandLineVersionRequested;

    if (parser.isSet(helpOption))
        return CommandLineHelpRequested;

    return CommandLineOk;
}

QCoreApplication* createApplication(int &argc, char *argv[], ApplicationOptions &applicationOptions, CommandLineParseResult &status)
{
    QStringList arguments;
    for (int a = 0; a < argc; ++a) {
        arguments << QString::fromLocal8Bit(argv[a]);
    }

    QCommandLineParser parser;
    // A boolean option for running Cmgui sans GUI (--sans-gui)
    const QCommandLineOption sansGuiOption(QStringList() << "s" << "sans-gui", "Run Cmgui sans GUI.");
    parser.addOption(sansGuiOption);

    const QCommandLineOption commandFileOption(QStringList() << "f" << "command-file", "Command file name to load.");
    parser.addOption(commandFileOption);

    QString errorMessage;
    status = parseCommandLine(parser, arguments, &errorMessage);


    QCoreApplication *activeApplication;

    switch (status) {
    case CommandLineError:
    case CommandLineHelpRequested:
    case CommandLineVersionRequested:
        activeApplication = new QCoreApplication(argc, argv);
        break;
    default:
        if (parser.isSet(sansGuiOption)) {
            activeApplication = new QCoreApplication(argc, argv);
        } else {
            QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
            activeApplication = new QApplication(argc, argv);
        }
    }

    QCoreApplication::setOrganizationName("UoA");
    QCoreApplication::setApplicationName(applicationName);
    QCoreApplication::setApplicationVersion(CMGUI_VERSION_STRING);

    switch (status) {
    case CommandLineOk:
        applicationOptions.name = arguments.at(0);
        if (parser.isSet(commandFileOption)) {
            applicationOptions.commandFile = parser.value(commandFileOption);
        }
        break;
    case CommandLineError:
        qCritical() << errorMessage;
        qInfo().noquote() << parser.helpText();
    case CommandLineHelpRequested:
        qInfo().noquote() << parser.helpText();
    case CommandLineVersionRequested:
        qInfo().noquote() << parser.applicationDescription();
    }

    return activeApplication;
}

/**
 * Main program for the CMISS Graphical User Interface Cmgui
 */
int main(int argc, char *argv[])
{
    CommandLineParseResult status = CommandLineOk;
    ApplicationOptions applicationOptions;
    QScopedPointer<QCoreApplication> application(createApplication(argc, argv, applicationOptions, status));

    QSharedPointer<CmguiCommandModel>  mainModel(new CmguiCommandModel(applicationOptions));
    QScopedPointer<CmguiCommandWindow> mainWindow(nullptr);
    if (qobject_cast<QApplication *>(application.data())) {
       mainWindow.reset(new CmguiCommandWindow());
       mainWindow->setModel(mainModel);
       mainWindow->show();
    } else {
        switch (status) {
        case CommandLineOk:
            qDebug() << "Run application without GUI.";
            break;
        case CommandLineError:
            return 3;
        case CommandLineHelpRequested:
        case CommandLineVersionRequested:
            return 0;
        }
    }

    return application->exec();

#ifdef andrew

    int return_code = 0;
    struct cmzn_context_app *context = NULL;
	struct User_interface_module *UI_module = NULL;
	struct cmzn_command_data *command_data;

	/* display the version */
	display_message(INFORMATION_MESSAGE, "%s version %s %s\n%s\n"
		"Build information: %s %s\n", CMGUI_NAME_STRING, CMGUI_VERSION_STRING,
		CMGUI_DATETIME_STRING, CMGUI_COPYRIGHT_STRING, CMGUI_BUILD_STRING,
		CMGUI_REVISION_STRING);

	context = cmzn_context_app_create("default");
	if (context)
	{
#if defined (WX_USER_INTERFACE) || (!defined (WIN32_USER_INTERFACE) && !defined (_MSC_VER))
		UI_module = cmzn_context_create_user_interface(context, argc, argv, NULL);
#else /* !defined (WIN32_USER_INTERFACE)  && !defined (_MSC_VER)*/
		UI_module = cmzn_context_create_user_interface(context, argc, argv, current_instance,
			previous_instance, command_line, initial_main_window_state, NULL);
#endif /* !defined (WIN32_USER_INTERFACE)  && !defined (_MSC_VER)*/
		if (UI_module)
		{
#if defined (WX_USER_INTERFACE)
			if (UI_module->user_interface)
			{
				if (wxEntryStart(argc, argv))
				{
					wx_entry_started = 1;
					wxXmlResource::Get()->InitAllHandlers();
					wxCmguiApp &app = wxGetApp();
					if (&app)
					{
						app.SetEventDispatcher(UI_module->event_dispatcher);
					}
					else
					{
						display_message(ERROR_MESSAGE,
							"initialiseWxApp.  wxCmguiApp not initialised.");
					}
				}
				else
				{
					display_message(ERROR_MESSAGE,
						"initialiseWxApp.  Invalid arguments.");
				}
			}
#endif
			if (NULL != (command_data = cmzn_context_get_default_command_interpreter(context)))
			{
				cmzn_command_data_set_cmgui_string(command_data, CMGUI_NAME_STRING,
					CMGUI_VERSION_STRING, CMGUI_DATETIME_STRING, CMGUI_COPYRIGHT_STRING, CMGUI_BUILD_STRING,
					CMGUI_REVISION_STRING);
				cmzn_command_data_main_loop(command_data);
				cmzn_command_data_destroy(&command_data);
				return_code = 0;
			}
			else
			{
				return_code = 1;
			}
			User_interface_module_destroy(&UI_module);
		}
		else
		{
			return_code = 1;
		}
		cmzn_context_app_destroy(&context);
		Context_internal_cleanup();
	}
	else
	{
		return_code = 1;
	}

	return (return_code);
#endif
} /* main */
