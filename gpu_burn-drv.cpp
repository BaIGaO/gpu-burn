/*
 * Original project released under the public domain by Ville Timonen in 2013
 *
 * All changes and improvements Copyright (c) 2013-2016 by Microway, Inc.
 *
 * This file is part of Microway gpu-burn
 *
 * Microway gpu-burn is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Microway gpu-burn is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gpu-burn.  If not, see <http://www.gnu.org/licenses/>
 */

#define SIZE 1024ul // Matrices are SIZE*SIZE..  1024^2 should be efficiently implemented in CUBLAS
#define USEMEM 0.9 // Try to allocate 90% of memory

#include <cstdio>
#include <string>
#include <map>
#include <vector>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <cuda.h>
#include "cublas_v2.h"

void checkError(int rCode, std::string desc = "") {
	static std::map<int, std::string> g_errorStrings;
	if (!g_errorStrings.size()) {
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_INVALID_VALUE, "CUDA_ERROR_INVALID_VALUE"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_OUT_OF_MEMORY, "CUDA_ERROR_OUT_OF_MEMORY"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_NOT_INITIALIZED, "CUDA_ERROR_NOT_INITIALIZED"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_DEINITIALIZED, "CUDA_ERROR_DEINITIALIZED"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_NO_DEVICE, "CUDA_ERROR_NO_DEVICE"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_INVALID_DEVICE, "CUDA_ERROR_INVALID_DEVICE"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_INVALID_IMAGE, "CUDA_ERROR_INVALID_IMAGE"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_INVALID_CONTEXT, "CUDA_ERROR_INVALID_CONTEXT"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_MAP_FAILED, "CUDA_ERROR_MAP_FAILED"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_UNMAP_FAILED, "CUDA_ERROR_UNMAP_FAILED"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_ARRAY_IS_MAPPED, "CUDA_ERROR_ARRAY_IS_MAPPED"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_ALREADY_MAPPED, "CUDA_ERROR_ALREADY_MAPPED"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_NO_BINARY_FOR_GPU, "CUDA_ERROR_NO_BINARY_FOR_GPU"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_ALREADY_ACQUIRED, "CUDA_ERROR_ALREADY_ACQUIRED"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_NOT_MAPPED, "CUDA_ERROR_NOT_MAPPED"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_NOT_MAPPED_AS_ARRAY, "CUDA_ERROR_NOT_MAPPED_AS_ARRAY"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_NOT_MAPPED_AS_POINTER, "CUDA_ERROR_NOT_MAPPED_AS_POINTER"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_UNSUPPORTED_LIMIT, "CUDA_ERROR_UNSUPPORTED_LIMIT"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_CONTEXT_ALREADY_IN_USE, "CUDA_ERROR_CONTEXT_ALREADY_IN_USE"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_INVALID_SOURCE, "CUDA_ERROR_INVALID_SOURCE"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_FILE_NOT_FOUND, "CUDA_ERROR_FILE_NOT_FOUND"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND, "CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_SHARED_OBJECT_INIT_FAILED, "CUDA_ERROR_SHARED_OBJECT_INIT_FAILED"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_OPERATING_SYSTEM, "CUDA_ERROR_OPERATING_SYSTEM"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_INVALID_HANDLE, "CUDA_ERROR_INVALID_HANDLE"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_NOT_FOUND, "CUDA_ERROR_NOT_FOUND"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_NOT_READY, "CUDA_ERROR_NOT_READY"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_LAUNCH_FAILED, "CUDA_ERROR_LAUNCH_FAILED"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_LAUNCH_OUT_OF_RESOURCES, "CUDA_ERROR_LAUNCH_OUT_OF_RESOURCES"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_LAUNCH_TIMEOUT, "CUDA_ERROR_LAUNCH_TIMEOUT"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_LAUNCH_INCOMPATIBLE_TEXTURING, "CUDA_ERROR_LAUNCH_INCOMPATIBLE_TEXTURING"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_PRIMARY_CONTEXT_ACTIVE, "CUDA_ERROR_PRIMARY_CONTEXT_ACTIVE"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_CONTEXT_IS_DESTROYED, "CUDA_ERROR_CONTEXT_IS_DESTROYED"));
		g_errorStrings.insert(std::pair<int, std::string>(CUDA_ERROR_UNKNOWN, "CUDA_ERROR_UNKNOWN"));
	}

	if (rCode != CUDA_SUCCESS)
		throw ((desc == "") ?
				std::string("Error: ") :
				(std::string("Error in \"") + desc + std::string("\": "))) +
			g_errorStrings[rCode];
}

