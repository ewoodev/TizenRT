/****************************************************************************
 *
 * Copyright 2018 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <signal.h>
#include <sched.h>
#include <stdbool.h>

#include "signal/signal.h"
/****************************************************************************
 * Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Global Variables
 ****************************************************************************/

/****************************************************************************
 * Private Variables
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sig_is_handler_registered
 *
 * Description:
 *   Return true when the target task currently has a sigaction queue entry
 *   for the requested signal number.
 *
 * Parameters:
 *   tcb   - Target task to inspect
 *   signo - Signal number to look up
 *
 * Return Value:
 *   true when an action entry exists; false otherwise.
 *
 ****************************************************************************/

bool sig_is_handler_registered(struct tcb_s *tcb, int signo)
{
	if (tcb == NULL || !GOOD_SIGNO(signo)) {
		return false;
	}

	return sig_findaction(tcb, signo) != NULL;
}
