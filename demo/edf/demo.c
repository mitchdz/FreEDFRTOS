/*
 * FreeRTOS V202012.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/******************************************************************************
 * NOTE 1: The FreeRTOS demo threads will not be running continuously, so
 * do not expect to get real time behaviour from the FreeRTOS Linux port, or
 * this demo application.  Also, the timing information in the FreeRTOS+Trace
 * logs have no meaningful units.  See the documentation page for the Linux
 * port for further information:
 * https://freertos.org/FreeRTOS-simulator-for-Linux.html
 *
 * NOTE 2:  This project provides two demo applications.  A simple blinky style
 * project, and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting in main.c is used to select
 * between the two.  See the notes on using mainCREATE_SIMPLE_BLINKY_DEMO_ONLY
 * in main.c.  This file implements the simply blinky version.  Console output
 * is used in place of the normal LED toggling.
 *
 * NOTE 3:  This file only contains the source code that is specific to the
 * basic demo.  Generic functions, such FreeRTOS hook functions, are defined
 * in main.c.
 ******************************************************************************
 *
 * main_blinky() creates one queue, one software timer, and two tasks.  It then
 * starts the scheduler.
 *
 * The Queue Send Task:
 * The queue send task is implemented by the prvQueueSendTask() function in
 * this file.  It uses vTaskDelayUntil() to create a periodic task that sends
 * the value 100 to the queue every 200 milliseconds (please read the notes
 * above regarding the accuracy of timing under Linux).
 *
 * The Queue Send Software Timer:
 * The timer is an auto-reload timer with a period of two seconds.  The timer's
 * callback function writes the value 200 to the queue.  The callback function
 * is implemented by prvQueueSendTimerCallback() within this file.
 *
 * The Queue Receive Task:
 * The queue receive task is implemented by the prvQueueReceiveTask() function
 * in this file.  prvQueueReceiveTask() waits for data to arrive on the queue.
 * When data is received, the task checks the value of the data, then outputs a
 * message to indicate if the data came from the queue send task or the queue
 * send software timer.
 *
 * Expected Behaviour:
 * - The queue send task writes to the queue every 200ms, so every 200ms the
 *   queue receive task will output a message indicating that data was received
 *   on the queue from the queue send task.
 * - The queue send software timer has a period of two seconds, and is reset
 *   each time a key is pressed.  So if two seconds expire without a key being
 *   pressed then the queue receive task will output a message indicating that
 *   data was received on the queue from the queue send software timer.
 *
 * NOTE:  Console input and output relies on Linux system calls, which can
 * interfere with the execution of the FreeRTOS Linux port. This demo only
 * uses Linux system call occasionally. Heavier use of Linux system calls
 * may crash the port.
 */

#include <stdio.h>
#include <pthread.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Local includes. */
#include "console.h"

/*-----------------------------------------------------------*/

enum TaskStatus { statusCOMPLETED, statusRUNNING, statusCREATED };

struct Message
{
	enum TaskStatus status;
	struct TaskParameters *params;
};

struct TaskParameters
{
	TickType_t deadline;
	TaskHandle_t handle;
};

/* EDF Stuff */
static void prvSchedulerTask( void *pvParameters );
static QueueHandle_t xSchedulerMessages = NULL;

#define mainTASK_ARRAY_SIZE 10
static struct TaskParameters *taskArray[ mainTASK_ARRAY_SIZE ] = { NULL };

static void addTaskToArray( struct TaskParameters *params );
static void removeTaskFromArray( struct TaskParameters *params );
static void setTaskPriorities();

#define mainSCHEDULER_PRIORITY ( configMAX_PRIORITIES - 1)
#define mainNEW_TASK_PRIORITY ( configMAX_PRIORITIES - 2 )

/* EDF Test Tasks */
static void prvTestTask( void *pvParameters );


