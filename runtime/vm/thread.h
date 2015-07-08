// Copyright (c) 2015, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#ifndef VM_THREAD_H_
#define VM_THREAD_H_

#include "vm/base_isolate.h"
#include "vm/globals.h"
#include "vm/os_thread.h"
#include "vm/store_buffer.h"

namespace dart {

class CHA;
class Isolate;
class RawBool;
class RawObject;
class Object;


// List of VM-global objects/addresses cached in each Thread object.
#define CACHED_VM_OBJECTS_LIST(V)                                              \
  V(RawObject*, object_null_, Object::null(), NULL)                            \
  V(RawBool*, bool_true_, Object::bool_true().raw(), NULL)                     \
  V(RawBool*, bool_false_, Object::bool_false().raw(), NULL)                   \

#define CACHED_ADDRESSES_LIST(V)                                               \
  V(uword, update_store_buffer_entry_point_,                                   \
    StubCode::UpdateStoreBufferEntryPoint(), 0)

#define CACHED_CONSTANTS_LIST(V)                                               \
  CACHED_VM_OBJECTS_LIST(V)                                                    \
  CACHED_ADDRESSES_LIST(V)


// A VM thread; may be executing Dart code or performing helper tasks like
// garbage collection or compilation. The Thread structure associated with
// a thread is allocated by EnsureInit before entering an isolate, and destroyed
// automatically when the underlying OS thread exits. NOTE: On Windows, CleanUp
// must currently be called manually (issue 23474).
class Thread {
 public:
  // The currently executing thread, or NULL if not yet initialized.
  static Thread* Current() {
    return reinterpret_cast<Thread*>(OSThread::GetThreadLocal(thread_key_));
  }

  // Initializes the current thread as a VM thread, if not already done.
  static void EnsureInit();

  // Makes the current thread enter 'isolate'.
  static void EnterIsolate(Isolate* isolate);
  // Makes the current thread exit its isolate.
  static void ExitIsolate();

  // A VM thread other than the main mutator thread can enter an isolate as a
  // "helper" to gain limited concurrent access to the isolate. One example is
  // SweeperTask (which uses the class table, which is copy-on-write).
  // TODO(koda): Properly synchronize heap access to expand allowed operations.
  static void EnterIsolateAsHelper(Isolate* isolate);
  static void ExitIsolateAsHelper();

  // Called when the current thread transitions from mutator to collector.
  // Empties the store buffer block into the isolate.
  // TODO(koda): Always run GC in separate thread.
  static void PrepareForGC();

#if defined(TARGET_OS_WINDOWS)
  // Clears the state of the current thread and frees the allocation.
  static void CleanUp();
#endif

  // Called at VM startup.
  static void InitOnceBeforeIsolate();
  static void InitOnceAfterObjectAndStubCode();

  // The topmost zone used for allocation in this thread.
  Zone* zone() {
    return reinterpret_cast<BaseIsolate*>(isolate())->current_zone();
  }

  // The isolate that this thread is operating on, or NULL if none.
  Isolate* isolate() const { return isolate_; }
  static intptr_t isolate_offset() {
    return OFFSET_OF(Thread, isolate_);
  }

  // The (topmost) CHA for the compilation in the isolate of this thread.
  // TODO(23153): Move this out of Isolate/Thread.
  CHA* cha() const;
  void set_cha(CHA* value);

  void StoreBufferAddObject(RawObject* obj);
  void StoreBufferAddObjectGC(RawObject* obj);
#if defined(TESTING)
  bool StoreBufferContains(RawObject* obj) const {
    return store_buffer_block_->Contains(obj);
  }
#endif
  void StoreBufferBlockProcess(bool check_threshold);
  static intptr_t store_buffer_block_offset() {
    return OFFSET_OF(Thread, store_buffer_block_);
  }

#define DEFINE_OFFSET_METHOD(type_name, member_name, expr, default_init_value) \
  static intptr_t member_name##offset() {                                      \
    return OFFSET_OF(Thread, member_name);                                     \
  }
CACHED_CONSTANTS_LIST(DEFINE_OFFSET_METHOD)
#undef DEFINE_OFFSET_METHOD

  static bool CanLoadFromThread(const Object& object);
  static intptr_t OffsetFromThread(const Object& object);

 private:
  static ThreadLocalKey thread_key_;

  Isolate* isolate_;
  StoreBufferBlock* store_buffer_block_;
#define DECLARE_MEMBERS(type_name, member_name, expr, default_init_value)      \
  type_name member_name;
CACHED_CONSTANTS_LIST(DECLARE_MEMBERS)
#undef DECLARE_MEMBERS

  explicit Thread(bool init_vm_constants = true);

  void InitVMConstants();

  static void SetCurrent(Thread* current);

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

}  // namespace dart

#endif  // VM_THREAD_H_
