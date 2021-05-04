#include <stdio.h>
#include <stdlib.h>
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
#define mainRUNNING_PRIORITY ( mainNEW_TASK_PRIORITY - 1 )
#define mainWAITING_PRIORITY ( mainRUNNING_PRIORITY - 1 )

/* EDF Test Tasks */
static void prvTestTask( void *pvParameters );

/* SAD Stuff */
static void prvSadTask( void *pvParameters );
static void prvDataTask( void *pvParameters );

#define DATA_LEN 10
int data1[DATA_LEN];
int data2[DATA_LEN];

/*** SEE THE COMMENTS AT THE TOP OF THIS FILE ***/
void main_crashy_mc_splody_demo_demo( void )
{
	/* Create the queue. */
	xSchedulerMessages = xQueueCreate( 10, sizeof( struct Message ) );

	struct TaskParameters params1;

	params1.deadline = portMAX_DELAY;

	if( xSchedulerMessages != NULL )
	{
		/* Start the two tasks as described in the comments at the top of this
		file. */
		xTaskCreate( prvSchedulerTask,
					"edf",
					configMINIMAL_STACK_SIZE,
					NULL,
					mainSCHEDULER_PRIORITY,
					NULL );

		xTaskCreate( prvDataTask,
					"datagen",
					configMINIMAL_STACK_SIZE,
					&params1,
					mainNEW_TASK_PRIORITY,
					&params1.handle );

		/* Start the tasks and timer running. */
		vTaskStartScheduler();
	}

	for( ;; );
}
/*-----------------------------------------------------------*/

static void prvSchedulerTask( void *pvParameters )
{
	struct Message msg;
	struct TaskParameters *params = pvParameters;
	int res;

	for ( ;; )
	{
		if ( xQueueReceive( xSchedulerMessages, &msg, portMAX_DELAY ) )
		{
		#ifdef DEBUG
			printf( "SCHEDULER: Message received\n" );
		#endif
			switch ( msg.status )
			{
			case statusCOMPLETED:
			#ifdef DEBUG
				printf( "SCHEDULER: A task completed\n" );
				printf( "SCHEDULER: Deadline: %lu\n", msg.params->deadline );
			#endif
				vTaskDelete( msg.params->handle );
				removeTaskFromArray( msg.params );
				setTaskPriorities();
				break;
			case statusCREATED:
			#ifdef DEBUG
				printf( "SCHEDULER: A task was created\n" );
			#endif
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

static void prvSadTask( void *pvParameters )
{
	struct Message msg;
	struct TaskParameters params = *(struct TaskParameters *)pvParameters;
	msg.params = &params;

	msg.status = statusCREATED;
	xQueueSend( xSchedulerMessages, &msg, 50 );

	unsigned int sum = 0;
	int diff;
	for ( int i = 0; i < DATA_LEN; i++ )
	{
		diff = data1[i] - data2[i];
		if ( diff < 0 ) diff *= -1;
		sum += diff;
	}
#ifdef DEBUG
	printf( "TASK: SAD Calculation: %u\n", sum );
#endif

	msg.status = statusCOMPLETED;
	xQueueSend( xSchedulerMessages, &msg, 50 );

	for ( ;; );
}

static void prvDataTask( void *pvParameters )
{
	struct Message msg;
	struct TaskParameters params = *(struct TaskParameters *)pvParameters;
	msg.params = &params;

	msg.status = statusCREATED;
	xQueueSend( xSchedulerMessages, &msg, 50 );

	struct TaskParameters sadParams;

	srand(0);

	while ( xTaskGetTickCount() < 20 )
	{
		for ( int i = 0; i < DATA_LEN; i++ )
		{
			data1[i] = rand() & 0xF;
			data2[i] = rand() & 0xF;
		}

	#ifdef DEBUG
		printf( "TASK: Data 1: " );
		for ( int i = 0; i < DATA_LEN; i++ )
		{
			printf( "%d ", data1[i] );
		}
		printf( "\n" );

		printf( "TASK: Data 2: " );
		for ( int i = 0; i < DATA_LEN; i++ )
		{
			printf( "%d ", data2[i] );
		}
		printf( "\n" );
	#endif

		sadParams.deadline = xTaskGetTickCount() + 10;
		xTaskCreate( prvSadTask,
					 "sad",
					 configMINIMAL_STACK_SIZE,
					 &sadParams,
					 mainNEW_TASK_PRIORITY,
					 &sadParams.handle );
	}

	msg.status = statusCOMPLETED;
	xQueueSend( xSchedulerMessages, &msg, 50 );

	for ( ;; );
}

static void prvTestTask( void *pvParameters )
{
	struct Message msg;
	struct TaskParameters params = *(struct TaskParameters *)pvParameters;
	msg.params = &params;

	msg.status = statusCREATED;
	xQueueSend( xSchedulerMessages, &msg, 50 );

	msg.status = statusCOMPLETED;
	xQueueSend( xSchedulerMessages, &msg, 50 );

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
			vTaskPrioritySet( taskArray[i]->handle, mainRUNNING_PRIORITY );
		}
		else
		{
			vTaskPrioritySet( taskArray[i]->handle, mainWAITING_PRIORITY );
		}
	}
}