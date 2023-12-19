#ifndef HEY_DSL
#define HEY_DSL

#define hey_dsl(CTX) \
	for ( \
		struct { \
			hey_exec_t* ctx; \
			hey_index_t last_index; \
		} hey_dsl_scope__ = { \
			.ctx = (CTX), \
		}; \
		hey_dsl_scope__.ctx != NULL; \
		hey_dsl_scope__.ctx = NULL \
	)

#define h_ctx() hey_dsl_scope__.ctx
#define h_scope() hey_dsl_scope__
#define h_str(STRING) hey_push_str(h_ctx(), (STRING), false)
#define h_str_special(STRING) hey_push_str(h_ctx(), (STRING), true)
#define h_fmt(FMT, ...) hey_push_str_fmt(h_ctx(), false, (FMT), __VA_ARGS__)
#define h_fmt_special(FMT, ...) hey_push_str_fmt(h_ctx(), true, (FMT), __VA_ARGS__)
#define h_generate(...) hey_generate(h_ctx(), (hey_generate_options_t){ __VA_ARGS__ })
#define h_lit(STRING) h_str(HEY_STR(STRING))
#define h_lit_special(STRING) h_str_special(HEY_STR(STRING))

#endif