void checkError(cublasStatus_t rCode, std::string desc = "") {
	static std::map<cublasStatus_t, std::string> g_errorStrings;
	if (!g_errorStrings.size()) {
		g_errorStrings.insert(std::pair<cublasStatus_t, std::string>(CUBLAS_STATUS_NOT_INITIALIZED, "CUBLAS_STATUS_NOT_INITIALIZED"));
		g_errorStrings.insert(std::pair<cublasStatus_t, std::string>(CUBLAS_STATUS_ALLOC_FAILED, "CUBLAS_STATUS_ALLOC_FAILED"));
		g_errorStrings.insert(std::pair<cublasStatus_t, std::string>(CUBLAS_STATUS_INVALID_VALUE, "CUBLAS_STATUS_INVALID_VALUE"));
		g_errorStrings.insert(std::pair<cublasStatus_t, std::string>(CUBLAS_STATUS_ARCH_MISMATCH, "CUBLAS_STATUS_ARCH_MISMATCH"));
		g_errorStrings.insert(std::pair<cublasStatus_t, std::string>(CUBLAS_STATUS_MAPPING_ERROR, "CUBLAS_STATUS_MAPPING_ERROR"));
		g_errorStrings.insert(std::pair<cublasStatus_t, std::string>(CUBLAS_STATUS_EXECUTION_FAILED, "CUBLAS_STATUS_EXECUTION_FAILED"));
		g_errorStrings.insert(std::pair<cublasStatus_t, std::string>(CUBLAS_STATUS_INTERNAL_ERROR, "CUBLAS_STATUS_INTERNAL_ERROR"));
	}

	if (rCode != CUBLAS_STATUS_SUCCESS)
		throw ((desc == "") ?
				std::string("Error: ") :
				(std::string("Error in \"") + desc + std::string("\": "))) +
			g_errorStrings[rCode];
}

