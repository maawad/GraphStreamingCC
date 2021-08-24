#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include "../include/work_queue.h"

static bool shutdown = false;
static std::atomic<uint64_t> upd_processed;

// queries the buffer tree and verifies that the data
// Should be run in a seperate thread
void querier(WorkQueue *wq, int nodes) {
	data_ret_t data;
	while(true) {
		bool valid = wq->get_data(data);
		if(!valid && shutdown)
			return;
	}
}

void run_test(const int nodes, const unsigned long updates) {
	shutdown = false;
	
	unsigned long node_size = 24*pow((log2(nodes)), 3);
	node_size /= sizeof(node_id_t);
       	WorkQueue *wq = new WorkQueue(node_size, nodes, 80);

	// create queriers
	std::thread query_threads[40];
	for (int t = 0; t < 40; t++) {
		query_threads[t] = std::thread(querier, wq, nodes);
	}

	auto start = std::chrono::steady_clock::now();
	for (uint64_t i = 0; i < updates; i++) {
		if(i % 1000000000 == 0)
			printf("processed so far: %lu\n", i);
		
		update_t upd;
		upd.first = i % nodes;
		upd.second = (nodes - 1) - (i % nodes);
		wq->insert(upd);
		std::swap(upd.first, upd.second);
		wq->insert(upd);
	}
	wq->force_flush();
	shutdown = true;
	wq->set_non_block(true);

	std::chrono::duration<double> delta = std::chrono::steady_clock::now() - start;
	printf("Insertions took %f seconds: average rate = %f\n", delta.count(), updates/delta.count());

	for (int i = 0; i < 40; i++) {
		query_threads[i].join();
	}
	delete wq;
}

TEST(Throughput_Test, kron15) {
	run_test(8192, 17542263);
}

TEST(Throughput_Test, kron17) {
	run_test(131072, 4474931789);
}

TEST(Throughput_Test, kron18) {
	run_test(262144, 17891985703);
}
