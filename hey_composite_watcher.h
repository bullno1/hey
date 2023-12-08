#ifndef HEY_COMPOSITE_WATCHER_H
#define HEY_COMPOSITE_WATCHER_H

#include "hey.h"

HEY_API hey_watcher_t
hey_make_composite_watcher(const hey_watcher_t watchers[]);

#endif

#ifdef HEY_IMPLEMENTATION

HEY_PRIVATE void
hey_composite_watcher(const hey_event_t* event, hey_exec_t* ctx, void* userdata) {
	const hey_watcher_t* watchers = userdata;
	for (hey_index_t i = 0; watchers[i].fn != NULL; ++i) {
		watchers[i].fn(event, ctx, watchers[i].userdata);
	}
}

hey_watcher_t
hey_make_composite_watcher(const hey_watcher_t watchers[]) {
	return (hey_watcher_t){
		.fn = hey_composite_watcher,
		.userdata = (void*)watchers,
	};
}

#endif
