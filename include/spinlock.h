#pragma once

int * spinlock_create();
void spinlock_acquire(int * lock);
void spinlock_release(int * lock);