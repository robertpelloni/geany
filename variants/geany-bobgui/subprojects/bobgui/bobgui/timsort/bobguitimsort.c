/* Lots of code for an adaptive, stable, natural mergesort.  There are many
 * pieces to this algorithm; read listsort.txt for overviews and details.
 */

#include "config.h"

#include "bobguitimsortprivate.h"

/*
 * This is the minimum sized sequence that will be merged.  Shorter
 * sequences will be lengthened by calling binarySort.  If the entire
 * array is less than this length, no merges will be performed.
 *
 * This constant should be a power of two.  It was 64 in Tim Peter's C
 * implementation, but 32 was empirically determined to work better in
 * [Android's Java] implementation.  In the unlikely event that you set
 * this constant to be a number that's not a power of two, you'll need
 * to change the compute_min_run() computation.
 *
 * If you decrease this constant, you must change the
 * BOBGUI_TIM_SORT_MAX_PENDING value, or you risk running out of space.
 * See Python's listsort.txt for a discussion of the minimum stack
 * length required as a function of the length of the array being sorted and
 * the minimum merge sequence length.
 */
#define MIN_MERGE 32

/*
 * When we get into galloping mode, we stay there until both runs win less
 * often than MIN_GALLOP consecutive times.
 */
#define MIN_GALLOP 7

/*
 * Returns the minimum acceptable run length for an array of the specified
 * length. Natural runs shorter than this will be extended with binary sort.
 *
 * Roughly speaking, the computation is:
 *
 *  If n < MIN_MERGE, return n (it's too small to bother with fancy stuff).
 *  Else if n is an exact power of 2, return MIN_MERGE/2.
 *  Else return an int k, MIN_MERGE/2 <= k <= MIN_MERGE, such that n/k
 *   is close to, but strictly less than, an exact power of 2.
 *
 * For the rationale, see listsort.txt.
 *
 * @param n the length of the array to be sorted
 * @return the length of the minimum run to be merged
 */
static gsize
compute_min_run (gsize n)
{
  gsize r = 0; // Becomes 1 if any 1 bits are shifted off

  while (n >= MIN_MERGE) {
    r |= (n & 1);
    n >>= 1;
  }
  return n + r;
}

void
bobgui_tim_sort_init (BobguiTimSort       *self,
                   gpointer          base,
                   gsize             size,
                   gsize             element_size,
                   GCompareDataFunc  compare_func,
                   gpointer          data)
{
  self->element_size = element_size;
  self->base = base;
  self->size = size;
  self->compare_func = compare_func;
  self->data = data;

  self->min_gallop = MIN_GALLOP;
  self->max_merge_size = G_MAXSIZE;
  self->min_run = compute_min_run (size);

  self->tmp = NULL;
  self->tmp_length = 0;
  self->pending_runs = 0;
}

void
bobgui_tim_sort_finish (BobguiTimSort *self)
{
  g_clear_pointer (&self->tmp, g_free);
}

void
bobgui_tim_sort (gpointer         base,
              gsize            size,
              gsize            element_size,
              GCompareDataFunc compare_func,
              gpointer         user_data)
{
  BobguiTimSort self;

  bobgui_tim_sort_init (&self, base, size, element_size, compare_func, user_data);

  while (bobgui_tim_sort_step (&self, NULL));

  bobgui_tim_sort_finish (&self);
}

static inline int
bobgui_tim_sort_compare (BobguiTimSort *self,
                      gpointer    a,
                      gpointer    b)
{
  return self->compare_func (a, b, self->data);
}


/**
 * Pushes the specified run onto the pending-run stack.
 *
 * @param runBase index of the first element in the run
 * @param runLen  the number of elements in the run
 */
static void
bobgui_tim_sort_push_run (BobguiTimSort *self,
                       void       *base,
                       gsize       len)
{
  g_assert (self->pending_runs < BOBGUI_TIM_SORT_MAX_PENDING);
  g_assert (len <= self->size);

  self->run[self->pending_runs].base = base;
  self->run[self->pending_runs].len = len;
  self->pending_runs++;

  /* Advance to find next run */
  self->base = ((char *) self->base) + len * self->element_size;
  self->size -= len;
}

/**
 * Ensures that the external array tmp has at least the specified
 * number of elements, increasing its size if necessary.  The size
 * increases exponentially to ensure amortized linear time complexity.
 *
 * @param min_capacity the minimum required capacity of the tmp array
 * @return tmp, whether or not it grew
 */
static gpointer
bobgui_tim_sort_ensure_capacity (BobguiTimSort *self,
                              gsize       min_capacity)
{
  if (self->tmp_length < min_capacity)
    {
      /* Compute smallest power of 2 > min_capacity */
      gsize new_size = min_capacity;
      new_size |= new_size >> 1;
      new_size |= new_size >> 2;
      new_size |= new_size >> 4;
      new_size |= new_size >> 8;
      new_size |= new_size >> 16;
#if GLIB_SIZEOF_SIZE_T > 4
      new_size |= new_size >> 32;
#endif

      new_size++;
      if (new_size == 0) /* (overflow) Not bloody likely! */
        new_size = min_capacity;

      g_free (self->tmp);
      self->tmp_length = new_size;
      self->tmp = g_malloc (self->tmp_length * self->element_size);
  }

  return self->tmp;
}

static void
bobgui_tim_sort_set_change (BobguiTimSortRun *out_change,
                         gpointer       base,
                         gsize          len)
{
  if (out_change)
    {
      out_change->base = base;
      out_change->len = len;
    }
}

