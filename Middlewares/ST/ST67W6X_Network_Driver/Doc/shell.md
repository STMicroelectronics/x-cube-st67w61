\page util_shell Utility Shell component

============
\tableofcontents

\section shell_intro Introduction

The Shell provides a command-line interface (CLI) that allows a user to interact with the application via a
serial interface (typically UART).

The shell is intended primarily for:

- Interactive control during development and debugging
- Exposing diagnostic commands (status, configuration, basic actions)
- Demonstration applications that require a simple console interface

The **shell component** features:

- auto completion.
- history (down/up key).
- navigate with right/left key (ctrl to move by word) and home/end key.
- backspace and delete.
- insert mode and overwrite mode (insert key to toggle).
- cancel command (ctrl+c).
- help &cmd help.
- decentralization of the command implementation.
- colorized CLI.

\section shell_getting_started Getting started

\subsection shell_gs_1 Add Shell in the project

To add the Shell feature into a project, The application writer must :

- Add all source files in the shell directory to the project.
- Add the shell directory to the include path.

All commands are placed in ROM by the linker, so the application writer must add 2 sections in the linker file.

These sections delimit the command table exported by `SHELL_CMD_EXPORT_*` macros. The shell iterates this table at
runtime to implement command discovery and help output.

An example of STM32CubeIDE linker script:

```
  /* The program code and other data into "FLASH" Rom type memory */
  .text :
  {
    *(.text)           /* .text sections (code) */
    ...

    . = ALIGN(4);
    __fsymtab_start = .;
    KEEP(*(FSymTab))
    __fsymtab_end = .;

    _etext = .;        /* define a global symbols at end of code */
  } >FLASH
```

An example of EWARM linker script:

```
keep { section FSymTab };
place in ROM_region   { readonly, ro section FSymTab };
```

An example of MDK-ARM configuration:
In Misc controls field of the project Linker options (Project->Options->Linker->Misc controls) add the following options:

```
--keep *.o(FSymTab) --diag_suppress L6319W
```

The diag suppress is needed to avoid the warning about the unused section.

\subsection shell_gs_2 Initialize the Shell

The following code initializes the Shell.

It requires a single parameter which is an optional FreeRTOS queue handle for shell output. When a queue is
provided, the shell prints through that queue (which can be shared with logging). When it is NULL, the shell
falls back to standard `printf`-style output.

```c
/* file: MeExample.c */

#include "shell.h"

static void shell_freertos_uart_rx_cplt_callback(UART_HandleTypeDef *huart)
{
  if (huart == &uart_shell_input)
  {
    /* Send char to Shell */
    shell_freertos_on_new_data(uart_rx_byte);

    /* Set up next uart character reception */
    HAL_UART_Receive_IT(&uart_shell_input, (uint8_t *)&uart_rx_byte, 1);
  }
}

void MyInitFunc(void)
{
  HAL_UART_RegisterCallback(&uart_shell_input, HAL_UART_RX_COMPLETE_CB_ID,
                            shell_freertos_uart_rx_cplt_callback);

  shell_freertos_init( NULL);

  /* Start the process by reading the user input */
  HAL_UART_Receive_IT(&uart_shell_input, (uint8_t *)&uart_rx_byte, 1);
}
```

If the shell output is shared with logging (e.g. same UART for Shell and Logging), the following initialization
sequence can be used. Here, `xLogQueue` is used to queue both Shell and Log messages.

```c
/* file: MeExample.c */
#include "logging.h"
#include "shell.h"

void MyInitFunc(void)
{
  /* Initialize the high level logging service. */
  QueueHandle_t xLogQueue = vLoggingInit(LogOutput);

  /* Register rx callback */
  HAL_UART_RegisterCallback(&uart_shell_input, HAL_UART_RX_COMPLETE_CB_ID,
                            shell_freertos_uart_rx_cplt_callback);

  /* Initialize the Shell input with xLogQueue. */
  shell_freertos_init(xLogQueue);

  /* Start the process by reading the user input */
  HAL_UART_Receive_IT(&uart_shell_input, (uint8_t *)&uart_rx_byte, 1);
}
```

\subsection shell_gs_3 Add Shell command

Commands are implemented and SHELL_CMD_EXPORT_ALIAS exports the command to the Shell database.

```c
/* file: MeExample.c */

#include "shell.h"
static void wifi_scan_cmd(int argc, char **argv)
{
  /* your code */
}
SHELL_CMD_EXPORT_ALIAS(wifi_scan_cmd, scan, scan [param1] <param2>);
```
