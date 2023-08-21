#include <math.h>
#include <iostream>
#include <vector>
#include <bitset>
#include <string>
#include <random>
#include <time.h>
#include <stdlib.h>
#include "mpi.h"
#include <stdio.h>
#include <string>
using std::string;
using std::to_string;
using std::floor;
using std::vector;
using std::cout;
// use command $ mpicxx QuickSort.cpp -o QS -std=c++11 
// to compile program

vector<int> List = vector<int>(0);

// Function to aid with printing
string vectorToString(vector<int> list) {
	string s = "{";
	for (int i = 0; i < list.size(); i++) {
		s = s + to_string(list[i]);
		if (i < list.size() - 1) {
			s = s + ", ";
		}
	}
	return s + "}";
}

// Recursive and sequential quicksort


void quickSort(vector<int>& list, int min, int max)
{
	if (max > min) //Base case
	{
		int p = list[max];
		int i = (min - 1);
		for (int j = min; j <= max - 1; j++)
		{
			if (list[j] < p)
			{
				i++;

				int temp = list[i];
				list[i] = list[j];
				list[j] = temp;
			}
		}
		int temp = list[i + 1];
		list[i + 1] = list[max];
		list[max] = temp;
		int index = i + 1;
		quickSort(list, min, index - 1);
		quickSort(list, index + 1, max);
	}
}


