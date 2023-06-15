#include "lock_free.h"
#include "lock_manager.h"
#include "porting.h"
#include "lock_free.h"
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <iostream>

namespace test_lockfree
{
	LF_TRAN_SYSTEM transys = LF_TRAN_SYSTEM_INITIALIZER;
	LF_FREELIST freelist = LF_FREELIST_INITIALIZER;

	static void *
	lock_alloc_entry (void)
	{
		LK_ENTRY *res;
		
		res = (LK_ENTRY *) malloc (sizeof (LK_ENTRY));
		std::cout << "allocation freelist: " << res << std::endl;
		return res;
	}

	static int
	lock_dealloc_entry (void *res)
	{
		std::cout << "free freelist: " << res << std::endl;
		free (res);
		return NO_ERROR;
	}

	static int
	lock_init_entry (void *entry)
	{
		LK_ENTRY *entry_ptr = (LK_ENTRY *) entry;
		if (entry_ptr != NULL)
		{
			return NO_ERROR;
		}
		else
		{
			assert (false);
			return ER_FAILED;
		}
	}

	static int
	lock_uninit_entry (void *entry)
	{
		LK_ENTRY *entry_ptr = (LK_ENTRY *) entry;

		if (entry_ptr == NULL)
		{
			return ER_FAILED;
		}

		entry_ptr->tran_index = -1;
		entry_ptr->thrd_entry = NULL;

		return NO_ERROR;
	}

	LF_ENTRY_DESCRIPTOR desc_test = {
		offsetof (LK_ENTRY, stack),
		offsetof (LK_ENTRY, next),
		offsetof (LK_ENTRY, del_id),
		0,				/* does not have a key, not used in a hash table */
		0,				/* does not have a mutex, protected by resource mutex */
		LF_EM_NOT_USING_MUTEX,
		lock_alloc_entry,
		lock_dealloc_entry,
		lock_init_entry,
		lock_uninit_entry,
		NULL,
		NULL,
		NULL,				/* no key */
		NULL				/* no inserts */
	};

    int test_freelist_aba ()
	{
		LF_TRAN_ENTRY *thread0;
		LK_ENTRY *test0;
		int i;
		
		/* transaction systerm pointer, thread number */
		if (lf_tran_system_init (&transys, 2) != NO_ERROR)
		{
			std::cout << "failed to initialize the tran system" << std::endl;
			return -1;
		}

		/* freelist pointer, inital block, block size, LF_ENTRY_DESCRIPTOR *, transystem pointer*/
		if (lf_freelist_init (&freelist, 1, 4, &desc_test, &transys) != NO_ERROR)
		{
			std::cout << "failed to initialize the freelist" << std::endl;
			return -1;
		}

		thread0 = lf_tran_request_entry (&transys);
		
		for (i = 0; i < 6; i++)
		{
			std::cout << "claim" << std::endl;
			test0 = (LK_ENTRY *) lf_freelist_claim (thread0, &freelist);
			std::cout << "block " << i << ": " << test0 << std::endl;
			/* retire */
			std::cout << "retire" << std::endl;
			lf_freelist_retire (thread0, &freelist, test0);
		}
	}
} // namespace name
