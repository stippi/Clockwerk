// AllocationChecker.h

#ifndef ALLOCATION_CHECKER_H
#define ALLOCATION_CHECKER_H

#include <new>

#include <image.h>

#include <Locker.h>
#include <OS.h>

#include "SymbolTable.h"

class ElfSymbolPatchGroup;
struct stack_frame;

// AllocationChecker
class AllocationChecker {
public:

	static	AllocationChecker*	CreateDefault(bool wallChecking = false);
	static	void				DeleteDefault();
	static	AllocationChecker*	GetDefault();

			void*				New(size_t size, bool array, bool cpp,
									bool clear = false);
			void				Delete(void *block, bool array, bool cpp);
			void*				Realloc(void *block, size_t size);
			void				ImplicitAllocation(void *address, size_t size,
												   bool array, bool cpp);

			bool				TrackAllocation(void* address,
												bool track = true);

			SymbolTable*		GetSymbolTable()	{ return &fSymbols; }

			uint64				SequenceIndex() const { return fSequenceIndex; }

			void				Dump(bool detailed = false,
									uint64 afterSequenceIndex = 0LL,
									bigtime_t after = 0LL);

			void				CheckWalls();

private:
	class Info;
	class GetInfoKey;
	class InfoNodeAllocator;
	class InfoGetValue;
	class InfoTree;

private:
								AllocationChecker(bool wallChecking);
								~AllocationChecker();

			void				_Allocated(void *address, size_t size,
									bool array, bool cpp);
			void				_Deleted(void *block, bool array, bool cpp);

			Info*				_AllocateInfo();
			void				_FreeInfo(Info* info);
			Info*				_FindInfo(void* block);

			void				_UpdateCurrentStackFrame();

			void				_Debugger(const char* message,
									Info* info = NULL);

private:
	static	void*				_CallocHook(size_t nmemb, size_t size);
	static	void				_FreeHook(void* ptr);
	static	void*				_MallocHook(size_t size);
	static	void*				_ReallocHook(void* ptr, size_t size);

	static	void*				_NewHook(size_t size);
	static	void*				_NewNothrowHook(size_t size,
									const std::nothrow_t&);
	static	void*				_NewVecHook(size_t size);
	static	void*				_NewVecNothrowHook(size_t size,
									const std::nothrow_t&);
	static	void 				_DeleteHook(void *ptr);
	static	void 				_DeleteNothrowHook(void *ptr,
									const std::nothrow_t&);
	static	void				_DeleteVecHook(void *ptr);
	static	void 				_DeleteVecNothrowHook(void *ptr,
									const std::nothrow_t&);

	static	char*				_StrdupHook(const char* str);
	static	image_id			_LoadAddOnHook(const char* path);
	static	status_t			_UnloadAddOnHook(image_id image);

			void				_InstallMallocHooks();
			void				_RestoreMallocHooks();

			image_id			_LoadAddOn(const char* path);
			status_t			_UnloadAddOn(image_id image);

	static int					_CompareInfoBySequenceIndex(const void* a,
									const void* b);

private:
	static	AllocationChecker*	fDefaultChecker;

			BLocker				fLock;
			SymbolTable			fSymbols;
			image_id			fAppImage;
			area_id				fAppTextArea;
			ElfSymbolPatchGroup* fPatchGroup;
			stack_frame*		fCurrentStackFrame;
			uint64				fSequenceIndex;
			area_id				fArea;
			InfoTree*			fUsedInfos;
			Info*				fInfos;
			int32				fInfoCount;
			int32				fNextUnusedIndex;
			Info*				fNextFreeInfo;
			bool				fDontLog;
			bool				fWallChecking;

			void*				(*fOldCallocHook)(size_t, size_t);
			void				(*fOldFreeHook)(void*);
			void*				(*fOldMallocHook)(size_t);
			void*				(*fOldReallocHook)(void*, size_t);

			void*				(*fOldNewHook)(size_t);
			void*				(*fOldNewNothrowHook)(size_t,
									const std::nothrow_t&);
			void*				(*fOldNewVecHook)(size_t size);
			void*				(*fOldNewVecNothrowHook)(size_t,
									const std::nothrow_t&);
			void 				(*fOldDeleteHook)(void*);
			void 				(*fOldDeleteNothrowHook)(void*,
									const std::nothrow_t&);
			void				(*fOldDeleteVecHook)(void*);
			void 				(*fOldDeleteVecNothrowHook)(void*,
									const std::nothrow_t&);

			char*				(*fOldStrdupHook)(const char*);
			image_id			(*fOldLoadAddOnHook)(const char*);
			status_t			(*fOldUnloadAddOnHook)(image_id);
};

#endif	// ALLOCATION_CHECKER_H
