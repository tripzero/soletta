#ifndef helper_types_h_
#define helper_types_h_

typedef bool (*task_cb_t)(void* data);

struct TaskWrapper {
	task_cb_t cb;
	void* data;
	uint32_t timeout;
};



#endif