/*** SEE THE COMMENTS AT THE TOP OF THIS FILE ***/
void main_crashy_mc_splody_demo_demo( void )
{
	/* Create the queue. */
	xSchedulerMessages = xQueueCreate( 10, sizeof( struct Message ) );

	struct TaskParameters params1;
	struct TaskParameters params2;

	params1.deadline = 2000;
	params2.deadline = 4000;

	if( xSchedulerMessages != NULL )
	{
		/* Start the two tasks as described in the comments at the top of this
		file. */
		xTaskCreate( prvSchedulerTask,
					"temp",
					configMINIMAL_STACK_SIZE,
					NULL,
					mainSCHEDULER_PRIORITY,
					NULL );

		xTaskCreate( prvTestTask,
					"test2",
					configMINIMAL_STACK_SIZE,
					&params2,
					mainNEW_TASK_PRIORITY,
					&params2.handle );

		xTaskCreate( prvTestTask,
					"test1",
					configMINIMAL_STACK_SIZE,
					&params1,
					mainNEW_TASK_PRIORITY,
					&params1.handle );

		/* Start the tasks and timer running. */
		vTaskStartScheduler();
	}

	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was insufficient FreeRTOS heap memory available for the idle and/or
	timer tasks	to be created.  See the memory management section on the
	FreeRTOS web site for more details. */
	for( ;; );
}
/*-----------------------------------------------------------*/

static void prvSchedulerTask( void *pvParameters )
{
	struct Message msg;
	struct TaskParameters *params = pvParameters;
	int res;

	printf( "SCHEDULER: EDF scheduler started\n" );

	for ( ;; )
	{
		if ( xQueueReceive( xSchedulerMessages, &msg, portMAX_DELAY ) )
		{
			printf( "SCHEDULER: Message received\n" );
			switch ( msg.status )
			{
				case statusCOMPLETED:
					printf( "SCHEDULER: A task completed\n" );
					vTaskDelete( msg.params->handle );
					removeTaskFromArray( msg.params );
					setTaskPriorities();
					break;
				case statusCREATED:
					printf( "SCHEDULER: A task was created\n" );
					addTaskToArray( msg.params );
					setTaskPriorities();
					break;
				case statusRUNNING:
					break;
				default:
					printf( "SCHEDULER: scrub\n" );
					break;
			}
		}
	}
}

static void prvTestTask( void *pvParameters )
{
	struct Message msg;
	struct TaskParameters *params = pvParameters;
	msg.params = params;

	msg.status = statusCREATED;
	xQueueSend( xSchedulerMessages, &msg, 50 );

	printf( "TASK: Task created, sent message\n" );
	printf( "TASK: Deadline of this task is %lu\n", params->deadline );

	msg.status = statusCOMPLETED;
	xQueueSend( xSchedulerMessages, &msg, 50 );
	printf( "TASK: Spaces: %d\n", uxQueueSpacesAvailable( xSchedulerMessages ) );

	for ( ;; );
}

static void addTaskToArray( struct TaskParameters *params )
{
	for ( int i = 0; i < mainTASK_ARRAY_SIZE; i++ )
	{
		if ( taskArray[i] == NULL )
		{
			taskArray[i] = params;
			return;
		}
	}
}

static void removeTaskFromArray( struct TaskParameters *params )
{
	for ( int i = 0; i < mainTASK_ARRAY_SIZE; i++ )
	{
		if ( taskArray[i] == params )
		{
			taskArray[i] = NULL;
			return;
		}
	}
}

static void setTaskPriorities()
{
	/**
	 * Set the priority based on tasks in ready queue
	 * The priority should be set lower than the default
	 * Want the created tasks to send their messages before
	 * any other tasks do anything else
	 */

	TickType_t earliestDeadline = 0xFFFFFFFFUL;

	/* Find the earliest deadline */
	for ( int i = 0; i < mainTASK_ARRAY_SIZE; i++ )
	{
		if ( taskArray[i] == NULL) continue;

		if ( taskArray[i]->deadline < earliestDeadline )
		{
			earliestDeadline = taskArray[i]->deadline;
		}
	}

	/* Apply updated priorities */
	for ( int i = 0; i < mainTASK_ARRAY_SIZE; i++ )
	{
		/* We expect time slicing to be enabled */
		if ( taskArray[i] == NULL ) continue;

		if ( taskArray[i]->deadline == earliestDeadline )
		{
			vTaskPrioritySet( taskArray[i]->handle, mainNEW_TASK_PRIORITY - 1);
		}
		else
		{
			vTaskPrioritySet( taskArray[i]->handle, mainNEW_TASK_PRIORITY - 2);
		}
	}
}