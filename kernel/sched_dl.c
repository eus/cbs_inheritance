/*
 * Deadline Scheduling Class (SCHED_DEADLINE)
 *
 * Earliest Deadline First (EDF) + Constant Bandwidth Server (CBS).
 *
 * Tasks that periodically executes their instances for less than their
 * runtime won't miss any of their deadlines.
 * Tasks that are not periodic or sporadic or that tries to execute more
 * than their reserved bandwidth will be slowed down (and may potentially
 * miss some of their deadlines), and won't affect any other task.
 *
 * Copyright (C) 2010 Dario Faggioli <raistlin@linux.it>,
 *                    Michael Trimarchi <trimarchimichael@yahoo.it>,
 *                    Fabio Checconi <fabio@gandalf.sssup.it>
 * Copyright (C) 2011 Tadeus Prastowo <eus@member.fsf.org>
 *
 * CAUTION: Unless otherwise mentioned, all functions in this file expect the
 * runqueue lock to be held and irq is disabled (i.e., no preemption too).
 */

static int dl_time_before(u64 a, u64 b) __attribute__((__noinline__));
static struct task_struct *dl_task_of(struct sched_dl_entity *dl_se)
	__attribute__((__noinline__));
static struct rq *rq_of_dl_rq(struct dl_rq *dl_rq)
	__attribute__((__noinline__));
static struct dl_rq *dl_rq_of_se(struct sched_dl_entity *dl_se)
	__attribute__((__noinline__));
static int on_dl_rq(struct sched_dl_entity *dl_se)
	__attribute__((__noinline__));
static int task_hosts_cbs(struct task_struct *p, struct sched_dl_entity *dl_se)
	__attribute__((__noinline__));
static struct sched_dl_entity *effective_cbs(struct task_struct *p)
	__attribute__((__noinline__));
static void set_effective_cbs(struct task_struct *p,
			      struct sched_dl_entity *effective_cbs)
	__attribute__((__noinline__));
static void zap_effective_cbs(struct task_struct *p)
	__attribute__((__noinline__));
static void get_cbs(struct sched_dl_entity *cbs)
	__attribute__((__noinline__));
static void put_cbs(struct sched_dl_entity *cbs)
	__attribute__((__noinline__));
static struct task_struct *cbs_queue_head(struct sched_dl_entity *cbs)
	__attribute__((__noinline__));
static int cbs_queue_empty(struct sched_dl_entity *cbs)
	__attribute__((__noinline__));
static struct cbs_queue_entry *cbs_enqueue(struct task_struct *p,
					   struct sched_dl_entity *cbs)
	__attribute__((__noinline__));
static void cbs_dequeue(struct cbs_queue_entry *entry,
			struct sched_dl_entity *cbs)
	__attribute__((__noinline__));
static struct cbs_membership_entry *
cbs_membership_add(struct sched_dl_entity *cbs, struct task_struct *p,
		   int to_head)
	__attribute__((__noinline__));
static void cbs_membership_del(struct cbs_membership_entry *membership)
	__attribute__((__noinline__));
static struct cbs_membership_entry *cbs_membership_first(struct task_struct *p)
	__attribute__((__noinline__));
static struct cbs_membership_entry *cbs_membership_last(struct task_struct *p)
	__attribute__((__noinline__));
static int cbs_membership_empty(struct task_struct *p)
	__attribute__((__noinline__));
static struct cbs_membership_entry *
associate_task_and_cbs(struct task_struct *p, struct sched_dl_entity *cbs)
	__attribute__((__noinline__));
static void disassociate_task_and_cbs(struct task_struct *p,
				      struct cbs_membership_entry *membership)
	__attribute__((__noinline__));
static void disassociate_task_and_all_cbs(struct task_struct *p)
	__attribute__((__noinline__));
void associate_task_and_its_cbs(struct task_struct *p)
	__attribute__((__noinline__));
void disassociate_task_and_its_cbs(struct task_struct *p)
	__attribute__((__noinline__));
static struct rq *cbs_rq_lock(struct sched_dl_entity *cbs, unsigned long *flags)
	__acquires(rq->lock) __attribute__((__noinline__));
static void cbs_rq_unlock(struct rq *rq, unsigned long *flags)
	__releases(rq->lock) __attribute__((__noinline__));
static void setup_new_dl_entity(struct sched_dl_entity *dl_se)
	__attribute__((__noinline__));
static void replenish_dl_entity(struct sched_dl_entity *dl_se)
	__attribute__((__noinline__));
static bool dl_entity_overflow(struct sched_dl_entity *dl_se, u64 t)
	__attribute__((__noinline__));
static void update_dl_entity(struct sched_dl_entity *dl_se)
	__attribute__((__noinline__));
static int start_dl_timer(struct sched_dl_entity *dl_se)
	__attribute__((__noinline__));
static enum hrtimer_restart dl_task_timer(struct hrtimer *timer)
	__attribute__((__noinline__));
static void init_dl_task_timer(struct sched_dl_entity *dl_se)
	__attribute__((__noinline__));
static int dl_runtime_exceeded(struct rq *rq, struct sched_dl_entity *dl_se)
	__attribute__((__noinline__));
static void update_curr_dl(struct rq *rq) __attribute__((__noinline__));
static void __enqueue_dl_entity(struct sched_dl_entity *dl_se)
	__attribute__((__noinline__));
static void __dequeue_dl_entity(struct sched_dl_entity *dl_se)
	__attribute__((__noinline__));
static void
enqueue_dl_entity(struct sched_dl_entity *dl_se, int flags)
	__attribute__((__noinline__));
static void dequeue_dl_entity(struct sched_dl_entity *dl_se)
	__attribute__((__noinline__));
static void enqueue_cbs(struct rq *rq, struct sched_dl_entity *cbs, int flags)
	__attribute__((__noinline__));
static void __enqueue_task_dl(struct rq *rq, struct task_struct *p, int flags,
			      struct cbs_membership_entry *membership)
	__attribute__((__noinline__));
static void enqueue_task_dl(struct rq *rq, struct task_struct *p, int flags)
	__attribute__((__noinline__));
static void dequeue_cbs(struct rq *rq, struct sched_dl_entity *cbs, int flags)
	__attribute__((__noinline__));
static void __dequeue_task_dl(struct rq *rq, int flags,
			      struct cbs_membership_entry *membership)
	__attribute__((__noinline__));
static void dequeue_task_dl(struct rq *rq, struct task_struct *p, int flags)
	__attribute__((__noinline__));
