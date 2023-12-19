#ifndef HEY_DSL
#define HEY_DSL

#define hey_dsl(CTX) \
	for ( \
		hey_exec_t* hey_dsl_ctx__ = (CTX); \
		hey_dsl_ctx__ != NULL; \
		hey_dsl_ctx__ = NULL \
	)

#define h_str(STRING) hey_push_str(hey_dsl_ctx__, (STRING), false)
#define h_str_special(STRING) hey_push_str(hey_dsl_ctx__, (STRING), true)
#define h_fmt(FMT, ...) hey_push_str_fmt(hey_dsl_ctx__, false, (FMT), __VA_ARGS__)
#define h_fmt_special(FMT, ...) hey_push_str_fmt(hey_dsl_ctx__, true, (FMT), __VA_ARGS__)
#define h_generate(...) hey_generate(hey_dsl_ctx__, (hey_generate_options_t){ __VA_ARGS__ })
#define h_lit(STRING) h_str(HEY_STR(STRING))
#define h_lit_special(STRING) h_str_special(HEY_STR(STRING))

#endif