template <class T> class GPU_Test {
	public:
	GPU_Test(int dev, bool doubles) : d_devNumber(dev), d_doubles(doubles) {
		checkError(cuDeviceGet(&d_dev, d_devNumber));
		checkError(cuCtxCreate(&d_ctx, 0, d_dev));

		bind();

		//checkError(cublasInit());
		checkError(cublasCreate(&d_cublas), "init");

		d_error = 0;
	}
	~GPU_Test() {
		bind();
		checkError(cuMemFree(d_Cdata), "Free A");
		checkError(cuMemFree(d_Adata), "Free B");
		checkError(cuMemFree(d_Bdata), "Free C");
		printf("Freed memory for dev %d\n", d_devNumber);

		cublasDestroy(d_cublas);
		printf("Uninitted cublas\n");
	}

	unsigned long long int getErrors() {
		unsigned long long int tempErrs = d_error;
		d_error = 0;
		return tempErrs;
	}

	size_t getIters() {
		return d_iters;
	}

	void bind() {
		checkError(cuCtxSetCurrent(d_ctx), "Bind CTX");
	}

	size_t totalMemory() {
		bind();
		size_t freeMem, totalMem;
		checkError(cuMemGetInfo(&freeMem, &totalMem));
		return totalMem;
	}

	size_t availMemory() {
		bind();
		size_t freeMem, totalMem;
		checkError(cuMemGetInfo(&freeMem, &totalMem));
		return freeMem;
	}

	void initBuffers(T *A, T *B) {
		bind();

		size_t useBytes = (size_t)((double)availMemory()*USEMEM);
		printf("Initialized device %d with %lu MB of memory (%lu MB available, using %lu MB of it), %s\n",
				d_devNumber, totalMemory()/1024ul/1024ul, availMemory()/1024ul/1024ul, useBytes/1024ul/1024ul,
				d_doubles ? "using DOUBLES" : "using FLOATS");
		size_t d_resultSize = sizeof(T)*SIZE*SIZE;
		d_iters = (useBytes - 2*d_resultSize)/d_resultSize; // We remove A and B sizes
		//printf("Results are %d bytes each, thus performing %d iterations\n", d_resultSize, d_iters);
		checkError(cuMemAlloc(&d_Cdata, d_iters*d_resultSize), "C alloc");
		checkError(cuMemAlloc(&d_Adata, d_resultSize), "A alloc");
		checkError(cuMemAlloc(&d_Bdata, d_resultSize), "B alloc");

		checkError(cuMemAlloc(&d_faultyElemData, sizeof(int)), "faulty data");

		// Populating matrices A and B
		checkError(cuMemcpyHtoD(d_Adata, A, d_resultSize), "A -> device");
		checkError(cuMemcpyHtoD(d_Bdata, B, d_resultSize), "A -> device");

		initCompareKernel();
	}

	void compute() {
		bind();
		static const float alpha = 1.0f;
		static const float beta = 0.0f;
		static const double alphaD = 1.0;
		static const double betaD = 0.0;

		for (size_t i = 0; i < d_iters; ++i) {
			if (d_doubles)
				checkError(cublasDgemm(d_cublas, CUBLAS_OP_N, CUBLAS_OP_N,
							SIZE, SIZE, SIZE, &alphaD,
							(const double*)d_Adata, SIZE,
							(const double*)d_Bdata, SIZE,
							&betaD,
							(double*)d_Cdata + i*SIZE*SIZE, SIZE), "DGEMM");
			else
				checkError(cublasSgemm(d_cublas, CUBLAS_OP_N, CUBLAS_OP_N,
							SIZE, SIZE, SIZE, &alpha,
							(const float*)d_Adata, SIZE,
							(const float*)d_Bdata, SIZE,
							&beta,
							(float*)d_Cdata + i*SIZE*SIZE, SIZE), "SGEMM");
		}
	}

	void initCompareKernel() {
		// The required file may be in the current directory or in /usr/libexec/
		if (access("gpu_burn.cuda_kernel", R_OK) != -1)
			checkError(cuModuleLoad(&d_module, "gpu_burn.cuda_kernel"), "load module");
		else if (access("/usr/libexec/gpu_burn.cuda_kernel", R_OK) != -1)
			checkError(cuModuleLoad(&d_module, "/usr/libexec/gpu_burn.cuda_kernel"), "load module");
		else
			fprintf(stderr, "\nUnable to find the CUDA kernels file: gpu_burn.cuda_kernel\n");

		checkError(cuModuleGetFunction(&d_function, d_module,
					d_doubles ? "compareD" : "compare"), "get func");

		checkError(cuFuncSetCacheConfig(d_function, CU_FUNC_CACHE_PREFER_L1), "L1 config");
		checkError(cuParamSetSize(d_function, __alignof(T*) + __alignof(int*) + __alignof(size_t)), "set param size");
		checkError(cuParamSetv(d_function, 0, &d_Cdata, sizeof(T*)), "set param");
		checkError(cuParamSetv(d_function, __alignof(T*), &d_faultyElemData, sizeof(T*)), "set param");
		checkError(cuParamSetv(d_function, __alignof(T*) + __alignof(int*), &d_iters, sizeof(size_t)), "set param");

		checkError(cuFuncSetBlockShape(d_function, g_blockSize, g_blockSize, 1), "set block size");
	}

	void compare() {
		int faultyElems;
		checkError(cuMemsetD32(d_faultyElemData, 0, 1), "memset");
		checkError(cuLaunchGrid(d_function, SIZE/g_blockSize, SIZE/g_blockSize), "Launch grid");
		checkError(cuMemcpyDtoH(&faultyElems, d_faultyElemData, sizeof(int)), "Read faultyelemdata");
		if (faultyElems) {
			d_error += (long long int)faultyElems;
			printf("WE FOUND %d FAULTY ELEMENTS from GPU %d\n", faultyElems, d_devNumber);
		}
	}

	private:
	bool d_doubles;
	int d_devNumber;
	size_t d_iters;
	size_t d_resultSize;

	long long int d_error;

	static const int g_blockSize = 16;

	CUdevice d_dev;
	CUcontext d_ctx;
	CUmodule d_module;
	CUfunction d_function;

	CUdeviceptr d_Cdata;
	CUdeviceptr d_Adata;
	CUdeviceptr d_Bdata;
	CUdeviceptr d_faultyElemData;

	cublasHandle_t d_cublas;
};

