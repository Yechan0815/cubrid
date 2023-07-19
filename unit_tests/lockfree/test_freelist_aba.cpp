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

		std::cout << "free uninit: " << entry_ptr << std::endl;

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
		LF_TRAN_ENTRY *threads[101];
		LK_ENTRY *block;
		int i, j;
		
		/* transaction systerm pointer, thread number */
		if (lf_tran_system_init (&transys, 101) != NO_ERROR)
		{
			std::cout << "failed to initialize the tran system" << std::endl;
			return -1;
		}

		/* freelist pointer, inital set, block number (set size), LF_ENTRY_DESCRIPTOR *, transystem pointer*/
		if (lf_freelist_init (&freelist, 100, 100, &desc_test, &transys) != NO_ERROR)
		{
			std::cout << "failed to initialize the freelist" << std::endl;
			return -1;
		}

		for (i = 0; i < 100; i++)
		{
			/* get the transaction entry for thread */
			threads[i] = lf_tran_request_entry (&transys);
		}

		std::cout << "alloc_cnt: " << freelist.alloc_cnt << std::endl;
		std::cout << "available_cnt: " << freelist.available_cnt << std::endl;
		std::cout << "retired_cnt: " << freelist.retired_cnt << std::endl;

		/* allocate the 99 block to thread 0 and retire */
		for (i = 0; i < 100; i++)
		{
			for (j = 0; j < 99; j++)
			{
				block = (LK_ENTRY *) lf_freelist_claim (threads[i], &freelist);
				std::cout << "th" << i << ": claim block: address: " << block << std::endl;

				lf_freelist_retire (threads[i], &freelist, block);
				std::cout << "th" << i << ": retire" << std::endl;
			}
		}

		std::cout << "alloc_cnt: " << freelist.alloc_cnt << std::endl;
		std::cout << "available_cnt: " << freelist.available_cnt << std::endl;
		std::cout << "retired_cnt: " << freelist.retired_cnt << std::endl;
	}
} // namespace name
