/*
 * TaskPriorities.h
 *
 *  Created on: Jul 4, 2011
 *      Author: david
 */

#ifndef TASKPRIORITIES_H_
#define TASKPRIORITIES_H_

#include <constants.h>

#define POOLCOPILOT_PRIO		48
#define XML_SERVER_PRIO			47
#define POOLCOP_SERIAL_PRIO		51
#define CONSOLE_PRIO			52
#define DATAWATCHDOG_PRIO		53
#define WATCHDOG_PRIO			62		// must be lowest priority user thread

#endif /* TASKPRIORITIES_H_ */