static void yield_task_dl(struct rq *rq) __attribute__((__noinline__));
static int should_preempt_curr(struct task_struct *curr,
			       struct sched_dl_entity *cbs)
	__attribute__((__noinline__));
static void check_preempt_curr_cbs(struct rq *rq, struct sched_dl_entity *cbs,
				   int flags) __attribute__((__noinline__));
static void check_preempt_curr_dl(struct rq *rq, struct task_struct *p,
				  int flags) __attribute__((__noinline__));
#ifdef CONFIG_SCHED_HRTICK
static void start_hrtick_dl(struct rq *rq, struct task_struct *p)
	__attribute__((__noinline__));
#else
static void start_hrtick_dl(struct rq *rq, struct task_struct *p)
	__attribute__((__always_inline__));
#endif
static struct sched_dl_entity *pick_next_dl_entity(struct rq *rq,
						   struct dl_rq *dl_rq)
	__attribute__((__noinline__));
struct task_struct *pick_next_task_dl(struct rq *rq)
	__attribute__((__noinline__));
static void put_prev_task_dl(struct rq *rq, struct task_struct *p)
	__attribute__((__noinline__));
static void task_tick_dl(struct rq *rq, struct task_struct *p, int queued)
	__attribute__((__noinline__));
static void task_fork_dl(struct task_struct *p) __attribute__((__noinline__));
void run_cbs_gc(struct task_struct *p) __attribute__((__noinline__));
static void task_dead_dl(struct task_struct *p) __attribute__((__noinline__));
static void set_curr_task_dl(struct rq *rq) __attribute__((__noinline__));
static void switched_from_dl(struct rq *rq, struct task_struct *p,
			     int running) __attribute__((__noinline__));
static void switched_to_dl(struct rq *rq, struct task_struct *p,
			   int running) __attribute__((__noinline__));
static void prio_changed_dl(struct rq *rq, struct task_struct *p,
			    int oldprio, int running)
	__attribute__((__noinline__));
#ifdef CONFIG_SMP
static int
select_task_rq_dl(struct rq *rq, struct task_struct *p, int sd_flag, int flags)
	__attribute__((__noinline__));
static void set_cpus_allowed_dl(struct task_struct *p,
				const struct cpumask *new_mask)
	__attribute__((__noinline__));
#endif
static struct bwi_history_entry *bwi_history_add(struct task_struct *desc,
						 struct task_struct *p)
	__attribute__((__noinline__));
static void bwi_history_del(struct bwi_history_entry *hist_entry)
	__attribute__((__noinline__));
static void bwi_extender(struct task_struct *parent, int count,
			 struct bwi_history_entry *hist_entry)
	__attribute__((__noinline__));
static void bwi_shortener(struct cbs_membership_entry *membership)
	__attribute__((__noinline__));
static int bwi_bft(unsigned depth, struct task_struct *parent, int count)
	__attribute__((__noinline__));
static void bwi_dft(struct cbs_membership_entry *parent)
	__attribute__((__noinline__));
static void
bwi_give_server_direct(struct task_struct *parent,
		       struct bwi_history_entry *parent_hist_entry,
		       struct cbs_membership_entry *parent_membership,
		       struct task_struct *direct_desc)
	__attribute__((__noinline__));
static void bwi_give_server_indirect(struct task_struct *direct_desc,
				     int count) __attribute__((__noinline__));
int bwi_give_server(struct task_struct *giver, struct task_struct *recvr,
		    int *key) __attribute__((__noinline__));
static void __bwi_take_back_server(struct bwi_history_entry *hist_entry)
	__attribute__((__noinline__));
int bwi_take_back_server(struct task_struct *taker, int key)
	__attribute__((__noinline__));
static void bwi_setprio(struct task_struct *p, int prio)
	__attribute__((__noinline__));

#ifdef CONFIG_SCHED_HRTICK
/* The following is in nanosecond */
#define SCHED_HRTICK_SMALLEST 10000
#endif

static int dl_time_before(u64 a, u64 b)
{
	return (s64)(a - b) < 0;
}

/* Return the task that hosts the CBS. */
static struct task_struct *dl_task_of(struct sched_dl_entity *dl_se)
{
	return container_of(dl_se, struct task_struct, dl);
}

static struct rq *rq_of_dl_rq(struct dl_rq *dl_rq)
{
	return container_of(dl_rq, struct rq, dl);
}

static struct dl_rq *dl_rq_of_se(struct sched_dl_entity *dl_se)
{
	struct task_struct *p = dl_task_of(dl_se);
	struct rq *rq = task_rq(p);

	return &rq->dl;
}

static int on_dl_rq(struct sched_dl_entity *dl_se)
{
	return !RB_EMPTY_NODE(&dl_se->rb_node);
}

/* Iterate the membership list of a task. */
#define cbs_membership_for_each(pos, p)				\
	list_for_each_entry(pos, &p->dl.cbs_membership, node)

/* Iterate the membership list of a task safe against membership removal. */
#define cbs_membership_for_each_safe(pos, n, p)				\
	list_for_each_entry_safe(pos, n, &p->dl.cbs_membership, node)

/* Iterate backward the membership list of a task. */
#define cbs_membership_for_each_reverse(pos, p)				\
	list_for_each_entry_reverse(pos, &p->dl.cbs_membership, node)

/* Iterate the BWI history list of a task. */
#define bwi_history_for_each(pos, p)				\
	list_for_each_entry(pos, &p->dl.bwi_history, node)

/* Iterate the BWI history list of a task safe against entry removal. */
#define bwi_history_for_each_safe(pos, n, p)				\
	list_for_each_entry_safe(pos, n, &p->dl.bwi_history, node)

/*
 * Return non-zero if the BWI history is empty. Otherwise, return zero.
 */
static int bwi_history_empty(struct task_struct *p)
{
	return list_empty(&p->dl.bwi_history);
}

/* Return non-zero if the CBS is the task's hosted CBS. */
static int task_hosts_cbs(struct task_struct *p, struct sched_dl_entity *dl_se)
{
	return (p == dl_task_of(dl_se));
}

/* Return the task's effective CBS. */
static struct sched_dl_entity *effective_cbs(struct task_struct *p)
{
	return p->dl.effective_cbs;
}

/* Set the task's effective CBS. */
static void set_effective_cbs(struct task_struct *p,
			      struct sched_dl_entity *effective_cbs)
{
	p->dl.effective_cbs = effective_cbs;
}