int main(int argc, char* argv[])
{
	int id = 0;
	int numProcess = 0;
	int numElements = 0;
	//Change if you want to see lists
	bool show = true;
	double startTime = 0;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	MPI_Comm_size(MPI_COMM_WORLD, &numProcess);

	//Display created processes
	//cout << "Created process " << id << "\n";

	vector<int> unsortedList;
	//Determines the number of elements 
	//using the information given when running
	//and converting string to int
	if (argc == 3 || argc == 2) {
		numElements = atoi(argv[1]);
	}
	//So long as the requested number of elements and processes make sense
	if (numElements >= numProcess) {
		if (id == 0) {

			cout << "Generating unsorted list with " << numElements << " elements...\n";
			// Generates random list
			unsortedList = vector<int>(0);

			for (int i = 0; i < numElements; i++) {
				unsortedList.push_back((numElements * ((double)rand() / (double)RAND_MAX)));
			}
			//Optional displays set up using "show" boolean variable
			if (show) {
				cout << "List to sort: " << vectorToString(unsortedList) << "\n";
			}
		}

		// Starting time recorded
		if (id == 0) {
			startTime = MPI_Wtime();
		}
		//Determine if the num processes works with hypercube, and the dimension of the hypercube
		if (((int)log2(numProcess)) - log2(numProcess) == 0) {
			int dimensions = log2(numProcess);
			if (id == 0) {
				// Shares the split list accross other processes
				cout << "Distributing list to " << numProcess << " processes...\n";
				//Determine amount of elements per process
				int sizePartition = numElements / numProcess;
				for (int i = 0; i < numProcess; i++) {

					int max = sizePartition;
					if (i == numProcess - 1) {
						max = unsortedList.size() - (i * sizePartition);
					}

					vector<int> toSend = vector<int>(0);
					//Fills list that will be sent with equal parts from original list
					for (int j = 0; j < max; j++) {
						toSend.push_back(unsortedList[i * sizePartition + j]);
					}
					if (i == 0) {
						List = toSend;
					}
					else {
						int size = toSend.size();
						MPI_Send(&size, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
						MPI_Send(&toSend[0], size, MPI_INT, i, 1, MPI_COMM_WORLD);
					}
				}
			}
			else {
				// Recieves list from master process with id = 0
				MPI_Status status;
				int recvSize = 0;
				MPI_Recv(&recvSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
				List.resize(recvSize);
				MPI_Recv(&List[0], recvSize, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
			}
			//To see what process receives what list
			//cout << "Process " << id << " recieved: " << vectorToString(List) << "\n";
			if (id == 0) {
				cout << "Running sort on " << dimensions << " dimensional hypercube topology...\n";
			}

			// Hypercube quicksort algorythm as described in the labs
			for (int i = 0; i < dimensions; i++) {
				MPI_Status status;

				//Select the pick the pivot based on the element closest to randomly chosen element
				int p = List[floor(((List.size() - 1) * ((double)rand() / (double)RAND_MAX)))];

				//Create two new lists
				vector<int> List1 = vector<int>(0);
				vector<int> List2 = vector<int>(0);
				//Split original list by pivot into two new lists
				for (int x : List) {
					if (x <= p) {
						List1.push_back(x);
					}
					else {
						List2.push_back(x);
					}
				}

				vector<int> toSend;
				int neighbourID = 0;
				//using shifts to find neighbourID
				if (((id >> (i)) & 1) == 0) {
					neighbourID = abs((id + pow(2, i)));
					toSend = List2;
					List = List1;
				}

				else {
					neighbourID = abs((id - pow(2, i)));
					toSend = List1;
					List = List2;
				}
				vector<int> C;
				int sendSize = toSend.size();
				int recvSize = 0;

				// Receiving and sending done in different orders to avoid deadlock
				if (((id >> (i)) & 1) == 0) {
					MPI_Send(&sendSize, 1, MPI_INT, neighbourID, 2, MPI_COMM_WORLD);
					if (sendSize > 0) {
						MPI_Send(&toSend[0], sendSize, MPI_INT, neighbourID, 3, MPI_COMM_WORLD);
					}

					MPI_Recv(&recvSize, 1, MPI_INT, neighbourID, 2, MPI_COMM_WORLD, &status);
					C = vector<int>(recvSize);
					if (recvSize > 0) {
						MPI_Recv(&C[0], recvSize, MPI_INT, neighbourID, 3, MPI_COMM_WORLD, &status);
					}
				}
				else {
					MPI_Recv(&recvSize, 1, MPI_INT, neighbourID, 2, MPI_COMM_WORLD, &status);
					C = vector<int>(recvSize);
					if (recvSize > 0) {
						MPI_Recv(&C[0], recvSize, MPI_INT, neighbourID, 3, MPI_COMM_WORLD, &status);
					}

					MPI_Send(&sendSize, 1, MPI_INT, neighbourID, 2, MPI_COMM_WORLD);
					if (sendSize > 0) {
						MPI_Send(&toSend[0], sendSize, MPI_INT, neighbourID, 3, MPI_COMM_WORLD);
					}
				}
				//To show neighbouring processes
				//cout << "          Process " << id << " linked with " << neighbourID << " on iteration " << i << "\n";

				for (int x : C) {
					List.push_back(x);
				}
			}

			// Quicksort on sublist
			quickSort(List, 0, List.size() - 1);

			if (id == 0) {
				// Merges the lists from different nodes before being shown as a sorted list
				cout << "Gathering lists...\n";

				vector<int> finishedSort = List;

				for (int i = 1; i < numProcess; i++) {
					MPI_Status status;

					int size = 0;
					MPI_Recv(&size, 1, MPI_INT, i, 4, MPI_COMM_WORLD, &status);
					vector<int> subsorted = vector<int>(size);
					MPI_Recv(&subsorted[0], size, MPI_INT, i, 5, MPI_COMM_WORLD, &status);

					for (int x : subsorted) {
						finishedSort.push_back(x);
					}
				}
				//Final quicksort to ensure correct results
				//quickSort(finishedSort, 0, finishedSort.size() - 1);

				//Record time taken to sort
				double elapsedTime = MPI_Wtime() - startTime;

				if (show) {
					cout << "Sorted list: " << vectorToString(finishedSort) << "\n";
				}
				else {
					cout << "Finished sorting\n";
				}

				cout << "Time to sort: " << elapsedTime << " seconds\n\n";

			}
			else {
				int sendingSize = List.size();
				MPI_Send(&sendingSize, 1, MPI_INT, 0, 4, MPI_COMM_WORLD);
				MPI_Send(&List[0], sendingSize, MPI_INT, 0, 5, MPI_COMM_WORLD);
			}
		}
		else if (id == 0) {
			cout << "This number of processors does not fit a hypercube\n";
		}
	}
	else if (id == 0) {
		cout << "More processors than elements to sort \n";
	}

	MPI_Finalize();
	return 0;
}

