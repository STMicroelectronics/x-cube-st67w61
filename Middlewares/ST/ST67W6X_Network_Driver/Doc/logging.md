\page util_logging Utility logging component

============
\tableofcontents

\section log_intro Introduction

The **logging component** provides 4 logging macros listed in increasing order of verbosity:

- LogError()
- LogWarn()
- LogInfo()
- LogDebug()

The **logging component** features:

- differed logging. Message is built within the calling function and sent to a log queue. The log queue is consumed by an output task. This limit the real time impact of logging.
- Standard log levels See https://www.FreeRTOS.org/logging.html
- Add metadata to the log message such as timestamp, filename and line number, taskname. Metadata inclusion is configurable.
- Memory consumption scales with needs (dynamic allocation of log buffer). Maximum Size is configurable.
- Thread safe
- Output hardware choice left to application writer

For example, LogError() is only called when there is an error so is the least verbose, whereas LogDebug() is called more frequently to provide debug level information.

\section log_getting_started Getting started

To use the logging a developer must follow the steps below:

- To copy the `logging_config_template.h` file into the project folder and rename it as `logging_config.h`.
- To initialize the logging component.
- To include the header `logging.h` in each compilation unit that uses the logging macro.

\subsection log_gs_config The configuration file

A global configuration file, named `logging_config.h`, must be provided. A developer can use this file
to fine tune the logging component for a specific application. To learn more about the available configuration
options look at the default configuration in `logging_config_template.h`. This file provide a default value
for all the configuration options. This means that the user configuration file can be empty.

An important parameter is the `LOG_LEVEL`. It defines the global verbosity level. Valid values for are:

- ::LOG_NONE (turns logging off)
- ::LOG_ERROR
- ::LOG_WARN
- ::LOG_INFO
- ::LOG_DEBUG

The LOG_LEVEL can be redefined for specific portion of the application. For example:

```c
/* file my_file.c */

/* Disable the logging for this file. */
#define LOG_LEVEL      LOG_NONE
#include "logging.h"
```

**NOTE**: All the log messages with a log level minor than `LOG_LEVEL` are stripped from the final application binary.

\subsection log_gs_init Logging initialization

The section \ref log_fw_arch "Firmware architecture" gives more technical details about the logging implementation.
For the moment we need to know how to initialize the logging component. Because it is composed by two parts, an
high level API and a low level driver, a developer must initialize both. for example:

```c
/* file: MeExample.c */

#include "logging.h"

static void LogOutput(const char *message);
{
  /* Here this application writer can output on UART, ITM, USB..*/
  printf("%s", message);
}

void MyInitFunc(void)
{
  /* Initialize the high level logging service. */
  vLoggingInit(LogOutput);

  /* Sent a message in the log. */
  LogInfo(("Hello logging!\r\n"));
}
```

**NOTE**: The logging must be initialized and used after the scheduler is started!!

\subsection log_gs_use_case Logging use case

After the logging component has been initialized it is straightforward to use it to log information. For example:

```c
/* file: MeModule.c */

#include "logging.h"

void MyFunc(void)
{
  /* Sent a message in the log. */
  LogInfo(("Module Func\r\n"));

  if (error_is_detected)
  {
    LogError("error in my module!");
  }
}
```

\section log_fw_arch Firmware architecture

The Logging firmware is a self-contained firmware component that can be easily ported on other applications based on FreeRTOS.
To decouple the hardware independent logic of the logging from the physical and hardware depending channel used to display
the log messages, the component is designed around two parts:

- A **log service** that exports the logging macro and is responsible to format the log messages to feed the output queue.
- An **output task** that gets messages from the output queue and output the messages to peripheral.

This idea and the data flow is displayed in \ref log_fig01 "Fig.1".

\anchor log_fig01 \image html log_flow.png "Fig.1 - High level architecture"

In this example the output peripheral is an UART of the MCU that sends the data to a terminal emulator running on the host PC (4).
When the application logs a message (1), the **logging service** allocates the necessary memory, formats the message by adding some information
before the message using the function vLoggingPrintf(), then it sends the formatted message in the log queue (2). The **output task** is unblocked. It will get the message from the queue send it to the peripheral. Once peripheral transfer is completed, the memory is freed.

For each message the logging service formats the message in the following way:

```
[LOG_LEVEL] [TIMESTAMP_MS] [TASK_NAME] (FILE_NAME:LINE_NUMBER) "log message"
```

For example the following code line from a task named `defaultTsk`, in the file app_freertos.c,

```
LogWarn(("Hello logging!\r\n"));
```

generate this entry in the log:

```
[WARNING] 0 [defaultTsk] (app_freertos.c:604) Hello logging!
```

\subsection log_inc_shell Add Shell output

The logging component has a low level driver ready to be used. This driver uses the UART to send the log messages to a
terminal (e.g. [Tera Term](https://teratermproject.github.io/index-en.html) terminal emulator running in a host PC). It is common for an embedded
application to provide, other than a logging service, a way to control the application, at least during the development,
for example a Command Line Interface (CLI). A CLI or a more complex shell uses also the UART. This driver has been designed
for this scenario, to allow to use one UART for both features.

During the initialization, LoggingInit(LogOutput) returns the queue handle used by **output task**. This queue handle can be used by another service to post messages to manage both text streams, the one for the logging and a second one (e.g. for a shell).

The result is displayed in
\ref log_fig02 "Fig.2"
\anchor log_fig02 \image html 2ch_log_flow.png "Fig.2 - Dual Channel Log and Shell data flow."

to initialize the driver, and the logging initialization looks like this code snippet:

```c
/* file: MeExample.c */

#include "logging.h"
#include "shell.h"

void MyInitFunc(void)
{
  /* Initialize the high level logging service. */
  QueueHandle_t xLogQueue = vLoggingInit(LogOutput);
  /* Initialize the shell input with uart handle and shell output with xLogQueue. */
  shell_freertos_init(&huart_cli_input, xLogQueue);
}
```

\subsection log_itm ITM driver

For the compatible MCU series, the component provides a driver to display the log messages using the Instrumentation
Trace Macrocell (ITM). This technologies is part of the [Arm® CoreSight™ debugger technology](https://documentation-service.arm.com/static/63297311defc2c309b7124b4).

to initialize the driver with LogOutput implementing ITM output, the logging initialization looks like this code snippet:

```c
/* file: MeExample.c */

#include "logging.h"

static void LogOutput(const char *message);
{
  for (int32_t i = 0; i < strlen(message); i++)
  {
    ITM_SendChar(message[i]);
  }
}

void MyInitFunc(void)
{
  /* Initialize the high level logging service. */
  vLoggingInit(LogOutput);

  /* Sent a message in the log. */
  LogInfo(("Hello logging!\r\n"));
}
```

This driver allows to free the UART for other purpose (e.g. SHELL), but the data are transmitted in a synchronous way.

\subsection log_fw_arch_other Other considerations

It is important to note the at OutputTask has low priority to minimize real time impact of the logging system . Still, it must have enough time to consume the message from the queue and free allocated memory.
