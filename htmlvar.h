/*
 * htmlvar.h
 *
 *  Created on: Jun 4, 2011
 *      Author: david
 */

#ifndef HTMLVAR_H_
#define HTMLVAR_H_

#include <constants.h>
#include <system.h>
#include <startnet.h>
#include "PoolCop.h"

const char * FooWithParameters(int fd, int v);
extern void WriteHtmlVariable(int fd, float val);

#endif /* HTMLVAR_H_ */