// Returns the number of devices
int initCuda() {
	checkError(cuInit(0));
	int deviceCount = 0;
	checkError(cuDeviceGetCount(&deviceCount));

	if (!deviceCount)
		throw std::string("No CUDA devices");

	#ifdef USEDEV
	if (USEDEV >= deviceCount)
		throw std::string("Not enough devices for USEDEV");
	#endif

	return deviceCount;
}

template<class T> void startBurn(int index, int writeFd, T *A, T *B, bool doubles) {
	GPU_Test<T> *our;
	try {
		our = new GPU_Test<T>(index, doubles);
		our->initBuffers(A, B);
	} catch (std::string e) {
		fprintf(stderr, "Couldn't init a GPU test: %s\n", e.c_str());
		exit(124);
	}

	// The actual work
	/*int iters = 0;
	unsigned long long int errors = 0;*/
	try {
		while (true) {
			our->compute();
			our->compare();
			/*errors += our->getErrors();
			iters++;*/
			int ops = our->getIters();
			write(writeFd, &ops, sizeof(int));
			ops = our->getErrors();
			write(writeFd, &ops, sizeof(int));
		}
	} catch (std::string e) {
		fprintf(stderr, "Failure during compute: %s\n", e.c_str());
		int ops = -1;
		// Signalling that we failed
		write(writeFd, &ops, sizeof(int));
		write(writeFd, &ops, sizeof(int));
		exit(111);
	}
}

int pollTemp(pid_t *p) {
	int tempPipe[2];
	pipe(tempPipe);

	pid_t myPid = fork();

	if (!myPid) {
		close(tempPipe[0]);
		dup2(tempPipe[1], STDOUT_FILENO); // Stdout
		//execlp("nvidia-smi", "nvidia-smi", "-l", "5", "-q", "-d", "TEMPERATURE", NULL);
		execlp("tegrastats","tegrastats", NULL);

		fprintf(stderr, "Could not invoke tegrastate, no temps available\n");

		exit(0);
	}

	*p = myPid;
	close(tempPipe[1]);

	return tempPipe[0];
}

void updateTemps(int handle, std::vector<float> *temps) {
	const int readSize = 10240;
	static int gpuIter = 0;
	char data[readSize+1];
	char buff[10]={0,0,0,0,0,0,0,0,0,0}; 
	char* finddata = NULL;

	int curPos = 0;
	do {
		read(handle, data+curPos, sizeof(char));
	} while (data[curPos++] != '\n');

	data[curPos-1] = 0;
	
	float tempValue;
	// FIXME: The syntax of this print might change in the future..
	//if (sscanf(data, "        GPU Current Temp            : %d C", &tempValue) == 1) {
	finddata = strstr(data,"GPU@");
	if (sscanf(finddata, "%*[^@]@%[^C]", buff) == 1) {
		//printf("read temp val %d\n", tempValue);
		tempValue =  strtof(buff,NULL);
		temps->at(gpuIter) = tempValue;
		gpuIter = (gpuIter+1)%(temps->size());
	} else if (!strcmp(data, "        Gpu                     : N/A"))
		gpuIter = (gpuIter+1)%(temps->size()); // We rotate the iterator for N/A values as well
}

