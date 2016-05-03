import asyncio
import cython
from libc.stdint cimport uint32_t, uint16_t, uint64_t
from cpython.mem cimport PyMem_Malloc, PyMem_Realloc, PyMem_Free

cdef extern from "sol-mainloop.h":
	struct sol_mainloop_source_type:
		pass

	struct sol_mainloop_implementation:
		void run()
		void init()
		void quit()
		void * idle_add(bint (*)(void*), void*)
		bint idle_del(void*)
		void* timeout_add(uint32_t timeout_ms, bint (*cb)(void *), const void *)
		bint timeout_del(void*)

		void * fd_add(int fd, uint32_t flags, bint (*cb)(void *data, int fd, uint32_t active_flags), const void *data)
		bint fd_del(void *handle)
		bint fd_set_flags(void *handle, uint32_t flags)
		uint32_t fd_get_flags(const void *handle)

		void *source_add(const sol_mainloop_source_type *type, const void *data)
		void source_del(void *handle)
		void *source_get_data(const void *handle)

		void *child_watch_add(uint64_t pid, void (*cb)(void *data, uint64_t pid, int status), const void *data)
		bint child_watch_del(void *handle)

		void shutdown()
		uint16_t api_version
		pass
	
	int sol_init()
	void sol_shutdown()

	cdef uint16_t SOL_MAINLOOP_IMPLEMENTATION_API_VERSION "SOL_MAINLOOP_IMPLEMENTATION_API_VERSION" ()

	bint sol_mainloop_set_implementation(const sol_mainloop_implementation *)

	cdef void emit_ifdef "#if defined(SOL_NO_API_VERSION) //" ()
	cdef void emit_endif "#endif //" ()

class MainloopAsyncio:

	def __init__(self, loop = asyncio.get_event_loop()):
		self.loop = loop

	def __del__(self):
		sol_shutdown()

_loop = MainloopAsyncio()

def get_event_loop():
	return _loop.loop

cdef class TaskWrapper:

	cdef bint (*) (void * data)* cb
	cdef void* data
	cdef uint32_t timeout

@asyncio.coroutine
def task(TaskWrapper* self):
	while self.cb(self.data):
		yield from asyncio.sleep(self.timeout / 1000)

cdef void * new_task(bint (*cb) (void * data), void* data, uint32_t timeout = 100):
	cdef TaskWrapper *t = <TaskWrapper*>PyMem_Malloc(sizeof(TaskWrapper))
	t.cb = cb
	t.data = data
	t.timeout = timeout
	
	_loop.loop.create_task(task(t))

	return t


cdef void wrap_sol_init():
	print("soletta.asyncio is initialized!!!")
	pass

cdef void wrap_sol_shutdown():
	pass

cdef void wrap_sol_quit():
	_loop.loop.stop()

cdef void wrap_sol_run():
	_loop.loop.run_forever()

cdef void *wrap_sol_timeout_add(uint32_t timeout_ms, bint (*cb)(void *), const void *data):
	
	cdef void *wrapper = new_task(cb, data, timeout_ms)

	return wrapper

cdef bint wrap_sol_timeout_del(void* handle):
	(<object?>handle).cancel()
	return True

cdef void *wrap_idle_add(bint (*cb)(void*), const void* data):
	
	cdef void *wrapper = new_task(cb, data)

	return wrapper

cdef bint wrap_idle_del(void* handle):
	(<object?>handle).cancel()
	return True

cdef void quit():
	_loop.stop()

cdef void * wrap_source_add(const sol_mainloop_source_type * type, const void * data):
	pass

cdef void wrap_source_del(void * src):
	pass

cdef void * wrap_source_get_data(const void* arg):
	return NULL

cdef void * wrap_fd_add(int fd, uint32_t flags, bint (*cb)(void *data, int fd, uint32_t active_flags), const void *data):
	return NULL

cdef bint wrap_fd_del(void *handle):
	return False

cdef bint wrap_fd_set_flags(void *handle, uint32_t flags):
	return False

cdef uint32_t wrap_fd_get_flags(const void *handle):
	return 0

cdef void * wrap_child_watch_add(uint64_t pid, void (*cb)(void *data, uint64_t pid, int status), const void *data):
	return NULL

cdef bint wrap_child_watch_del(void *handle):
	return False	

cdef sol_mainloop_implementation _py_asyncio_impl

"""
TODO: we want to compile in the api_version...

emit_ifdef()
_py_asyncio_impl.api_version = SOL_MAINLOOP_IMPLEMENTATION_API_VERSION()
emit_endif()
"""

_py_asyncio_impl.api_version = 1
_py_asyncio_impl.run = wrap_sol_run
_py_asyncio_impl.idle_add = wrap_idle_add
_py_asyncio_impl.idle_del = wrap_idle_del
_py_asyncio_impl.init = wrap_sol_init
_py_asyncio_impl.quit = wrap_sol_quit
_py_asyncio_impl.shutdown = wrap_sol_shutdown
_py_asyncio_impl.timeout_add = wrap_sol_timeout_add
_py_asyncio_impl.timeout_del = wrap_sol_timeout_del

_py_asyncio_impl.source_add = wrap_source_add
_py_asyncio_impl.source_del = wrap_source_del
_py_asyncio_impl.source_get_data = wrap_source_get_data

_py_asyncio_impl.fd_add = wrap_fd_add
_py_asyncio_impl.fd_del = wrap_fd_del
_py_asyncio_impl.fd_get_flags = wrap_fd_get_flags
_py_asyncio_impl.fd_set_flags = wrap_fd_set_flags

_py_asyncio_impl.child_watch_add = wrap_child_watch_add
_py_asyncio_impl.child_watch_del = wrap_child_watch_del

sol_mainloop_set_implementation(&_py_asyncio_impl) # to be called on module load