/* Nullify the task's effective CBS. */
static void zap_effective_cbs(struct task_struct *p)
{
	p->dl.effective_cbs = NULL;
}

/* Increment the CBS refcount. */
static void get_cbs(struct sched_dl_entity *cbs)
{
	if (cbs->nr_task == 0
	    && !list_empty(&current->dl.cbs_gc_list)
	    && list_first_entry(&current->dl.cbs_gc_list,
				struct sched_dl_entity, gc_node) == cbs) {
		/*
		 * Current goes from SCHED_DL to ~SCHED_DL and back
		 * to SCHED_DL
		 */
		list_del_init(current->dl.cbs_gc_list.next);
	} else
		get_task_struct(dl_task_of(cbs));

	cbs->nr_task++;
}

/*
 * Decrement the CBS refcount. If the refcount is zero, the CBS should be
 * assumed to have been destroyed.
 */
static void put_cbs(struct sched_dl_entity *cbs)
{
	BUG_ON(cbs->nr_task == 0);

	cbs->nr_task--;

	if (cbs->nr_task == 0
	    && hrtimer_active(&cbs->dl_timer)
	    && hrtimer_try_to_cancel(&cbs->dl_timer) == -1)
		if (task_hosts_cbs(current, cbs))
			list_add(&cbs->gc_node, &current->dl.cbs_gc_list);
		else
			list_add_tail(&cbs->gc_node, &current->dl.cbs_gc_list);
	else
		put_task_struct(dl_task_of(cbs));
}

/*
 * Return a task at the head of the CBS queue. If the queue is empty,
 * NULL is returned.
 */
static struct task_struct *cbs_queue_head(struct sched_dl_entity *cbs)
{
	if (list_empty(&cbs->cbs_queue))
		return NULL;

	return list_first_entry(&cbs->cbs_queue, struct cbs_queue_entry,
				node)->task;
}

/*
 * Return non-zero if the CBS queue is empty. Otherwise, return zero.
 */
static int cbs_queue_empty(struct sched_dl_entity *cbs)
{
	return list_empty(&cbs->cbs_queue);
}

/*
 * Enqueue a task to the CBS queue. This will add an entry to the CBS queue.
 */
static struct cbs_queue_entry *cbs_enqueue(struct task_struct *p,
					   struct sched_dl_entity *cbs)
{
	struct cbs_queue_entry *queue_entry;
	queue_entry = kmalloc(sizeof(*queue_entry), GFP_ATOMIC);

	queue_entry->task = p;
	list_add_tail(&queue_entry->node, &cbs->cbs_queue);

	return queue_entry;
}

/*
 * Dequeue a task from the CBS queue. This will delete an entry from
 * the CBS queue.
 */
static void cbs_dequeue(struct cbs_queue_entry *entry,
			struct sched_dl_entity *cbs)
{
	list_del(&entry->node);
	kfree(entry);
}

/* Add a CBS membership entry to a task's CBS membership list. */
static struct cbs_membership_entry *
cbs_membership_add(struct sched_dl_entity *cbs, struct task_struct *p,
		   int to_head)
{
	struct cbs_membership_entry *membership_entry;
	membership_entry = kmalloc(sizeof(*membership_entry), GFP_ATOMIC);

	membership_entry->cbs = cbs;
	membership_entry->entry_at_cbs = NULL;
	INIT_LIST_HEAD(&membership_entry->downstreams);
	INIT_LIST_HEAD(&membership_entry->downstream_node);
	INIT_LIST_HEAD(&membership_entry->chain_node);
	if (to_head)
		list_add(&membership_entry->node, &p->dl.cbs_membership);
	else
		list_add_tail(&membership_entry->node, &p->dl.cbs_membership);

	return membership_entry;
}

/* Delete a CBS membership entry from a task's CBS membership list. */
static void cbs_membership_del(struct cbs_membership_entry *membership)
{
	BUG_ON(membership->entry_at_cbs != NULL);

	BUG_ON(!list_empty(&membership->downstreams));
	BUG_ON(!list_empty(&membership->downstream_node));
	BUG_ON(!list_empty(&membership->chain_node));

	list_del(&membership->node);
	kfree(membership);
}

/*
 * Return the first entry of the membership list. NULL is returned if
 * the membership list is empty.
 */
static struct cbs_membership_entry *cbs_membership_first(struct task_struct *p)
{
	struct list_head *hd = &p->dl.cbs_membership;

	if (list_empty(hd))
		return NULL;

	return list_first_entry(hd, struct cbs_membership_entry, node);
}

/*
 * Return the last entry added to the membership list. NULL is
 * returned if the membership list is empty.
 */
static struct cbs_membership_entry *cbs_membership_last(struct task_struct *p)
{
	struct list_head *hd = &p->dl.cbs_membership;

	if (list_empty(hd))
		return NULL;

	return list_entry(hd->prev, struct cbs_membership_entry, node);
}

/* Return non-zero if the CBS membership entry of a task is empty. */
static int cbs_membership_empty(struct task_struct *p)
{
	return list_empty(&p->dl.cbs_membership);
}

/*
 * Make a task eligible to run on a CBS. This will add an element to the task's
 * CBS membership list.
 */
static struct cbs_membership_entry *
associate_task_and_cbs(struct task_struct *p, struct sched_dl_entity *cbs)
{
	get_cbs(cbs);
	return cbs_membership_add(cbs, p, 0);
}

/*
 * Make a task ineligible to run on a CBS on which the task was eligible to.
 * This will delete an element from the task's CBS membership list.
 */
static void disassociate_task_and_cbs(struct task_struct *p,
				      struct cbs_membership_entry *membership)
{
	struct sched_dl_entity *cbs = membership->cbs;

	list_del_init(&membership->downstream_node);
	list_del_init(&membership->chain_node);

	cbs_membership_del(membership);
	put_cbs(cbs);
}

/* 
 * Make a task ineligible to run on all CBSes on which the task was eligible
 * to. This will delete all elements from the task's CBS membership list.
 */
static void disassociate_task_and_all_cbs(struct task_struct *p)
{
	struct cbs_membership_entry *membership, *next;

	cbs_membership_for_each_safe(membership, next, p) {
		disassociate_task_and_cbs(p, membership);
	}
}

