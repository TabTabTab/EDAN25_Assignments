#include <cstdio>
#include <cstring>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "timebase.h"

static void addToSum(unsigned long long increment);

class spinlock_t {
private:
	std::atomic<bool> flag = ATOMIC_VAR_INIT(false);
public:
	spinlock_t()
	{		
	}
	~spinlock_t()
	{	
		//flag.clear(std::memory_order_release);
	}

void lock() 
{
		bool f = false;
 
        // now make new_node the new head, but if the head
        // is no longer what's stored in new_node->next
        // (some other thread must have inserted a node just now)
        // then put that new head into new_node->next and try again
        while(!flag.compare_exchange_weak(
                                f, //detta vill vi att den var innan
                                true, //detta önskar vi sätta
                                std::memory_order_acq_rel,
                                std::memory_order_acquire)){
        	f = false;
        }

	//while(flag.test_and_set(std::memory_order_acquire));
}

void unlock() 
{
	/*
	bool t = true;
	while(!std::atomic_compare_exchange_weak_explicit(
                                &flag, // detta vil lvi sätta till true
                                &t, //detta vill vi att den var innan
                                false, //detta önskar vi sätta
                                std::memory_order_acquire,
                                std::memory_order_relaxed)){
        	t = true;
    	}*/
	flag.store(false, std::memory_order_release);
}
};

class worklist_t {
	int*			a;
	size_t			n;
	volatile size_t			total;	// sum a[0]..a[n-1]
	spinlock_t* spin;
		
public:
	worklist_t(size_t max)
	{
		n = max+1;
		total = 0;
		a = (int*) calloc(n, sizeof(a[0]));
		if (a == NULL) {
			fprintf(stderr, "no memory!\n");
			abort();
		}
		spin = new spinlock_t();
	}

	~worklist_t()
	{	
		free(a);
	}

	void reset()
	{
		total = 0;
		memset(a, 0, n*sizeof a[0]);
	}

	void put(int num)
	{
		spin->lock();
		a[num] += 1;
		total += 1;
		spin->unlock();
	}

	int get()
	{
		int				i;
		int				num;
		while(true){
			while(total <= 0);
			spin->lock();
			if(total>0)
				break;
			spin->unlock();
		}
		for (i = 1; i <= n; i += 1)
			if (a[i] > 0)
				break;

		if (i <= n) {
			a[i] -= 1;
			total -= 1;
		} else if (a[0] == 0) {
			fprintf(stderr, "corrupt data at line %d!\n", __LINE__);
			abort();
		} else 
			i = 0;

		spin->unlock();
		return i;
	}
};

static worklist_t*		worklist;
static std::atomic<unsigned long long>	sum;
static int			iterations;
static int			max;

static void produce()
{
	int		i;
	int		n;

	for (i = 0; i < iterations; i += 1)
		for (n = 1; n <= max; n += 1)
			worklist->put(n);

	worklist->put(0);
}

static unsigned long long factorial(unsigned long long n)
{
	return n <= 1 ? 1 : n * factorial(n - 1);
}

static void consume()
{
	int			n;
	unsigned long long	f;

	while ((n = worklist->get()) > 0) {
		f = factorial(n);
		addToSum(f);
	}
}

static void addToSum(unsigned long long increment)
{
	sum.fetch_add(increment, std::memory_order_relaxed);
}


static void work()
{
	sum = 0;
	worklist->reset();

	std::thread p(produce);
	std::thread a(consume);
	std::thread b(consume);
	std::thread c(consume);
	std::thread d(consume);

	p.join();
	a.join();
	b.join();
	c.join();
	d.join();
}

int main(void)
{
	double			begin;
	double			end;
	unsigned long long	correct;
	int			i;
	
	printf("mutex/condvar and mutex for sum\n");

	init_timebase();

	iterations	= 100000;
	max		= 12;
	correct		= 0;

	for (i = 1; i <= max; i += 1)
		correct += factorial(i);
	correct *= iterations;

	worklist = new worklist_t(max);

	for (i = 1; i <= 10; i += 1) {
		begin = timebase_sec();
		work();
		end = timebase_sec();

		if (sum != correct) {
			fprintf(stderr, "wrong output!\n");
			abort();
		}

		printf("T = %1.2lf s\n", end - begin);
	}

	delete worklist;

	return 0;
}
