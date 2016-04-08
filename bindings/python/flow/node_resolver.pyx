
cdef extern from "sol-flow-resolver.h":
	struct sol_flow_resolver:
		const char* name
		void* data
		int resolve(void*, const char*, struct sol_flow_node_type const **, struct sol_flow_node_named_options* named_opts)

	const struct sol_flow_resolver *sol_flow_get_default_resolver()
	const struct sol_flow_resolver *sol_flow_get_builtins_resolver()
	int sol_flow_resolve(const struct sol_flow_resolver *, const char *, const struct sol_flow_node_type **, struct sol_flow_node_named_options *)