void associate_task_and_its_cbs(struct task_struct *p)
{
	struct cbs_membership_entry *membership;
	struct bwi_history_entry *hist_entry;
	struct sched_dl_entity *cbs = &p->dl;

	get_cbs(cbs);
	membership = cbs_membership_add(cbs, p, 1);

	bwi_history_for_each(hist_entry, p) {
		if (!hist_entry->has_inherited_hosted_cbs) {
			bwi_give_server_direct(p, hist_entry, membership,
					       hist_entry->desc);
			bwi_give_server_indirect(hist_entry->desc, 1);
			hist_entry->has_inherited_hosted_cbs = 1;
		}
	}
}

void disassociate_task_and_its_cbs(struct task_struct *p)
{
	struct cbs_membership_entry *membership = cbs_membership_first(p);
	struct sched_dl_entity *cbs = &p->dl;

	BUG_ON(cbs != membership->cbs);

	cbs_membership_del(membership);
	put_cbs(cbs);
}

static struct rq *cbs_rq_lock(struct sched_dl_entity *cbs, unsigned long *flags)
	__acquires(rq->lock)
{
	struct rq *rq = cbs->cbs_rq;

	raw_spin_lock_irqsave(&rq->lock, *flags);

	return rq;
}

static void cbs_rq_unlock(struct rq *rq, unsigned long *flags)
	__releases(rq->lock)
{
	raw_spin_unlock_irqrestore(&rq->lock, *flags);
}

/*
 * We are being explicitly informed that a new instance is starting,
 * and this means that:
 *  - the absolute deadline of the entity has to be placed at
 *    current time + relative deadline;
 *  - the runtime of the entity has to be set to the maximum value.
 *
 * The capability of specifying such event is useful whenever a -deadline
 * entity wants to (try to!) synchronize its behaviour with the scheduler's
 * one, and to (try to!) reconcile itself with its own scheduling
 * parameters.
 */
static void setup_new_dl_entity(struct sched_dl_entity *dl_se)
{
	struct dl_rq *dl_rq = dl_rq_of_se(dl_se);
	struct rq *rq = rq_of_dl_rq(dl_rq);

	WARN_ON(!dl_se->dl_new || dl_se->dl_throttled);

	dl_se->deadline = rq->clock + dl_se->dl_deadline;
	dl_se->runtime = dl_se->dl_runtime;
	dl_se->dl_new = 0;
}

/*
 * Pure Earliest Deadline First (EDF) scheduling does not deal with the
 * possibility of a entity lasting more than what it declared, and thus
 * exhausting its runtime.
 *
 * Here we are interested in making runtime overrun possible, but we do
 * not want a entity which is misbehaving to affect the scheduling of all
 * other entities.
 * Therefore, a budgeting strategy called Constant Bandwidth Server (CBS)
 * is used, in order to confine each entity within its own bandwidth.
 *
 * This function deals exactly with that, and ensures that when the runtime
 * of a entity is replenished, its deadline is also postponed. That ensures
 * the overrunning entity can't interfere with other entity in the system and
 * can't make them miss their deadlines. Reasons why this kind of overruns
 * could happen are, typically, a entity voluntarily trying to overcume its
 * runtime, or it just underestimated it during sched_setscheduler_ex().
 */
static void replenish_dl_entity(struct sched_dl_entity *dl_se)
{
	struct dl_rq *dl_rq = dl_rq_of_se(dl_se);
	struct rq *rq = rq_of_dl_rq(dl_rq);

	/*
	 * We Keep moving the deadline away until we get some
	 * available runtime for the entity. This ensures correct
	 * handling of situations where the runtime overrun is
	 * arbitrary large.
	 */
	while (dl_se->runtime <= 0) {
		dl_se->deadline += dl_se->dl_deadline;
		dl_se->runtime += dl_se->dl_runtime;
	}

	/*
	 * At this point, the deadline really should be "in
	 * the future" with respect to rq->clock. If it's
	 * not, we are, for some reason, lagging too much!
	 * Anyway, after having warn userspace abut that,
	 * we still try to keep the things running by
	 * resetting the deadline and the budget of the
	 * entity.
	 */
	if (dl_time_before(dl_se->deadline, rq->clock)) {
		WARN_ON_ONCE(1);
		dl_se->deadline = rq->clock + dl_se->dl_deadline;
		dl_se->runtime = dl_se->dl_runtime;
	}
}

/*
 * Here we check if --at time t-- an entity (which is probably being
 * [re]activated or, in general, enqueued) can use its remaining runtime
 * and its current deadline _without_ exceeding the bandwidth it is
 * assigned (function returns true if it can).
 *
 * For this to hold, we must check if:
 *   runtime / (deadline - t) < dl_runtime / dl_deadline .
 */
static bool dl_entity_overflow(struct sched_dl_entity *dl_se, u64 t)
{
	u64 left, right;

	/*
	 * left and right are the two sides of the equation above,
	 * after a bit of shuffling to use multiplications instead
	 * of divisions.
	 *
	 * Note that none of the time values involved in the two
	 * multiplications are absolute: dl_deadline and dl_runtime
	 * are the relative deadline and the maximum runtime of each
	 * instance, runtime is the runtime left for the last instance
	 * and (deadline - t), since t is rq->clock, is the time left
	 * to the (absolute) deadline. Therefore, overflowing the u64
	 * type is very unlikely to occur in both cases.
	 */
	left = dl_se->dl_deadline * dl_se->runtime;
	right = (dl_se->deadline - t) * dl_se->dl_runtime;

	return dl_time_before(right, left);
}

/*
 * When a -deadline entity is queued back on the runqueue, its runtime and
 * deadline might need updating.
 *
 * The policy here is that we update the deadline of the entity only if:
 *  - the current deadline is in the past,
 *  - using the remaining runtime with the current deadline would make
 *    the entity exceed its bandwidth.
 */
static void update_dl_entity(struct sched_dl_entity *dl_se)
{
	struct dl_rq *dl_rq = dl_rq_of_se(dl_se);
	struct rq *rq = rq_of_dl_rq(dl_rq);

	/*
	 * The arrival of a new instance needs special treatment, i.e.,
	 * the actual scheduling parameters have to be "renewed".
	 */
	if (dl_se->dl_new) {
		setup_new_dl_entity(dl_se);
		return;
	}

	if (dl_time_before(dl_se->deadline, rq->clock) ||
	    dl_entity_overflow(dl_se, rq->clock)) {
		dl_se->deadline = rq->clock + dl_se->dl_deadline;
		dl_se->runtime = dl_se->dl_runtime;
	}
}

