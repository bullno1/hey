#define HEY_IMPLEMENTATION
#define SOKOL_IMPL

#include <hey_dsl.h>
#include <hey.h>
#include <hey_script.h>
#include "common.h"

static void
scripting(hey_exec_t* ctx, void* userdata) {
	(void)userdata;

	hey_script_action_def_t* actions = HEY_ARRAY(hey_script_action_def_t,
		{
			.name = HEY_STR("spawn"),
			.description = HEY_STR("Spawn an entity"),
			.example_description = HEY_STR("Spawn an entity called 'monster'"),
			.args = HEY_ARRAY(hey_script_arg_def_t,
				{
					.name = HEY_STR("id"),
					.description = HEY_STR("Unique id for this entity"),
					.example = HEY_STR("monster"),
					.parser = hey_script_string_parser,
				}
			),
		},
		{
			.name = HEY_STR("dialog"),
			.description = HEY_STR("Show a speech bubble"),
			.example_description = HEY_STR("Make an entity called 'player' say 'Oh my god'"),
			.args = HEY_ARRAY(hey_script_arg_def_t,
				{
					.name = HEY_STR("entity_id"),
					.description = HEY_STR("The unique id of the entity that will say something. This entity must first be created through `spawn`"),
					.example = HEY_STR("player"),
					.parser = hey_script_string_parser,
				},
				{
					.name = HEY_STR("text"),
					.description = HEY_STR("The line to say"),
					.example = HEY_STR("Oh my god"),
					.parser = hey_script_string_parser,
				}
			),
		},
		{
			.name = HEY_STR("narration"),
			.description = HEY_STR("For anything from the narrator's point of view that does not affect any game entity"),
			.example_description = HEY_STR("It was a beautiful morning. The birds were chirping."),
			.args = HEY_ARRAY(hey_script_arg_def_t,
				{
					.name = HEY_STR("content"),
					.description = HEY_STR("The narration"),
					.example = HEY_STR("It was a beautiful morning. The birds were chirping."),
					.parser = hey_script_string_parser,
				}
			),
		}
	);

	hey_dsl(ctx) {
		h_lit_special(
			"<s><|system|>\n"
			"You are a game developer. Your job is to write cut scene scripts in YAML. You have access to the following primitives:\n\n"
		);

		for (hey_index_t i = 0; actions[i].name.chars != NULL; ++i) {
			const hey_script_action_def_t* action = &actions[i];
			h_fmt(
				"- name: %.*s\n"
				"  description: %.*s\n",
				action->name.length, action->name.chars,
				action->description.length, action->description.chars
			);

			h_lit("  arguments:\n");
			for (hey_index_t j = 0; action->args[j].name.chars != NULL; ++j) {
				const hey_script_arg_def_t* arg = &action->args[j];
				h_fmt(
					"    %.*s: %.*s\n",
					arg->name.length, arg->name.chars,
					arg->description.length, arg->description.chars
				);
			}
		}
		h_lit_special("</s>\n");

		for (hey_index_t i = 0; actions[i].name.chars != NULL; ++i) {
			const hey_script_action_def_t* action = &actions[i];
			h_fmt_special(
				"<|user|>\n"
				"Write a script for this scenario: %.*s",
				action->example_description.length, action->example_description.chars
			);
			h_lit_special("</s>\n<|assistant|>\n```\n");
			hey_script_push_example(ctx, action);
			h_lit("```");
			h_lit_special("</s>\n");
		}
		h_lit_special(
			"<|user|>\n"
			"Write a script for this scenario: "
			"It was a stormy night. Bob is sound asleep.\nSuddenly he heard a noise. He wakes up and says 'What?'.\n"
			"Dave arrive.\nHe says 'Hello'.</s>\n"
			"<|assistant|>\n```\n"
		);
		hey_script_generate(
			ctx,
			(hey_script_receiver_t){ 0 },
			HEY_STR("```"),
			actions
		);
	}
}

int
main(int argc, const char* argv[]) {
	return example_main(argc, argv, scripting);
}
