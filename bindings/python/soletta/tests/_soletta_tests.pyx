import soletta.asyncio
from libc.stdint cimport uint32_t
from cython.operator cimport dereference as dref

cdef extern from "sol-mainloop.h":
	struct sol_timeout:
		pass

	struct sol_mainloop_implementation:
		pass

	int sol_init()

	sol_timeout* sol_timeout_add(uint32_t, bint (*)(void*), void*)
	bint sol_timeout_del(void*)

	const sol_mainloop_implementation *sol_mainloop_get_implementation()


tests = {}

"""
Tests should raise on a failure or otherwise return None  or post_check on success.
post_check - methods that run after the mainloop has completed its queued tasks.

"""

def add_test(func):
	tests[func] = False

cdef bint cb(void* data):
	print("timeout add callback called")
	success = <bint>data
	success = True
	print("callback returning...")
	return False

cdef bint cb2(void* data):
	count = <int>data
	count+=1

	return True

cdef bint timeout_success = False

def test_timeout():
	
	t = sol_timeout_add(100, cb, <void*>timeout_success)

	assert(t)

	def post():
		return timeout_success

	return post
	

def test_timeout_del():
	cdef sol_timeout* t

	callbackcounts = 0

	t = sol_timeout_add(100, cb2, <void*>callbackcounts)

	def test_timeout_del_post():
		return callbackcounts >= 1
	
	return test_timeout_del_post

def test_mainloop_impl():
	if not sol_mainloop_get_implementation():
		raise Exception("mainloop implementation None")

def run_tests():

	sol_init()

	add_test(test_mainloop_impl)
	add_test(test_timeout)

	post_test_checks = []

	for test in tests:
		try:
			post = test()
			if post:
				post_test_checks.append(post)
			print("{}: pass".format(test.__name__))
		except Exception, e:
			print("{}: FAIL".format(test.__name__))
			print(e)

	loop = soletta.asyncio.get_event_loop()
	loop.call_later(10, loop.stop)
	loop.run_forever()

	print("running post test checks...")
	for post in post_test_checks:
		try:
			assert(post())
			print("{}: pass".format(post.__name__))
		except:
			print("{}: FAIL ".format(post.__name__))


