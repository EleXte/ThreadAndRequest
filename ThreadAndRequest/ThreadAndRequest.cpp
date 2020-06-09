#include "pch.h"
#include <stdio.h>	/* printf, NULL */
#include <iostream>
#include <ctime>
#include <queue> 
#include "windows.h"
#include <cstdlib>

using namespace std;

HANDLE* workerThreads;
CRITICAL_SECTION crit;

//Threads and programm working time
const int countGenerationThreads = 1;
const int countProcessThreads = 3;
const int timeWorking = 40; //In Seconds(!)

//Ranges for simulataion time operation. Global Just for play with them quick.
const int minP = 25;
const int maxP = 60;
const int minG = 20;
const int maxG = 50;
const double getProc = 0.70; //How many time GetRequest must generate requests


enum Stopper {
	JUST_DOIT,   // Just work... until next time
	TIME_2_STOP   //It's time to stop, realy
};

Stopper choiceStop() {
	//create rng and return stop or work signal type
	int rng = rand() % 50 + 1;
	return rng >= 40 ? TIME_2_STOP : JUST_DOIT;
}

class Request
{
private:
	static int NextID;
public:
	int id;

	Request() {
		id = Request::NextID++;
		std::cout << "\n [Req-GET-TRUE] [ID " << id << "] Getted from system.";
	};
};

int Request::NextID = 0;

class SharedObjects
{
private:
	
public:

	std::queue<Request*> queueRequest;
	std::queue<Request*> stoppedRequest;
	std::queue<Request*> completeRequest;

	int simTime; //Simulation time for generation in secconds
	int lostRequests; //Number of losted Requests in GetRequest

	SharedObjects() {
		/*empty*/
	};

	Request* takeRequest() {
		Request* val = NULL;
		///std::cout << "\nQuery size show 2 " << queueRequest.size();
		EnterCriticalSection(&crit);
		if (queueRequest.empty() == false)
		{
			val = queueRequest.front();
			queueRequest.pop();
		}
		LeaveCriticalSection(&crit);
		return val;
	}

	void addRequest(Request* &val) {
		EnterCriticalSection(&crit);
		queueRequest.push(val);
		LeaveCriticalSection(&crit);
	}

	void moveToStopped(Request* &val) {
		EnterCriticalSection(&crit);
		stoppedRequest.push(val);
		LeaveCriticalSection(&crit);
	}

	void moveToComplete(Request* &val) {
		EnterCriticalSection(&crit);
		completeRequest.push(val);
		LeaveCriticalSection(&crit);
	}

	void cleanAllQueues() {
		Request* val;
		while (queueRequest.empty() == false)
		{
			val = queueRequest.front();
			queueRequest.pop();
			delete val;
		}
		std::cout << "\n[END] Queue with Processing Requests cleaned.";
		while (stoppedRequest.empty() == false)
		{
			val = stoppedRequest.front();
			stoppedRequest.pop();
			delete val;
		}
		std::cout << "\n[END] Queue with Stopped Requests cleaned.";
		while (completeRequest.empty() == false)
		{
			val = completeRequest.front();
			completeRequest.pop();
			delete val;
		}
		std::cout << "\n[END] Queue with Completed Requests cleaned.";
	}
};

bool checkStop(Stopper stopSignal) {
	return stopSignal == TIME_2_STOP ? true : false;
}


Request* GetRequest(Stopper stopSignal) throw() {
	int wait = rand() % maxG + minG;
	//Operation time simulation
	Sleep(wait);

	//End of operation with signal check
	return checkStop(stopSignal) ? NULL : (new Request());
};


void ProcessRequest(Request* req, Stopper stopSignal) throw() {
	int wait = rand() % maxP + minP;
	Sleep(wait); //Simulate pre-process operations
	//Check for signal
	if (checkStop(stopSignal) == true) {
		std::cout << "\n[Req-PRC-STOP] [ID " << req->id << "] Is stopped in processing by signal.";
		return;
	}
	
	Sleep(wait); //Simulate post-process operations
	std::cout << "\n[Req-PRC-TRUE] [ID " << req->id << "] Process is fully complete.";
	/*
	TODO: Place for function to return result of process. Not for realisation now.
	*/
}

DWORD WINAPI simRequests(LPVOID lpParam) {
	/*
	This function work some times (given by main thread is second)
	*/
	SharedObjects* s = (SharedObjects*)lpParam;
	time_t now, end;
	time(&end);
	time(&now);
	end += s->simTime; //Calculate time stamp to end
	//Start getting request
	while (now < end) {
		Request* req = GetRequest(choiceStop());
		if (req != NULL) {
			s->addRequest(req);
		}
		else {
			//Calculate how many request not created
			s->lostRequests++;
			std::cout << "\n [Req-GET-LOST] [NUM " << s->lostRequests << "] Chancel getting Request.";
		}
		time(&now);
	}

	return 0;
}

DWORD WINAPI simProcess(LPVOID lpParam) {
	/*
	Works only while requests have in array.
	If request will be added after end process his be ignored.
	I do that only for chance simulate situation when request stoped analize for unknow reason.
	*/
	Sleep(300); //Starting sleep to simulate acess for needed resource
	SharedObjects* s = (SharedObjects*)lpParam;
	Stopper check;
	while (s->queueRequest.empty() == false) {
		check = choiceStop();
		Request* req = s->takeRequest();
		if (check == false) {
			s->moveToComplete(req);
		} else {
			s->moveToStopped(req);
		}
		ProcessRequest(req, check);
	}

	return 0;
}

int main()
{
    std::cout << "[Start] Initial base parametrs.\n"; 
	std::srand(unsigned(std::time(0)));
	int countTotalThreads = countGenerationThreads + countProcessThreads;
	HANDLE hProducerThread;
	HANDLE hConsumerThread;
	SharedObjects s;

	s.simTime = timeWorking * getProc;
	std::cout << "\n[Start] Time of Generation request simulation: " << s.simTime << " seconds.";

	InitializeCriticalSection(&crit);
	workerThreads = new HANDLE[countTotalThreads];

	std::cout << "\n[Start] Initial Get Threads.\n";
	for (int i = 0; i < countGenerationThreads; i++) {
		hProducerThread = CreateThread(NULL, 0, simRequests, &s, 0, NULL);
		if (hProducerThread == NULL) {
			std::cout << "\n[ERROR] hProducerThread create failed: " << GetLastError();
			return 1;
		}
		workerThreads[i] = hProducerThread;
	}

	std::cout << "[Start] Initial Processing Threads.\n";
	for (int i = countGenerationThreads; i < countTotalThreads; i++) {
		hConsumerThread = CreateThread(NULL, 0, simProcess, &s, 0, NULL);
		if (hConsumerThread == NULL) {
			std::cout << "\n[ERROR] hConsumerThread create failed: " << GetLastError();
			return 1;
		}
		workerThreads[i] = hConsumerThread;
	}

	Sleep(timeWorking * 1000);

	std::cout << "\n\n[End] Close all threads.\n";
	for (int i = 0; i < countTotalThreads; i++) {
		CloseHandle(workerThreads[i]);
	}
	delete[] workerThreads;


	std::cout << "\n[END] Total Requests losted while getting: " << s.lostRequests;
	std::cout << "\n[END] Total Requests Left in main queue: " << s.queueRequest.size();
	std::cout << "\n[END] Total Stopped Requests: " << s.stoppedRequest.size();
	std::cout << "\n[END] Total Complete Requests: " << s.completeRequest.size();
	s.cleanAllQueues();

	std::cout << "\n[DONE] All done. Press any key to close programm.";
	getc(stdin);

}
