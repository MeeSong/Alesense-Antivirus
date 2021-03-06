#include "ScanManager.h"
#include "ScanQueue.h"
#include "Traversal.h"
HANDLE ThreadScanTicket;
HANDLE ThreadPool_Scan[SCAN_POOL_MAX];
HANDLE ThreadPool_Scan_MainEvent;
HANDLE ThreadPool_Scan_ThreadEvent;
HANDLE ThreadPool_Scan_QueueEvent;
ENGINE_INFORMATION Engines[ENGINES_NUM] ;

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	

	
}

PVOID CreateFileIdentify(LPWSTR Path,PF_SCAN_CALLBACK ScanCallBack,DWORD32 CallBackPtr32,DWORD32 CallBackValue)
{
	if(IdentifyTaskCount == SCAN_POOL_MAX)
	{
		PSCAN_QUEUE_INFORMATION QInfo =  (PSCAN_QUEUE_INFORMATION)malloc(sizeof(SCAN_QUEUE_INFORMATION));
		QInfo->Next = IdentifyQueue;
		lstrcpy(QInfo->Info.Path,Path);
		QInfo->Info.ScanCallBack = ScanCallBack;
		QInfo->Info.CallBackPtr32 = CallBackPtr32;
		QInfo->Info.CallBackValue = CallBackValue;
		QInfo->Next = NULL;
		WaitForSingleObject(ThreadPool_Scan_QueueEvent,INFINITE);
		if(IdentifyQueue == NULL)
		{
			IdentifyQueueTop = QInfo;
			IdentifyQueue = QInfo;
		}
		else
		{
		IdentifyQueueTop->Next = QInfo;
		IdentifyQueueTop = QInfo;
		}
		SetEvent(ThreadPool_Scan_QueueEvent);
	}
	else
	{
	WaitForSingleObject(ThreadPool_Scan_MainEvent,INFINITE);
	IdentifyTaskCount ++;
	lstrcpy(IdentifyTemp.Path,Path);
	IdentifyTemp.ScanCallBack = ScanCallBack;
	IdentifyTemp.CallBackPtr32 = CallBackPtr32;
	IdentifyTemp.CallBackValue = CallBackValue;
	SetEvent(ThreadPool_Scan_ThreadEvent);
	}
	return 0;
}

void LoadDllForEngine(const LPWSTR DllPath,const char* FunctionName,PENGINE_INFORMATION EInfo)
{
	EInfo->hDll = LoadLibrary(DllPath);
	if (EInfo->hDll == NULL)
	{
		MessageBox(0, L"加载dll失败!", DllPath, MB_ICONERROR + MB_OK);
		return;
	}
	EInfo->FunctionCall = (SCANENGINE_NORMALCALL)GetProcAddress(EInfo->hDll,FunctionName);
	EInfo->FunctionCall2 = (SCANENGINE_NOINFORCALL)EInfo->FunctionCall;
	EInfo->Init = (SCANENGINE_INITCALL)GetProcAddress(EInfo->hDll,"InitlizeEngine");
	EInfo->Term = (SCANENGINE_INITCALL)GetProcAddress(EInfo->hDll,"TerminateEngine");
	if(EInfo->Init != NULL) EInfo->Init();
}
//=====================Export==========================//
void InitializeScanManager()
{
	ThreadPool_Scan_MainEvent = CreateEvent(NULL,FALSE,TRUE,NULL); //同步事件
	ThreadPool_Scan_ThreadEvent = CreateEvent(NULL,FALSE,FALSE,NULL); //同步事件
	ThreadPool_Scan_QueueEvent = CreateEvent(NULL,FALSE,TRUE,NULL); //同步事件
	IdentifyQueueTop = IdentifyQueue;
	for (int i = 0; i < SCAN_POOL_MAX; i++)
	{
		ThreadPool_Scan[i] = StartThread(ScanThreadPool,0);
	}
	for (int i = 0; i < ENGINES_NUM; i++)
	{
		switch (i)
		{
		case ScanEngine::JavHeur:
			LoadDllForEngine(L"JavHeur.dll","HeurScan",&Engines[i]); /////temp for debug
			break;
		case ScanEngine::FileVerify:
			LoadDllForEngine(L"AsVerify.dll","VerifyFileDigitalSignature",&Engines[i]);
			break;
		}
	}
	AsCreateNamedPipe(L"AsCoreScan",OnUIScanMessage);
}
void TerminateScanManager()
{
	PSCAN_QUEUE_INFORMATION Queue;
	for (int i = 0; i < ENGINES_NUM; i++)
	{
		if(Engines[i].Term != NULL) Engines[i].Term();
		FreeLibrary(Engines[i].hDll);
	}
	
	while(IdentifyQueue != NULL)
	{
		Queue = IdentifyQueue;
		IdentifyQueue = (PSCAN_QUEUE_INFORMATION)IdentifyQueue->Next;
		free(Queue);
	}
	CloseHandle(ThreadPool_Scan_MainEvent);
	CloseHandle(ThreadPool_Scan_ThreadEvent);
	CloseHandle(ThreadPool_Scan_QueueEvent);
}

