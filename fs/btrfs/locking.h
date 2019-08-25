/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2008 Oracle.  All rights reserved.
 */

#ifndef BTRFS_LOCKING_H
#define BTRFS_LOCKING_H

#include <linux/atomic.h>
#include <linux/wait.h>
#include <linux/percpu_counter.h>

#define BTRFS_WRITE_LOCK 1
#define BTRFS_READ_LOCK 2
#define BTRFS_WRITE_LOCK_BLOCKING 3
#define BTRFS_READ_LOCK_BLOCKING 4

void btrfs_tree_lock(struct extent_buffer *eb);
void btrfs_tree_unlock(struct extent_buffer *eb);

void btrfs_tree_read_lock(struct extent_buffer *eb);
void btrfs_tree_read_unlock(struct extent_buffer *eb);
void btrfs_tree_read_unlock_blocking(struct extent_buffer *eb);
void btrfs_set_lock_blocking_read(struct extent_buffer *eb);
void btrfs_set_lock_blocking_write(struct extent_buffer *eb);
void btrfs_assert_tree_locked(struct extent_buffer *eb);
int btrfs_try_tree_read_lock(struct extent_buffer *eb);
int btrfs_try_tree_write_lock(struct extent_buffer *eb);
int btrfs_tree_read_lock_atomic(struct extent_buffer *eb);


static inline void btrfs_tree_unlock_rw(struct extent_buffer *eb, int rw)
{
	if (rw == BTRFS_WRITE_LOCK || rw == BTRFS_WRITE_LOCK_BLOCKING)
		btrfs_tree_unlock(eb);
	else if (rw == BTRFS_READ_LOCK_BLOCKING)
		btrfs_tree_read_unlock_blocking(eb);
	else if (rw == BTRFS_READ_LOCK)
		btrfs_tree_read_unlock(eb);
	else
		BUG();
}


struct btrfs_drw_lock {
	atomic_t readers;
	struct percpu_counter writers;
	wait_queue_head_t pending_writers;
	wait_queue_head_t pending_readers;
};

int btrfs_drw_lock_init(struct btrfs_drw_lock *lock);
void btrfs_drw_lock_destroy(struct btrfs_drw_lock *lock);
void btrfs_drw_write_lock(struct btrfs_drw_lock *lock);
bool btrfs_drw_try_write_lock(struct btrfs_drw_lock *lock);
void btrfs_drw_write_unlock(struct btrfs_drw_lock *lock);
void btrfs_drw_read_lock(struct btrfs_drw_lock *lock);
void btrfs_drw_read_unlock(struct btrfs_drw_lock *lock);

#endif