/*<private>
 * bobgui_tim_sort_get_runs:
 * @self: a BobguiTimSort
 * @runs: (out) (caller-allocates): Place to store the 0-terminated list of
 *     runs
 *
 * Stores the already presorted list of runs - ranges of items that are
 * known to be sorted among themselves.
 *
 * This can be used with bobgui_tim_sort_set_runs() when resuming a sort later.
 **/
void
bobgui_tim_sort_get_runs (BobguiTimSort *self,
                       gsize       runs[BOBGUI_TIM_SORT_MAX_PENDING + 1])
{
  gsize i;

  g_return_if_fail (self);
  g_return_if_fail (runs);

  for (i = 0; i < self->pending_runs; i++)
    runs[i] = self->run[i].len;

  runs[self->pending_runs] = 0;
}

/*<private>
 * bobgui_tim_sort_set_runs:
 * @self: a freshly initialized BobguiTimSort
 * @runs: (array length=zero-terminated): a 0-terminated list of runs
 *
 * Sets the list of runs. A run is a range of items that are already
 * sorted correctly among themselves. Runs must appear at the beginning of
 * the array.
 *
 * Runs can only be set at the beginning of the sort operation.
 **/
void
bobgui_tim_sort_set_runs (BobguiTimSort *self,
                       gsize      *runs)
{
  gsize i;

  g_return_if_fail (self);
  g_return_if_fail (self->pending_runs == 0);

  for (i = 0; runs[i] != 0; i++)
    bobgui_tim_sort_push_run (self, self->base, runs[i]);
}

/*
 * bobgui_tim_sort_set_max_merge_size:
 * @self: a BobguiTimSort
 * @max_merge_size: Maximum size of a merge step, 0 for unlimited
 *
 * Sets the maximum size of a merge step. Every time
 * bobgui_tim_sort_step() is called and a merge operation has to be
 * done, the @max_merge_size will be used to limit the size of
 * the merge.
 *
 * The benefit is that merges happen faster, and if you're using
 * an incremental sorting algorithm in the main thread, this will
 * limit the runtime.
 *
 * The disadvantage is that setting up merges is expensive and that
 * various optimizations benefit from larger merges, so the total
 * runtime of the sorting will increase with the number of merges.
 *
 * A good estimate is to set a @max_merge_size to 1024 for around
 * 1ms runtimes, if your compare function is fast.
 *
 * By default, max_merge_size is set to unlimited.
 **/
void
bobgui_tim_sort_set_max_merge_size (BobguiTimSort *self,
                                 gsize       max_merge_size)
{
  g_return_if_fail (self != NULL);

  if (max_merge_size == 0)
    max_merge_size = G_MAXSIZE;
  self->max_merge_size = max_merge_size;
}

/**
 * bobgui_tim_sort_get_progress:
 * @self: a BobguiTimSort
 *
 * Does a progress estimate about sort progress, estimates relative
 * to the number of items to sort.
 *
 * Note that this is entirely a progress estimate and does not have
 * a relationship with items put in their correct place.  
 * It is also an estimate, so no guarantees are made about accuracy,
 * other than that it will only report 100% completion when it is
 * indeed done sorting.
 *
 * To get a percentage, you need to divide this number by the total
 * number of elements that are being sorted.
 *
 * Returns: Rough guess of sort progress
 **/
gsize
bobgui_tim_sort_get_progress (BobguiTimSort *self)
{
#define DEPTH 4
  gsize i;
  gsize last, progress;

  g_return_val_if_fail (self != NULL, 0);

  if (self->pending_runs == 0)
    return 0;

  last = self->run[0].len;
  progress = 0;

  for (i = 1; i < DEPTH + 1 && i < self->pending_runs; i++)
    {
      progress += (DEPTH + 1 - i) * MAX (last, self->run[i].len);
      last = MIN (last, self->run[i].len);
    }
  if (i < DEPTH + 1)
    progress += (DEPTH + 1 - i) * last;

  return progress / DEPTH;
#undef DEPTH
}

#if 1
#define WIDTH 4
#include "bobguitimsort-impl.c"

#define WIDTH 8
#include "bobguitimsort-impl.c"

#define WIDTH 16
#include "bobguitimsort-impl.c"
#endif

#define NAME default
#define WIDTH (self->element_size)
#include "bobguitimsort-impl.c"

/*
 * bobgui_tim_sort_step:
 * @self: a BobguiTimSort
 * @out_change: (optional): Return location for changed
 *     area. If a change did not cause any changes (for example,
 *     if an already sorted array gets sorted), out_change
 *     will be set to %NULL and 0.
 *
 * Performs another step in the sorting process. If a
 * step was performed, %TRUE is returned and @out_change is
 * set to the smallest area that contains all changes while
 * sorting.
 *
 * If the data is completely sorted, %FALSE will be
 * returned.
 *
 * Returns: %TRUE if an action was performed
 **/
gboolean
bobgui_tim_sort_step (BobguiTimSort    *self,
                   BobguiTimSortRun *out_change)
{
  gboolean result;

  g_assert (self);

  switch (self->element_size)
  {
    case 4:
      result = bobgui_tim_sort_step_4 (self, out_change);
      break;
    case 8:
      result = bobgui_tim_sort_step_8 (self, out_change);
      break;
    case 16:
      result = bobgui_tim_sort_step_16 (self, out_change);
      break;
    default:
      result = bobgui_tim_sort_step_default (self, out_change);
      break;
  }

  return result;
}