void listenClients(std::vector<int> clientFd, std::vector<pid_t> clientPid, int runTime) {
	fd_set waitHandles;

	pid_t tempPid;
	int tempHandle = pollTemp(&tempPid);
	int maxHandle = tempHandle;

	FD_ZERO(&waitHandles);
	FD_SET(tempHandle, &waitHandles);

	for (size_t i = 0; i < clientFd.size(); ++i) {
		if (clientFd.at(i) > maxHandle)
			maxHandle = clientFd.at(i);
		FD_SET(clientFd.at(i), &waitHandles);
	}

	std::vector<float> clientTemp;
	std::vector<int> clientErrors;
	std::vector<int> clientCalcs;
	std::vector<bool> clientFaulty;

	for (size_t i = 0; i < clientFd.size(); ++i) {
		clientTemp.push_back(0);
		clientErrors.push_back(0);
		clientCalcs.push_back(0);
		clientFaulty.push_back(false);
	}

	time_t startTime = time(0);
	int changeCount;
	float nextReport = 10.0f;
	bool childReport = false;
	while ((changeCount = select(maxHandle+1, &waitHandles, NULL, NULL, NULL))) {
		//printf("got new data! %d\n", changeCount);
		// Going through all descriptors
		for (size_t i = 0; i < clientFd.size(); ++i)
			if (FD_ISSET(clientFd.at(i), &waitHandles)) {
				// First, reading processed
				int processed, errors;
				read(clientFd.at(i), &processed, sizeof(int));
				// Then errors
				read(clientFd.at(i), &errors, sizeof(int));

				clientErrors.at(i) += errors;
				if (processed == -1)
					clientCalcs.at(i) = -1;
				else
					clientCalcs.at(i) += processed;

				childReport = true;
			}

		if (FD_ISSET(tempHandle, &waitHandles))
			updateTemps(tempHandle, &clientTemp);

		// Resetting the listeners
		FD_ZERO(&waitHandles);
		FD_SET(tempHandle, &waitHandles);
		for (size_t i = 0; i < clientFd.size(); ++i)
			FD_SET(clientFd.at(i), &waitHandles);

		// Printing progress (if a child has initted already)
		if (childReport) {
			float elapsed = fminf((float)(time(0)-startTime)/(float)runTime*100.0f, 100.0f);
			printf("\r%.1f%%  ", elapsed);
			printf("proc: ");
			for (size_t i = 0; i < clientCalcs.size(); ++i) {
				if (clientCalcs.at(i) > 1000000 )
				  printf("%.2fM", (float)clientCalcs.at(i)/(float)1000000);
				else if (clientCalcs.at(i) > 1000 )
				  printf("%dK", clientCalcs.at(i)/1000);
				else
				  printf("%d", clientCalcs.at(i));
				if (i != clientCalcs.size() - 1)
					printf("/");
			}
			printf(" err: ");
			for (size_t i = 0; i < clientErrors.size(); ++i) {
				std::string note = "%d";
				if (clientCalcs.at(i) == -1)
					note += " (DIED!)";
				else if (clientErrors.at(i))
					note += " (WARNING!)";

				printf(note.c_str(), clientErrors.at(i));
				if (i != clientCalcs.size() - 1)
					printf("/");
			}
			printf(" tmp: ");
			for (size_t i = 0; i < clientTemp.size(); ++i) {
				printf(clientTemp.at(i) != 0 ? "%3.1fC" : "-- ", clientTemp.at(i));
				if (i != clientCalcs.size() - 1)
					printf("/");
			}

			fflush(stdout);

			if (nextReport < elapsed) {
				nextReport = elapsed + 10.0f;
				printf("\n\tSummary at:   ");
				fflush(stdout);
				system("date"); // Printing a date
				fflush(stdout);
				printf("\n");
				//printf("\t(checkpoint)\n");
				for (size_t i = 0; i < clientErrors.size(); ++i) {
					if (clientErrors.at(i))
						clientFaulty.at(i) = true;
					clientErrors.at(i) = 0;
				}
			}
		}

		// Checking whether all clients are dead
		bool oneAlive = false;
		for (size_t i = 0; i < clientCalcs.size(); ++i)
			if (clientCalcs.at(i) != -1)
				oneAlive = true;
		if (!oneAlive) {
			fprintf(stderr, "\n\nNo clients are alive!  Aborting\n");
			exit(123);
		}

		if (startTime + runTime < time(0))
			break;
	}

	printf("\nKilling processes.. ");
	fflush(stdout);
	for (size_t i = 0; i < clientPid.size(); ++i)
		kill(clientPid.at(i), 15);

	kill(tempPid, 15);
	close(tempHandle);

	while (wait(NULL) != -1);
	printf("done\n");

	printf("\nTested %d GPUs:\n", (int)clientPid.size());
	for (size_t i = 0; i < clientPid.size(); ++i)
		printf("\tGPU %d: %s\n", (int)i, clientFaulty.at(i) ? "FAULTY" : "OK");
}

