#define HEY_IMPLEMENTATION
#define SOKOL_IMPL

#include <hey_dsl.h>
#include <hey.h>
#include <hey_script.h>
#include "common.h"

static void
scripting(hey_exec_t* ctx, void* userdata) {
	const exec_input_t* input = userdata;

	hey_script_action_def_t* actions = HEY_ARRAY(hey_script_action_def_t,
		{
			.name = HEY_STR("spawn_character"),
			.description = HEY_STR("Spawn a character"),
			.example_description = HEY_STR("Spawn an angry dagger wielding goblin called `goblin`"),
			.args = HEY_ARRAY(hey_script_arg_def_t,
				{
					.name = HEY_STR("id"),
					.description = HEY_STR("Unique id for this character"),
					.example = HEY_STR("goblin"),
					.parser = hey_script_string_parser,
				},
				{
					.name = HEY_STR("description"),
					.description = HEY_STR("A short description for this entity"),
					.example = HEY_STR("It is a dagger wielding goblin."),
					.parser = hey_script_string_parser,
				}
			),
		},
		{
			.name = HEY_STR("spawn_object"),
			.description = HEY_STR("Spawn an object"),
			.example_description = HEY_STR("Spawn a shiny dagger lying on the ground called `dagger`"),
			.args = HEY_ARRAY(hey_script_arg_def_t,
				{
					.name = HEY_STR("id"),
					.description = HEY_STR("Unique id for this object"),
					.example = HEY_STR("dagger"),
					.parser = hey_script_string_parser,
				},
				{
					.name = HEY_STR("description"),
					.description = HEY_STR("A short description for this object"),
					.example = HEY_STR("The dagger is shining."),
					.parser = hey_script_string_parser,
				}
			),
		},
		{
			.name = HEY_STR("dialog"),
			.description = HEY_STR("Show a speech bubble"),
			.example_description = HEY_STR("Make an entity called `player` say 'Oh my god'"),
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
			.name = HEY_STR("sound"),
			.description = HEY_STR("To play a sound effect"),
			.example_description = HEY_STR("The clock is ticking."),
			.args = HEY_ARRAY(hey_script_arg_def_t,
				{
					.name = HEY_STR("description"),
					.description = HEY_STR("Description of the sound"),
					.example = HEY_STR("Tik tok tik tok"),
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
			"You are an expert in scripting. You assist the user in writing in a scripting language based on YAML. You have access to the following primitives:\n\n"
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
		h_fmt_special(
			"<|user|>\n"
			"Write a script for this scenario: %.*s</s>\n"
			"<|assistant|>\n",
			input->input_string.length, input->input_string.chars
		);
		h_lit(
			"Let's break down the scene into individual action before writing the script:\n"
			"* First, we introduce the characters.\n"
		);

		// Generate step-by-step plan until the model decides to start scripting
		const hey_llm_t* llm = hey_get_llm(ctx);
		bool fence_generated = false;
		while (true) {
			// TODO: To ensure that eventually the brain storming must end,
			// gradually boost the strength of ```
			hey_index_t choice = h_choose(HEY_STR("*"), HEY_STR("```"), HEY_STR("\n"));
			if (choice != 0) { break; }
			fence_generated = choice == 1;

			h_generate(.controller = hey_ends_at_token(llm->nl));
		}
		if (fence_generated) {
			h_lit("\n");
		} else {
			h_lit("```\n");
		}

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
