#include <atomic>
#include <mpi.h>
#include <exception>
#include <stdio.h>
class TTS {
	std::atomic<bool>state{ false };
public:
	void lock() {
		while (true)
		{
			while (state.load()) {

			}
			if (!state.exchange(true))
			{
				return;
			}
		}
	}
	void unlock() {
		state.store(false);
	}
};
int getAndIncrement(int temp) {
	TTS* lock = new TTS();
	lock->lock();
	//printf("enter locked");
	try
	{

		temp = temp + 1;


	}
	catch (const std::exception&)
	{
		lock->unlock();
	}
	lock->unlock();
	return temp;
}
int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);

	// Check that only 2 MPI processes are spawn
	int comm_size;
	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

	// Get my rank
	int my_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	// Create the window
	const int ARRAY_SIZE = 8;
	int window_buffer[ARRAY_SIZE];
	MPI_Win window;
	if (my_rank == 0)
	{
		window_buffer[1] = 0;
		//window_buffer[2] = getAndIncrement();
		//window_buffer[3] = getAndIncrement();
		//window_buffer[4] = getAndIncrement();
		//window_buffer[5] = getAndIncrement();
		//window_buffer[6] = getAndIncrement();
		//window_buffer[7] = getAndIncrement();
		//window_buffer[1] = 40;

	}
	MPI_Win_create(window_buffer, ARRAY_SIZE * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &window);
	MPI_Win_fence(0, window);
	int remote_value;
	if (my_rank == 0)
	{
		MPI_Win_lock(MPI_LOCK_EXCLUSIVE, my_rank, 0, window);

		// Fetch the second integer in MPI process 1 window
		MPI_Get(&remote_value, 1, MPI_INT, 0, 1, 1, MPI_INT, window);

		// Push my value into the first integer in MPI process 1 window
	//	printf("remote value is %d \n", remote_value);
		int my_value = getAndIncrement(remote_value);
		MPI_Put(&my_value, 1, MPI_INT, 1, 0, 1, MPI_INT, window);
		MPI_Win_unlock(my_rank, window);
	}
	MPI_Win_fence(0, window);
	for (int i = 0; i < comm_size; i++)
	{
		MPI_Barrier(MPI_COMM_WORLD);
		if (i == my_rank && my_rank != 0) {
			MPI_Win_lock(MPI_LOCK_EXCLUSIVE, my_rank, 0, window);
			// Fetch the second integer in MPI process 1 window
			MPI_Get(&remote_value, 1, MPI_INT, my_rank, 0, 1, MPI_INT, window);
			// Push my value into the first integer in MPI process 1 window
			//printf("remote value is %d \n", remote_value);
			if (my_rank + 1 < comm_size) {
				int my_value = getAndIncrement(remote_value);
				MPI_Put(&my_value, 1, MPI_INT, my_rank + 1, 0, 1, MPI_INT, window);
			}
			MPI_Win_unlock(my_rank, window);
		}
		MPI_Win_fence(0, window);
	}


	if (my_rank == 0)
	{
		printf("[MPI process %d] Value fetched from MPI process %d window_buffer[0]: %d.\n", my_rank, my_rank, remote_value);
	}
	else if (my_rank < comm_size)
	{
		printf("[MPI process %d] Value put in my window_buffer[0] by MPI process %d: %d.\n", my_rank, my_rank - 1, window_buffer[0]);
		printf("[MPI process %d] Value fetched from MPI process %d window_buffer[0]: %d.\n", my_rank, my_rank, remote_value);
	}
	// Destroy the window
	MPI_Win_free(&window);

	MPI_Finalize();

	return EXIT_SUCCESS;
}