template<class T> void launch(int runLength, bool useDoubles) {
	//system("nvidia-smi -L");

	// Initting A and B with random data
	T *A = (T*) malloc(sizeof(T)*SIZE*SIZE);
	T *B = (T*) malloc(sizeof(T)*SIZE*SIZE);
	srand(10);
	for (size_t i = 0; i < SIZE*SIZE; ++i) {
		A[i] = (T)((double)(rand()%1000000)/100000.0);
		B[i] = (T)((double)(rand()%1000000)/100000.0);
	}

	// Forking a process..  This one checks the number of devices to use,
	// returns the value, and continues to use the first one.
	int mainPipe[2];
	pipe(mainPipe);
	int readMain = mainPipe[0];
	std::vector<int> clientPipes;
	std::vector<pid_t> clientPids;
	clientPipes.push_back(readMain);

	pid_t myPid = fork();
	if (!myPid) {
		// Child
		close(mainPipe[0]);
		int writeFd = mainPipe[1];
		int devCount = initCuda();
		write(writeFd, &devCount, sizeof(int));

		startBurn<T>(0, writeFd, A, B, useDoubles);

		close(writeFd);
		return;
	} else {
		clientPids.push_back(myPid);

		close(mainPipe[1]);
		int devCount;
	    read(readMain, &devCount, sizeof(int));

		if (!devCount) {
			fprintf(stderr, "No CUDA devices\n");
		} else {

			for (int i = 1; i < devCount; ++i) {
				int slavePipe[2];
				pipe(slavePipe);
				clientPipes.push_back(slavePipe[0]);

				pid_t slavePid = fork();

				if (!slavePid) {
					// Child
					close(slavePipe[0]);
					initCuda();
					startBurn<T>(i, slavePipe[1], A, B, useDoubles);

					close(slavePipe[1]);
					return;
				} else {
					clientPids.push_back(slavePid);
					close(slavePipe[1]);
				}
			}

			listenClients(clientPipes, clientPids, runLength);
		}
	}

	for (size_t i = 0; i < clientPipes.size(); ++i)
		close(clientPipes.at(i));

	free(A);
	free(B);
}

int main(int argc, char **argv) {
	int runLength = 10;
	bool useDoubles = false;
	int thisParam = 0;
	if (argc >= 2 && std::string(argv[1]) == "-d") {
			useDoubles = true;
			thisParam++;
		}
	if (argc-thisParam < 2)
		printf("Run length not specified in the command line.  Burning for 10 secs\n");
	else
		runLength = atoi(argv[1+thisParam]);

	if (useDoubles)
		launch<double>(runLength, useDoubles);
	else
		launch<float>(runLength, useDoubles);

	return 0;
}