/*
 * If the entity depleted all its runtime, and if we want it to sleep
 * while waiting for some new execution time to become available, we
 * set the bandwidth enforcement timer to the replenishment instant
 * and try to activate it.
 *
 * Notice that it is important for the caller to know if the timer
 * actually started or not (i.e., the replenishment instant is in
 * the future or in the past).
 */
static int start_dl_timer(struct sched_dl_entity *dl_se)
{
	struct dl_rq *dl_rq = dl_rq_of_se(dl_se);
	struct rq *rq = rq_of_dl_rq(dl_rq);
	ktime_t now, act;
	ktime_t soft, hard;
	unsigned long range;
	s64 delta;

	/*
	 * We want the timer to fire at the deadline, but considering
	 * that it is actually coming from rq->clock and not from
	 * hrtimer's time base reading.
	 */
	act = ns_to_ktime(dl_se->deadline);
	now = hrtimer_cb_get_time(&dl_se->dl_timer);
	delta = ktime_to_ns(now) - rq->clock;
	act = ktime_add_ns(act, delta);

	/*
	 * If the expiry time already passed, e.g., because the value
	 * chosen as the deadline is too small, don't even try to
	 * start the timer in the past!
	 */
	if (ktime_us_delta(act, now) < 0)
		return 0;

	hrtimer_set_expires(&dl_se->dl_timer, act);

	soft = hrtimer_get_softexpires(&dl_se->dl_timer);
	hard = hrtimer_get_expires(&dl_se->dl_timer);
	range = ktime_to_ns(ktime_sub(hard, soft));
	__hrtimer_start_range_ns(&dl_se->dl_timer, soft,
				 range, HRTIMER_MODE_ABS, 0);

	return hrtimer_active(&dl_se->dl_timer);
}

/*
 * This is the bandwidth enforcement timer callback. If here, we know
 * a CBS is not on its dl_rq, since the fact that the timer was running
 * means the task is throttled and needs a runtime replenishment.
 *
 * However, what we actually do depends on the fact the task is active,
 * (it is on its rq) or has been removed from there by a call to
 * dequeue_task_dl(). In the former case we must issue the runtime
 * replenishment and add the task back to the dl_rq; in the latter, we just
 * do nothing but clearing dl_throttled, so that runtime and deadline
 * updating (and the queueing back to dl_rq) will be done by the
 * next call to enqueue_task_dl().
 *
 * This function can run from either one of the following contexts:
 * - hard-IRQ (irq is disabled and so is preemption)
 * - soft-IRQ
 *
 * - rq lock is not held.
 */
static enum hrtimer_restart dl_task_timer(struct hrtimer *timer)
{
	unsigned long flags;
	struct sched_dl_entity *dl_se = container_of(timer,
						     struct sched_dl_entity,
						     dl_timer);
	struct rq *rq = cbs_rq_lock(dl_se, &flags);

	/*
	 * We need to take care of a possible races here. In fact, the
	 * CBS might have been destroyed.
	 */
	if (dl_se->nr_task == 0)
		goto unlock;

	dl_se->dl_throttled = 0;
	if (!cbs_queue_empty(dl_se)) {
		enqueue_cbs(rq, dl_se, ENQUEUE_REPLENISH);
		check_preempt_curr_cbs(rq, dl_se, 0);
	}
unlock:
	cbs_rq_unlock(rq, &flags);

	return HRTIMER_NORESTART;
}

static void init_dl_task_timer(struct sched_dl_entity *dl_se)
{
	struct hrtimer *timer = &dl_se->dl_timer;

	if (hrtimer_active(timer)) {
		hrtimer_try_to_cancel(timer);
		return;
	}

	hrtimer_init(timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer->function = dl_task_timer;
}

static
int dl_runtime_exceeded(struct rq *rq, struct sched_dl_entity *dl_se)
{
	int dmiss = dl_time_before(dl_se->deadline, rq->clock);
	int rorun = dl_se->runtime <= 0;

	if (!rorun && !dmiss)
		return 0;

	/*
	 * If we are beyond our current deadline and we are still
	 * executing, then we have already used some of the runtime of
	 * the next instance. Thus, if we do not account that, we are
	 * stealing bandwidth from the system at each deadline miss!
	 */
	if (dmiss) {
		dl_se->runtime = rorun ? dl_se->runtime : 0;
		dl_se->runtime -= rq->clock - dl_se->deadline;
	}

	return 1;
}

/*
 * Update the current task's runtime statistics (provided it is still
 * a -deadline task and has not been removed from the dl_rq).
 */
static void update_curr_dl(struct rq *rq)
{
	struct task_struct *curr = rq->curr;
	struct sched_dl_entity *dl_se = effective_cbs(curr);
	u64 delta_exec;

	if (!dl_task(curr) || !on_dl_rq(dl_se))
		return;

	delta_exec = rq->clock - curr->se.exec_start;
	if (unlikely((s64)delta_exec < 0))
		delta_exec = 0;

	schedstat_set(curr->se.statistics.exec_max,
		      max(curr->se.statistics.exec_max, delta_exec));

	curr->se.sum_exec_runtime += delta_exec;
	account_group_exec_runtime(curr, delta_exec);

	curr->se.exec_start = rq->clock;
	cpuacct_charge(curr, delta_exec);

	dl_se->runtime -= delta_exec;
	if (dl_runtime_exceeded(rq, dl_se)) {
		dequeue_cbs(rq, dl_se, 0);
		if (0 && likely(start_dl_timer(dl_se)))
			dl_se->dl_throttled = 1;
		else
			enqueue_cbs(rq, dl_se, ENQUEUE_REPLENISH);

		resched_task(curr);
	}
}

static void __enqueue_dl_entity(struct sched_dl_entity *dl_se)
{
	struct dl_rq *dl_rq = dl_rq_of_se(dl_se);
	struct rb_node **link = &dl_rq->rb_root.rb_node;
	struct rb_node *parent = NULL;
	struct sched_dl_entity *entry;
	int leftmost = 1;

	BUG_ON(!RB_EMPTY_NODE(&dl_se->rb_node));

	while (*link) {
		parent = *link;
		entry = rb_entry(parent, struct sched_dl_entity, rb_node);
		if (dl_time_before(dl_se->deadline, entry->deadline))
			link = &parent->rb_left;
		else {
			link = &parent->rb_right;
			leftmost = 0;
		}
	}

	if (leftmost)
		dl_rq->rb_leftmost = &dl_se->rb_node;

	rb_link_node(&dl_se->rb_node, parent, link);
	rb_insert_color(&dl_se->rb_node, &dl_rq->rb_root);

	dl_rq->dl_nr_running++;
}

static void __dequeue_dl_entity(struct sched_dl_entity *dl_se)
{
	struct dl_rq *dl_rq = dl_rq_of_se(dl_se);

	if (RB_EMPTY_NODE(&dl_se->rb_node))
		return;

	if (dl_rq->rb_leftmost == &dl_se->rb_node) {
		struct rb_node *next_node;

		next_node = rb_next(&dl_se->rb_node);
		dl_rq->rb_leftmost = next_node;
	}

	rb_erase(&dl_se->rb_node, &dl_rq->rb_root);
	RB_CLEAR_NODE(&dl_se->rb_node);

	dl_rq->dl_nr_running--;
}

static void
enqueue_dl_entity(struct sched_dl_entity *dl_se, int flags)
{
	BUG_ON(on_dl_rq(dl_se));

	/*
	 * If this is a wakeup or a new instance, the scheduling
	 * parameters of the task might need updating. Otherwise,
	 * we want a replenishment of its runtime.
	 */
	if (!dl_se->dl_new && flags & ENQUEUE_REPLENISH)
		replenish_dl_entity(dl_se);
	else
		update_dl_entity(dl_se);

	__enqueue_dl_entity(dl_se);
}

static void dequeue_dl_entity(struct sched_dl_entity *dl_se)
{
	__dequeue_dl_entity(dl_se);
}

static void enqueue_cbs(struct rq *rq, struct sched_dl_entity *cbs, int flags)
{
	/*
	 * If cbs is throttled, we do nothing. In fact, if it exhausted
	 * its budget it needs a replenishment and, since it now is on
	 * its rq, the bandwidth timer callback (which clearly has not
	 * run yet) will take care of this.
	 */
	if (cbs->dl_throttled)
		return;

	enqueue_dl_entity(cbs, flags);
}

static void __enqueue_task_dl(struct rq *rq, struct task_struct *p, int flags,
			      struct cbs_membership_entry *membership)
{
	struct sched_dl_entity *cbs = membership->cbs;

	membership->entry_at_cbs = cbs_enqueue(p, cbs);

	if (!on_dl_rq(cbs))
		enqueue_cbs(rq, cbs, flags);
}

static void enqueue_task_dl(struct rq *rq, struct task_struct *p, int flags)
{
	struct cbs_membership_entry *membership;

	cbs_membership_for_each(membership, p) {
		__enqueue_task_dl(rq, p, flags, membership);
	}
}

static void dequeue_cbs(struct rq *rq, struct sched_dl_entity *cbs, int flags)
{
	dequeue_dl_entity(cbs);
}

static void __dequeue_task_dl(struct rq *rq, int flags,
			      struct cbs_membership_entry *membership)
{
	struct sched_dl_entity *cbs = membership->cbs;

	cbs_dequeue(membership->entry_at_cbs, cbs);
	membership->entry_at_cbs = NULL;

	if (cbs_queue_empty(cbs))
		dequeue_cbs(rq, cbs, flags);
}

static void dequeue_task_dl(struct rq *rq, struct task_struct *p, int flags)
{
	struct cbs_membership_entry *membership;

	update_curr_dl(rq);

	cbs_membership_for_each(membership, p) {
		__dequeue_task_dl(rq, flags, membership);
	}

	if (unlikely(p->state == TASK_DEAD)) {
		struct bwi_history_entry *hist_entry, *hist_next;

		bwi_history_for_each_safe(hist_entry, hist_next, p)
			__bwi_take_back_server(hist_entry);

		disassociate_task_and_all_cbs(p);
	}
}

/*
 * Yield task semantic for -deadline tasks is:
 *
 *   get off from the CPU until our next instance, with
 *   a new runtime.
 *
 * But, when the original CBS replenishment behavior is used, the
 * semantic of yield task becomes:
 *
 *   postpone the deadline and have a new runtime without getting off
 *   from the CPU.
 */
static void yield_task_dl(struct rq *rq)
{
	struct sched_dl_entity *cbs = effective_cbs(rq->curr);

	/*
	 * We make the task go to sleep until its current deadline by
	 * forcing its runtime to zero. This way, update_curr_dl() stops
	 * it and the bandwidth timer will wake it up and will give it
	 * new scheduling parameters (thanks to dl_new=1).
	 */
	if (cbs->runtime > 0) {
		cbs->dl_new = 1;
		cbs->runtime = 0;
	}
	update_curr_dl(rq);
}

static int should_preempt_curr(struct task_struct *curr,
			       struct sched_dl_entity *cbs)
{
	return (!dl_task(curr)
		|| (dl_time_before(cbs->deadline,
				   effective_cbs(curr)->deadline)
		    && effective_cbs(curr) != cbs));
}

static void check_preempt_curr_cbs(struct rq *rq, struct sched_dl_entity *cbs,
				   int flags)
{
	if (should_preempt_curr(rq->curr, cbs))
		resched_task(rq->curr);
}

static void check_preempt_curr_dl(struct rq *rq, struct task_struct *p,
				  int flags)
{
	struct cbs_membership_entry *membership;

	if (!dl_task(p))
		return;

	cbs_membership_for_each(membership, p) {
		struct sched_dl_entity *cbs = membership->cbs;

		if (cbs->dl_throttled
		    || membership->entry_at_cbs == NULL /* p can't use this */
		    || cbs_queue_head(cbs) != p)
			continue;

		if (should_preempt_curr(rq->curr, cbs)) {
			resched_task(rq->curr);
			break;
		}
	}
}

#ifdef CONFIG_SCHED_HRTICK
static void start_hrtick_dl(struct rq *rq, struct task_struct *p)
{
	struct sched_dl_entity *cbs = effective_cbs(p);

	if (cbs->runtime > SCHED_HRTICK_SMALLEST)
		hrtick_start(rq, cbs->runtime);
	else {
		replenish_dl_entity(cbs);
		resched_task(p);
	}
}
#else
static void start_hrtick_dl(struct rq *rq, struct task_struct *p)
{
}
#endif

static struct sched_dl_entity *pick_next_dl_entity(struct rq *rq,
						   struct dl_rq *dl_rq)
{
	struct rb_node *left = dl_rq->rb_leftmost;

	if (!left)
		return NULL;

	return rb_entry(left, struct sched_dl_entity, rb_node);
}

struct task_struct *pick_next_task_dl(struct rq *rq)
{
	struct sched_dl_entity *dl_se;
	struct task_struct *p;
	struct dl_rq *dl_rq;

	dl_rq = &rq->dl;

	if (unlikely(!dl_rq->dl_nr_running))
		return NULL;

	dl_se = pick_next_dl_entity(rq, dl_rq);
	BUG_ON(!dl_se);

	p = cbs_queue_head(dl_se);
	BUG_ON(p == NULL);

	set_effective_cbs(p, dl_se);

	p->se.exec_start = rq->clock;
#ifdef CONFIG_SCHED_HRTICK
	if (hrtick_enabled(rq))
		start_hrtick_dl(rq, p);
#endif
	return p;
}

static void put_prev_task_dl(struct rq *rq, struct task_struct *p)
{
	update_curr_dl(rq);
	p->se.exec_start = 0;
	zap_effective_cbs(p);
}

static void task_tick_dl(struct rq *rq, struct task_struct *p, int queued)
{
	update_curr_dl(rq);

#ifdef CONFIG_SCHED_HRTICK
	if (hrtick_enabled(rq) && queued && effective_cbs(p)->runtime > 0)
		start_hrtick_dl(rq, p);
#endif
}

/*
 * - rq->lock is not held
 * - interrupt is not disabled
 * - preemption is disabled due to fn get_cpu in fn sched_fork
 *
 * - p is not yet in the runqueue
 */
static void task_fork_dl(struct task_struct *p)
{
	struct rq *rq = this_rq();
	unsigned long flags;

	/*
	 * The child of a -deadline task will be SCHED_DEADLINE, but
	 * as a throttled task. This means the parent (or someone else)
	 * must call sched_setscheduler_ex() on it, or it won't even
	 * start.
	 */
	p->dl.dl_throttled = 1;
	p->dl.dl_new = 0;

	raw_spin_lock_irqsave(&rq->lock, flags);
	associate_task_and_its_cbs(p);
	raw_spin_unlock_irqrestore(&rq->lock, flags);
}

/*
 * - must be called from task_dead scheduling interface callback
 * - rq->lock must *not* be held
 */
void run_cbs_gc(struct task_struct *p)
{
	struct sched_dl_entity *cbs;

	/*
	 * No need to modify the link list since the list will be destroyed
	 * along with the task.
	 */
	list_for_each_entry(cbs, &p->dl.cbs_gc_list, gc_node) {
		hrtimer_cancel(&cbs->dl_timer);
		put_task_struct(dl_task_of(cbs));
	}
}

/*
 * - rq->lock is not held
 * - interrupt is not disabled
 * - preemption is not disabled
 */
static void task_dead_dl(struct task_struct *p)
{
	/*
	 * We are not holding any lock here, so it is safe to
	 * wait for the bandwidth timer to be removed.
	 */
	run_cbs_gc(p);
}

static void set_curr_task_dl(struct rq *rq)
{
	struct task_struct *p = rq->curr;

	p->se.exec_start = rq->clock;

	/*
	 * This can only be invoked from either __sched_setscheduler or
	 * rt_mutex_setprio. For the former case, there are two
	 * possibilities:
	 *
	 * Possibility I: curr switches from ~SCHED_DL to SCHED_DL (i.e.,
	 * activating its own CBS). In this case, curr does not have an
	 * effective CBS unless curr is running on another CBS under
	 * bandwidth inheritance. When curr has no effective CBS, curr's
	 * effective CBS should be set to its own CBS.
	 *
	 * Possibility II: curr adjusts its CBS bandwidth (from SCHED_DL to
	 * SCHED_DL). In this case, curr already has an effective CBS.
	 *
	 * For the latter case, currently it is not considered yet.
	 */
	if (effective_cbs(p) == NULL)
		set_effective_cbs(p, &p->dl);
}

static void switched_from_dl(struct rq *rq, struct task_struct *p,
			     int running)
{
}

static void switched_to_dl(struct rq *rq, struct task_struct *p,
			   int running)
{
	if (!running)
		check_preempt_curr_dl(rq, p, 0);
}

static void prio_changed_dl(struct rq *rq, struct task_struct *p,
			    int oldprio, int running)
{
	switched_to_dl(rq, p, running);
}

#ifdef CONFIG_SMP
static int
select_task_rq_dl(struct rq *rq, struct task_struct *p, int sd_flag, int flags)
{
	return task_cpu(p);
}

static void set_cpus_allowed_dl(struct task_struct *p,
				const struct cpumask *new_mask)
{
	int weight = cpumask_weight(new_mask);

	BUG_ON(!dl_task(p));

	cpumask_copy(&p->cpus_allowed, new_mask);
	p->dl.nr_cpus_allowed = weight;
}
#endif

static const struct sched_class dl_sched_class = {
	.next			= &rt_sched_class,
	.enqueue_task		= enqueue_task_dl,
	.dequeue_task		= dequeue_task_dl,
	.yield_task		= yield_task_dl,

	.check_preempt_curr	= check_preempt_curr_dl,

	.pick_next_task		= pick_next_task_dl,
	.put_prev_task		= put_prev_task_dl,

#ifdef CONFIG_SMP
	.select_task_rq		= select_task_rq_dl,

	.set_cpus_allowed       = set_cpus_allowed_dl,
#endif

	.set_curr_task		= set_curr_task_dl,
	.task_tick		= task_tick_dl,
	.task_fork              = task_fork_dl,
	.task_dead		= task_dead_dl,

	.prio_changed           = prio_changed_dl,
	.switched_from		= switched_from_dl,
	.switched_to		= switched_to_dl,
};

static struct bwi_history_entry *bwi_history_add(struct task_struct *desc,
						 struct task_struct *p)
{
	struct cbs_membership_entry *first_membership = cbs_membership_first(p);
	struct bwi_history_entry *entry;

	entry = kmalloc(sizeof(*entry), GFP_ATOMIC);
	entry->desc = desc;
	entry->has_inherited_hosted_cbs = (first_membership != NULL
					   && &p->dl == first_membership->cbs);
	INIT_LIST_HEAD(&entry->bwi_chains);
	if (list_empty(&p->dl.bwi_history)) {
		entry->id = 0;
	} else {
		entry->id = (container_of(p->dl.bwi_history.prev,
					  struct bwi_history_entry, node)->id
			     + 1);
	}

	list_add_tail(&entry->node, &p->dl.bwi_history);

	return entry;
}

static void bwi_history_del(struct bwi_history_entry *hist_entry)
{
	BUG_ON(!list_empty(&hist_entry->bwi_chains));

	list_del(&hist_entry->node);
	kfree(hist_entry);
}

static void bwi_extender(struct task_struct *parent, int count,
			 struct bwi_history_entry *hist_entry)
{
	struct cbs_membership_entry *membership;

	cbs_membership_for_each_reverse(membership, parent) {
		struct cbs_membership_entry *e;

		if (count-- == 0)
			return;

		e = associate_task_and_cbs(hist_entry->desc, membership->cbs);
		list_add_tail(&e->chain_node, &hist_entry->bwi_chains);
		list_add_tail(&e->downstream_node, &membership->downstreams);

		if (hist_entry->desc->se.on_rq) {
			struct rq *rq = task_rq(hist_entry->desc);
			int cbs_on_rq = on_dl_rq(e->cbs);

			__enqueue_task_dl(rq, hist_entry->desc, 0, e);

			if (!cbs_on_rq)
				check_preempt_curr_cbs(rq, e->cbs, 0);
		}
		/* TODO: In SMP, think about when hist_entry->desc is running */
	}
}

static void bwi_shortener(struct cbs_membership_entry *membership)
{
	BUG_ON(!list_empty(&membership->downstreams));

	list_del_init(&membership->chain_node);

	if (!list_empty(&membership->downstream_node))
		list_del_init(&membership->downstream_node);

	if (membership->entry_at_cbs != NULL) {
		struct rq *rq = task_rq(membership->entry_at_cbs->task);

		__dequeue_task_dl(rq, 0, membership);
	}

	cbs_membership_del(membership);
}

static int bwi_bft(unsigned depth, struct task_struct *parent, int count)
{
	int processed_task_count = 0;
	struct bwi_history_entry *hist_entry;

	if (depth == 0) {
		/* Recursion base case */
		bwi_history_for_each(hist_entry, parent) {
			bwi_extender(parent, count, hist_entry);
			processed_task_count++;
		}

	} else {
		bwi_history_for_each(hist_entry, parent) {
			processed_task_count += bwi_bft(depth - 1,
							hist_entry->desc,
							count);
		}
	}

	return processed_task_count;
}

static void bwi_dft(struct cbs_membership_entry *parent)
{
	struct cbs_membership_entry *membership, *next;

	list_for_each_entry_safe(membership, next, &parent->downstreams,
				 downstream_node)
	{
		if (!list_empty(&membership->downstreams))
			bwi_dft(membership);

		bwi_shortener(membership);
	}
}

static void
bwi_give_server_direct(struct task_struct *parent,
		       struct bwi_history_entry *parent_hist_entry,
		       struct cbs_membership_entry *parent_membership,
		       struct task_struct *direct_desc)
{
	struct cbs_membership_entry *e;

	e = associate_task_and_cbs(direct_desc, parent_membership->cbs);
	list_add_tail(&e->chain_node, &parent_hist_entry->bwi_chains);

	if (!task_hosts_cbs(parent, parent_membership->cbs))
		list_add_tail(&e->downstream_node,
			      &parent_membership->downstreams);
}

static void bwi_give_server_indirect(struct task_struct *direct_desc, int count)
{
	int depth = 0;

	while (bwi_bft(depth, direct_desc, count) != 0)
		depth++;
}

static inline void check_class_changed(struct rq *rq, struct task_struct *p,
				       const struct sched_class *prev_class,
				       int oldprio, int running);
static inline int normal_prio(struct task_struct *p);

static void bwi_setprio(struct task_struct *p, int prio)
{
	int oldprio, on_rq, running;
	struct rq *rq = this_rq();
	const struct sched_class *prev_class;

	oldprio = p->prio;
	prev_class = p->sched_class;
	on_rq = p->se.on_rq;
	running = task_current(rq, p);
	if (on_rq)
		dequeue_task(rq, p, 0);
	if (running)
		p->sched_class->put_prev_task(rq, p);

	if (dl_prio(prio))
		p->sched_class = &dl_sched_class;
	else if (rt_prio(prio))
		p->sched_class = &rt_sched_class;
	else
		p->sched_class = &fair_sched_class;

	p->prio = prio;

	if (running)
		p->sched_class->set_curr_task(rq);
	if (on_rq) {
		enqueue_task(rq, p, oldprio < prio ? ENQUEUE_HEAD : 0);

		check_class_changed(rq, p, prev_class, oldprio, running);
	}
}

int bwi_give_server(struct task_struct *giver, struct task_struct *recvr,
		    int *key)
{
	struct cbs_membership_entry *membership;
	struct bwi_history_entry *hist_entry;
	int count = 0;

	if (cbs_membership_empty(giver)) /* BWI requires at least one CBS */
		return -EAGAIN;

	if (task_cpu(giver) != task_cpu(recvr)) /* Partitioned scheduling */
		return -EINVAL;

	/* TODO: prevent circular BWI by setting up parent chain */

	hist_entry = bwi_history_add(recvr, giver);
	*key = hist_entry->id;

	cbs_membership_for_each(membership, giver) {
		bwi_give_server_direct(giver, hist_entry, membership, recvr);
		count++;
	}

	bwi_give_server_indirect(recvr, count);

	bwi_setprio(recvr, MAX_DL_PRIO-1);

	return 0;
}

static void __bwi_take_back_server(struct bwi_history_entry *hist_entry)
{
	struct cbs_membership_entry *entry_at_desc, *next;

	list_for_each_entry_safe(entry_at_desc, next, &hist_entry->bwi_chains,
				 chain_node) {

		bwi_dft(entry_at_desc);
		bwi_shortener(entry_at_desc);
	}

	bwi_history_del(hist_entry);
}

int bwi_take_back_server(struct task_struct *taker, int key)
{
	struct bwi_history_entry *hist_entry, *hist_next;
	int found = 0;

	bwi_history_for_each_safe(hist_entry, hist_next, taker) {
		/*
		 * There can be more than one hist_entry with the same ID but
		 * they are always clustered together.
		 */
		if (hist_entry->id == key) {
			struct task_struct *p = hist_entry->desc;

			if (!found)
				found = 1;

			__bwi_take_back_server(hist_entry);
			bwi_setprio(p, normal_prio(p));

		} else if (found)
			break;
	}

	if (!found)
		return -EINVAL;

	return 0;
}
