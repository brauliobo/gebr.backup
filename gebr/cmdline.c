/*
  File autogenerated by gengetopt version 2.22
  generated with the following command:
  gengetopt --file-name=cmdline -C -a ggopt 

  The developers of gengetopt consider the fixed text that goes in all
  gengetopt output files to be in the public domain:
  we make no copyright claims on it.
*/

/* If we use autoconf.  */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getopt.h"

#include "cmdline.h"

const char *ggopt_purpose = "Seismic processing environment";

const char *ggopt_usage = "Usage: GêBR [OPTIONS]...";

const char *ggopt_description = "";

const char *ggopt_help[] = {
	"  -h, --help              Print help and exit",
	"  -V, --version           Print version and exit",
	"      --name=STRING       User name",
	"      --email=STRING      User email",
	"      --usermenus=STRING  User local menus dir",
	"      --data=STRING       Data user dir",
	"      --editor=STRING     HTML capable editor",
	"      --browser=STRING    HTML browser",
	"      --width=INT         Window width  (default=`700')",
	"      --height=INT        Window height  (default=`400')",
	"      --logexpand         Log exapander state  (default=off)",
	"      --server=STRING     Server hostname",
	0
};

typedef enum { ARG_NO, ARG_FLAG, ARG_STRING, ARG_INT
} cmdline_parser_arg_type;

static
void clear_given(struct ggopt *args_info);
static
void clear_args(struct ggopt *args_info);

static int
cmdline_parser_internal(int argc, char *const *argv, struct ggopt *args_info,
			struct cmdline_parser_params *params, const char *additional_error);

static int cmdline_parser_required2(struct ggopt *args_info, const char *prog_name, const char *additional_error);
struct line_list {
	char *string_arg;
	struct line_list *next;
};

static struct line_list *cmd_line_list = 0;
static struct line_list *cmd_line_list_tmp = 0;

static void free_cmd_list(void)
{
	/* free the list of a previous call */
	if (cmd_line_list) {
		while (cmd_line_list) {
			cmd_line_list_tmp = cmd_line_list;
			cmd_line_list = cmd_line_list->next;
			free(cmd_line_list_tmp->string_arg);
			free(cmd_line_list_tmp);
		}
	}
}

static char *gengetopt_strdup(const char *s);

static
void clear_given(struct ggopt *args_info)
{
	args_info->help_given = 0;
	args_info->version_given = 0;
	args_info->name_given = 0;
	args_info->email_given = 0;
	args_info->usermenus_given = 0;
	args_info->data_given = 0;
	args_info->editor_given = 0;
	args_info->browser_given = 0;
	args_info->width_given = 0;
	args_info->height_given = 0;
	args_info->logexpand_given = 0;
	args_info->server_given = 0;
}

static
void clear_args(struct ggopt *args_info)
{
	args_info->name_arg = NULL;
	args_info->name_orig = NULL;
	args_info->email_arg = NULL;
	args_info->email_orig = NULL;
	args_info->usermenus_arg = NULL;
	args_info->usermenus_orig = NULL;
	args_info->data_arg = NULL;
	args_info->data_orig = NULL;
	args_info->editor_arg = NULL;
	args_info->editor_orig = NULL;
	args_info->browser_arg = NULL;
	args_info->browser_orig = NULL;
	args_info->width_arg = 700;
	args_info->width_orig = NULL;
	args_info->height_arg = 400;
	args_info->height_orig = NULL;
	args_info->logexpand_flag = 0;
	args_info->server_arg = NULL;
	args_info->server_orig = NULL;

}

static
void init_args_info(struct ggopt *args_info)
{

	args_info->help_help = ggopt_help[0];
	args_info->version_help = ggopt_help[1];
	args_info->name_help = ggopt_help[2];
	args_info->email_help = ggopt_help[3];
	args_info->usermenus_help = ggopt_help[4];
	args_info->data_help = ggopt_help[5];
	args_info->editor_help = ggopt_help[6];
	args_info->browser_help = ggopt_help[7];
	args_info->width_help = ggopt_help[8];
	args_info->height_help = ggopt_help[9];
	args_info->logexpand_help = ggopt_help[10];
	args_info->server_help = ggopt_help[11];
	args_info->server_min = -1;
	args_info->server_max = -1;

}

void cmdline_parser_print_version(void)
{
	printf("%s %s\n", CMDLINE_PARSER_PACKAGE, CMDLINE_PARSER_VERSION);
}

static void print_help_common(void)
{
	cmdline_parser_print_version();

	if (strlen(ggopt_purpose) > 0)
		printf("\n%s\n", ggopt_purpose);

	if (strlen(ggopt_usage) > 0)
		printf("\n%s\n", ggopt_usage);

	printf("\n");

	if (strlen(ggopt_description) > 0)
		printf("%s\n", ggopt_description);
}

void cmdline_parser_print_help(void)
{
	int i = 0;
	print_help_common();
	while (ggopt_help[i])
		printf("%s\n", ggopt_help[i++]);
}

void cmdline_parser_init(struct ggopt *args_info)
{
	clear_given(args_info);
	clear_args(args_info);
	init_args_info(args_info);
}

void cmdline_parser_params_init(struct cmdline_parser_params *params)
{
	if (params) {
		params->override = 0;
		params->initialize = 1;
		params->check_required = 1;
		params->check_ambiguity = 0;
		params->print_errors = 1;
	}
}

struct cmdline_parser_params *cmdline_parser_params_create(void)
{
	struct cmdline_parser_params *params =
	    (struct cmdline_parser_params *)malloc(sizeof(struct cmdline_parser_params));
	cmdline_parser_params_init(params);
	return params;
}

static void free_string_field(char **s)
{
	if (*s) {
		free(*s);
		*s = 0;
	}
}

/** @brief generic value variable */
union generic_value {
	int int_arg;
	char *string_arg;
};

/** @brief holds temporary values for multiple options */
struct generic_list {
	union generic_value arg;
	char *orig;
	struct generic_list *next;
};

/**
 * @brief add a node at the head of the list 
 */
static void add_node(struct generic_list **list)
{
	struct generic_list *new_node = (struct generic_list *)malloc(sizeof(struct generic_list));
	new_node->next = *list;
	*list = new_node;
	new_node->arg.string_arg = NULL;
	new_node->orig = NULL;
}

static void free_multiple_string_field(unsigned int len, char ***arg, char ***orig)
{
	unsigned int i;
	if (*arg) {
		for (i = 0; i < len; ++i) {
			free_string_field(&((*arg)[i]));
			free_string_field(&((*orig)[i]));
		}
		free_string_field(&((*arg)[0]));	/* free default string */

		free(*arg);
		*arg = 0;
		free(*orig);
		*orig = 0;
	}
}

static void cmdline_parser_release(struct ggopt *args_info)
{

	free_string_field(&(args_info->name_arg));
	free_string_field(&(args_info->name_orig));
	free_string_field(&(args_info->email_arg));
	free_string_field(&(args_info->email_orig));
	free_string_field(&(args_info->usermenus_arg));
	free_string_field(&(args_info->usermenus_orig));
	free_string_field(&(args_info->data_arg));
	free_string_field(&(args_info->data_orig));
	free_string_field(&(args_info->editor_arg));
	free_string_field(&(args_info->editor_orig));
	free_string_field(&(args_info->browser_arg));
	free_string_field(&(args_info->browser_orig));
	free_string_field(&(args_info->width_orig));
	free_string_field(&(args_info->height_orig));
	free_multiple_string_field(args_info->server_given, &(args_info->server_arg), &(args_info->server_orig));

	clear_given(args_info);
}

static void write_into_file(FILE * outfile, const char *opt, const char *arg, char *values[])
{
	if (arg) {
		fprintf(outfile, "%s=\"%s\"\n", opt, arg);
	} else {
		fprintf(outfile, "%s\n", opt);
	}
}

static void write_multiple_into_file(FILE * outfile, int len, const char *opt, char **arg, char *values[])
{
	int i;

	for (i = 0; i < len; ++i)
		write_into_file(outfile, opt, (arg ? arg[i] : 0), values);
}

int cmdline_parser_dump(FILE * outfile, struct ggopt *args_info)
{
	int i = 0;

	if (!outfile) {
		fprintf(stderr, "%s: cannot dump options to stream\n", CMDLINE_PARSER_PACKAGE);
		return EXIT_FAILURE;
	}

	if (args_info->help_given)
		write_into_file(outfile, "help", 0, 0);
	if (args_info->version_given)
		write_into_file(outfile, "version", 0, 0);
	if (args_info->name_given)
		write_into_file(outfile, "name", args_info->name_orig, 0);
	if (args_info->email_given)
		write_into_file(outfile, "email", args_info->email_orig, 0);
	if (args_info->usermenus_given)
		write_into_file(outfile, "usermenus", args_info->usermenus_orig, 0);
	if (args_info->data_given)
		write_into_file(outfile, "data", args_info->data_orig, 0);
	if (args_info->editor_given)
		write_into_file(outfile, "editor", args_info->editor_orig, 0);
	if (args_info->browser_given)
		write_into_file(outfile, "browser", args_info->browser_orig, 0);
	if (args_info->width_given)
		write_into_file(outfile, "width", args_info->width_orig, 0);
	if (args_info->height_given)
		write_into_file(outfile, "height", args_info->height_orig, 0);
	if (args_info->logexpand_given)
		write_into_file(outfile, "logexpand", 0, 0);
	write_multiple_into_file(outfile, args_info->server_given, "server", args_info->server_orig, 0);

	i = EXIT_SUCCESS;
	return i;
}

int cmdline_parser_file_save(const char *filename, struct ggopt *args_info)
{
	FILE *outfile;
	int i = 0;

	outfile = fopen(filename, "w");

	if (!outfile) {
		fprintf(stderr, "%s: cannot open file for writing: %s\n", CMDLINE_PARSER_PACKAGE, filename);
		return EXIT_FAILURE;
	}

	i = cmdline_parser_dump(outfile, args_info);
	fclose(outfile);

	return i;
}

void cmdline_parser_free(struct ggopt *args_info)
{
	cmdline_parser_release(args_info);
}

/** @brief replacement of strdup, which is not standard */
char *gengetopt_strdup(const char *s)
{
	char *result = NULL;
	if (!s)
		return result;

	result = (char *)malloc(strlen(s) + 1);
	if (result == (char *)0)
		return (char *)0;
	strcpy(result, s);
	return result;
}

static char *get_multiple_arg_token(const char *arg)
{
	char *tok, *ret;
	size_t len, num_of_escape, i, j;

	if (!arg)
		return NULL;

	tok = strchr(arg, ',');
	num_of_escape = 0;

	/* make sure it is not escaped */
	while (tok) {
		if (*(tok - 1) == '\\') {
			/* find the next one */
			tok = strchr(tok + 1, ',');
			++num_of_escape;
		} else
			break;
	}

	if (tok)
		len = (size_t) (tok - arg + 1);
	else
		len = strlen(arg) + 1;

	len -= num_of_escape;

	ret = (char *)malloc(len);

	i = 0;
	j = 0;
	while (arg[i] && (j < len - 1)) {
		if (arg[i] == '\\' && arg[i + 1] && arg[i + 1] == ',')
			++i;

		ret[j++] = arg[i++];
	}

	ret[len - 1] = '\0';

	return ret;
}

static char *get_multiple_arg_token_next(const char *arg)
{
	char *tok;

	if (!arg)
		return NULL;

	tok = strchr(arg, ',');

	/* make sure it is not escaped */
	while (tok) {
		if (*(tok - 1) == '\\') {
			/* find the next one */
			tok = strchr(tok + 1, ',');
		} else
			break;
	}

	if (!tok || strlen(tok) == 1)
		return 0;

	return tok + 1;
}

static int
check_multiple_option_occurrences(const char *prog_name, unsigned int option_given, int min, int max,
				  const char *option_desc);

int
check_multiple_option_occurrences(const char *prog_name, unsigned int option_given, int min, int max,
				  const char *option_desc)
{
	int error = 0;

	if (option_given && !(min < 0 && max < 0)) {
		if (min >= 0 && max >= 0) {
			if (min == max) {
				/* specific occurrences */
				if (option_given != min) {
					fprintf(stderr, "%s: %s option occurrences must be %d\n",
						prog_name, option_desc, min);
					error = 1;
				}
			} else if (option_given < min || option_given > max) {
				/* range occurrences */
				fprintf(stderr, "%s: %s option occurrences must be between %d and %d\n",
					prog_name, option_desc, min, max);
				error = 1;
			}
		} else if (min >= 0) {
			/* at least check */
			if (option_given < min) {
				fprintf(stderr, "%s: %s option occurrences must be at least %d\n",
					prog_name, option_desc, min);
				error = 1;
			}
		} else if (max >= 0) {
			/* at most check */
			if (option_given > max) {
				fprintf(stderr, "%s: %s option occurrences must be at most %d\n",
					prog_name, option_desc, max);
				error = 1;
			}
		}
	}

	return error;
}

int cmdline_parser(int argc, char *const *argv, struct ggopt *args_info)
{
	return cmdline_parser2(argc, argv, args_info, 0, 1, 1);
}

int cmdline_parser_ext(int argc, char *const *argv, struct ggopt *args_info, struct cmdline_parser_params *params)
{
	int result;
	result = cmdline_parser_internal(argc, argv, args_info, params, NULL);

	if (result == EXIT_FAILURE) {
		cmdline_parser_free(args_info);
		exit(EXIT_FAILURE);
	}

	return result;
}

int
cmdline_parser2(int argc, char *const *argv, struct ggopt *args_info, int override, int initialize, int check_required)
{
	int result;
	struct cmdline_parser_params params;

	params.override = override;
	params.initialize = initialize;
	params.check_required = check_required;
	params.check_ambiguity = 0;
	params.print_errors = 1;

	result = cmdline_parser_internal(argc, argv, args_info, &params, NULL);

	if (result == EXIT_FAILURE) {
		cmdline_parser_free(args_info);
		exit(EXIT_FAILURE);
	}

	return result;
}

int cmdline_parser_required(struct ggopt *args_info, const char *prog_name)
{
	int result = EXIT_SUCCESS;

	if (cmdline_parser_required2(args_info, prog_name, NULL) > 0)
		result = EXIT_FAILURE;

	if (result == EXIT_FAILURE) {
		cmdline_parser_free(args_info);
		exit(EXIT_FAILURE);
	}

	return result;
}

int cmdline_parser_required2(struct ggopt *args_info, const char *prog_name, const char *additional_error)
{
	int error = 0;

	/* checks for required options */
	if (!args_info->usermenus_given) {
		fprintf(stderr, "%s: '--usermenus' option required%s\n", prog_name,
			(additional_error ? additional_error : ""));
		error = 1;
	}

	if (!args_info->data_given) {
		fprintf(stderr, "%s: '--data' option required%s\n", prog_name,
			(additional_error ? additional_error : ""));
		error = 1;
	}

	if (check_multiple_option_occurrences
	    (prog_name, args_info->server_given, args_info->server_min, args_info->server_max, "'--server'"))
		error = 1;

	/* checks for dependences among options */

	return error;
}

static char *package_name = 0;

/**
 * @brief updates an option
 * @param field the generic pointer to the field to update
 * @param orig_field the pointer to the orig field
 * @param field_given the pointer to the number of occurrence of this option
 * @param prev_given the pointer to the number of occurrence already seen
 * @param value the argument for this option (if null no arg was specified)
 * @param possible_values the possible values for this option (if specified)
 * @param default_value the default value (in case the option only accepts fixed values)
 * @param arg_type the type of this option
 * @param check_ambiguity @see cmdline_parser_params.check_ambiguity
 * @param override @see cmdline_parser_params.override
 * @param no_free whether to free a possible previous value
 * @param multiple_option whether this is a multiple option
 * @param long_opt the corresponding long option
 * @param short_opt the corresponding short option (or '-' if none)
 * @param additional_error possible further error specification
 */
static
int update_arg(void *field, char **orig_field,
	       unsigned int *field_given, unsigned int *prev_given,
	       char *value, char *possible_values[], const char *default_value,
	       cmdline_parser_arg_type arg_type,
	       int check_ambiguity, int override,
	       int no_free, int multiple_option, const char *long_opt, char short_opt, const char *additional_error)
{
	char *stop_char = 0;
	const char *val = value;
	int found;
	char **string_field;

	stop_char = 0;
	found = 0;

	if (!multiple_option && prev_given && (*prev_given || (check_ambiguity && *field_given))) {
		if (short_opt != '-')
			fprintf(stderr, "%s: `--%s' (`-%c') option given more than once%s\n",
				package_name, long_opt, short_opt, (additional_error ? additional_error : ""));
		else
			fprintf(stderr, "%s: `--%s' option given more than once%s\n",
				package_name, long_opt, (additional_error ? additional_error : ""));
		return 1;	/* failure */
	}

	if (field_given && *field_given && !override)
		return 0;
	if (prev_given)
		(*prev_given)++;
	if (field_given)
		(*field_given)++;
	if (possible_values)
		val = possible_values[found];

	switch (arg_type) {
	case ARG_FLAG:
		*((int *)field) = !*((int *)field);
		break;
	case ARG_INT:
		if (val)
			*((int *)field) = strtol(val, &stop_char, 0);
		break;
	case ARG_STRING:
		if (val) {
			string_field = (char **)field;
			if (!no_free && *string_field)
				free(*string_field);	/* free previous string */
			*string_field = gengetopt_strdup(val);
		}
		break;
	default:
		break;
	};

	/* check numeric conversion */
	switch (arg_type) {
	case ARG_INT:
		if (val && !(stop_char && *stop_char == '\0')) {
			fprintf(stderr, "%s: invalid numeric value: %s\n", package_name, val);
			return 1;	/* failure */
		}
		break;
	default:
		;
	};

	/* store the original value */
	switch (arg_type) {
	case ARG_NO:
	case ARG_FLAG:
		break;
	default:
		if (value && orig_field) {
			if (no_free) {
				*orig_field = value;
			} else {
				if (*orig_field)
					free(*orig_field);	/* free previous string */
				*orig_field = gengetopt_strdup(value);
			}
		}
	};

	return 0;		/* OK */
}

/**
 * @brief store information about a multiple option in a temporary list
 * @param list where to (temporarily) store multiple options
 */
static
int update_multiple_arg_temp(struct generic_list **list,
			     unsigned int *prev_given, const char *val,
			     char *possible_values[], const char *default_value,
			     cmdline_parser_arg_type arg_type,
			     const char *long_opt, char short_opt, const char *additional_error)
{
	char *multi_token, *multi_next;	/* store single arguments */

	if (arg_type == ARG_NO) {
		(*prev_given)++;
		return 0;	/* OK */
	}

	multi_token = get_multiple_arg_token(val);
	multi_next = get_multiple_arg_token_next(val);

	while (1) {
		add_node(list);
		if (update_arg((void *)&((*list)->arg), &((*list)->orig), 0,
			       prev_given, multi_token, possible_values, default_value,
			       arg_type, 0, 1, 1, 1, long_opt, short_opt, additional_error)) {
			if (multi_token)
				free(multi_token);
			return 1;	/* failure */
		}

		if (multi_next) {
			multi_token = get_multiple_arg_token(multi_next);
			multi_next = get_multiple_arg_token_next(multi_next);
		} else
			break;
	}

	return 0;		/* OK */
}

/**
 * @brief free the passed list (including possible string argument)
 */
static
void free_list(struct generic_list *list, short string_arg)
{
	if (list) {
		struct generic_list *tmp;
		while (list) {
			tmp = list;
			if (string_arg && list->arg.string_arg)
				free(list->arg.string_arg);
			if (list->orig)
				free(list->orig);
			list = list->next;
			free(tmp);
		}
	}
}

/**
 * @brief updates a multiple option starting from the passed list
 */
static
void update_multiple_arg(void *field, char ***orig_field,
			 unsigned int field_given, unsigned int prev_given, union generic_value *default_value,
			 cmdline_parser_arg_type arg_type, struct generic_list *list)
{
	int i;
	struct generic_list *tmp;

	if (prev_given && list) {
		*orig_field = (char **)realloc(*orig_field, (field_given + prev_given) * sizeof(char *));

		switch (arg_type) {
		case ARG_INT:
			*((int **)field) = (int *)realloc(*((int **)field), (field_given + prev_given) * sizeof(int));
			break;
		case ARG_STRING:
			*((char ***)field) =
			    (char **)realloc(*((char ***)field), (field_given + prev_given) * sizeof(char *));
			break;
		default:
			break;
		};

		for (i = (prev_given - 1); i >= 0; --i) {
			tmp = list;

			switch (arg_type) {
			case ARG_INT:
				(*((int **)field))[i + field_given] = tmp->arg.int_arg;
				break;
			case ARG_STRING:
				(*((char ***)field))[i + field_given] = tmp->arg.string_arg;
				break;
			default:
				break;
			}
			(*orig_field)[i + field_given] = list->orig;
			list = list->next;
			free(tmp);
		}
	} else {		/* set the default value */
		if (default_value && !field_given) {
			switch (arg_type) {
			case ARG_INT:
				if (!*((int **)field)) {
					*((int **)field) = (int *)malloc(sizeof(int));
					(*((int **)field))[0] = default_value->int_arg;
				}
				break;
			case ARG_STRING:
				if (!*((char ***)field)) {
					*((char ***)field) = (char **)malloc(sizeof(char *));
					(*((char ***)field))[0] = gengetopt_strdup(default_value->string_arg);
				}
				break;
			default:
				break;
			}
			if (!(*orig_field)) {
				*orig_field = (char **)malloc(sizeof(char *));
				(*orig_field)[0] = NULL;
			}
		}
	}
}

int
cmdline_parser_internal(int argc, char *const *argv, struct ggopt *args_info,
			struct cmdline_parser_params *params, const char *additional_error)
{
	int c;			/* Character of the parsed option.  */

	struct generic_list *server_list = NULL;
	int error = 0;
	struct ggopt local_args_info;

	int override;
	int initialize;
	int check_required;
	int check_ambiguity;

	package_name = argv[0];

	override = params->override;
	initialize = params->initialize;
	check_required = params->check_required;
	check_ambiguity = params->check_ambiguity;

	if (initialize)
		cmdline_parser_init(args_info);

	cmdline_parser_init(&local_args_info);

	optarg = 0;
	optind = 0;
	opterr = params->print_errors;
	optopt = '?';

	while (1) {
		int option_index = 0;

		static struct option long_options[] = {
			{"help", 0, NULL, 'h'},
			{"version", 0, NULL, 'V'},
			{"name", 1, NULL, 0},
			{"email", 1, NULL, 0},
			{"usermenus", 1, NULL, 0},
			{"data", 1, NULL, 0},
			{"editor", 1, NULL, 0},
			{"browser", 1, NULL, 0},
			{"width", 1, NULL, 0},
			{"height", 1, NULL, 0},
			{"logexpand", 0, NULL, 0},
			{"server", 1, NULL, 0},
			{NULL, 0, NULL, 0}
		};

		c = getopt_long(argc, argv, "hV", long_options, &option_index);

		if (c == -1)
			break;	/* Exit from `while (1)' loop.  */

		switch (c) {
		case 'h':	/* Print help and exit.  */
			cmdline_parser_print_help();
			cmdline_parser_free(&local_args_info);
			exit(EXIT_SUCCESS);

		case 'V':	/* Print version and exit.  */
			cmdline_parser_print_version();
			cmdline_parser_free(&local_args_info);
			exit(EXIT_SUCCESS);

		case 0:	/* Long option with no short option */
			/* User name.  */
			if (strcmp(long_options[option_index].name, "name") == 0) {

				if (update_arg((void *)&(args_info->name_arg),
					       &(args_info->name_orig), &(args_info->name_given),
					       &(local_args_info.name_given), optarg, 0, 0, ARG_STRING,
					       check_ambiguity, override, 0, 0, "name", '-', additional_error))
					goto failure;

			}
			/* User email.  */
			else if (strcmp(long_options[option_index].name, "email") == 0) {

				if (update_arg((void *)&(args_info->email_arg),
					       &(args_info->email_orig), &(args_info->email_given),
					       &(local_args_info.email_given), optarg, 0, 0, ARG_STRING,
					       check_ambiguity, override, 0, 0, "email", '-', additional_error))
					goto failure;

			}
			/* User local menus dir.  */
			else if (strcmp(long_options[option_index].name, "usermenus") == 0) {

				if (update_arg((void *)&(args_info->usermenus_arg),
					       &(args_info->usermenus_orig), &(args_info->usermenus_given),
					       &(local_args_info.usermenus_given), optarg, 0, 0, ARG_STRING,
					       check_ambiguity, override, 0, 0, "usermenus", '-', additional_error))
					goto failure;

			}
			/* Data user dir.  */
			else if (strcmp(long_options[option_index].name, "data") == 0) {

				if (update_arg((void *)&(args_info->data_arg),
					       &(args_info->data_orig), &(args_info->data_given),
					       &(local_args_info.data_given), optarg, 0, 0, ARG_STRING,
					       check_ambiguity, override, 0, 0, "data", '-', additional_error))
					goto failure;

			}
			/* HTML capable editor.  */
			else if (strcmp(long_options[option_index].name, "editor") == 0) {

				if (update_arg((void *)&(args_info->editor_arg),
					       &(args_info->editor_orig), &(args_info->editor_given),
					       &(local_args_info.editor_given), optarg, 0, 0, ARG_STRING,
					       check_ambiguity, override, 0, 0, "editor", '-', additional_error))
					goto failure;

			}
			/* HTML browser.  */
			else if (strcmp(long_options[option_index].name, "browser") == 0) {

				if (update_arg((void *)&(args_info->browser_arg),
					       &(args_info->browser_orig), &(args_info->browser_given),
					       &(local_args_info.browser_given), optarg, 0, 0, ARG_STRING,
					       check_ambiguity, override, 0, 0, "browser", '-', additional_error))
					goto failure;

			}
			/* Window width.  */
			else if (strcmp(long_options[option_index].name, "width") == 0) {

				if (update_arg((void *)&(args_info->width_arg),
					       &(args_info->width_orig), &(args_info->width_given),
					       &(local_args_info.width_given), optarg, 0, "700", ARG_INT,
					       check_ambiguity, override, 0, 0, "width", '-', additional_error))
					goto failure;

			}
			/* Window height.  */
			else if (strcmp(long_options[option_index].name, "height") == 0) {

				if (update_arg((void *)&(args_info->height_arg),
					       &(args_info->height_orig), &(args_info->height_given),
					       &(local_args_info.height_given), optarg, 0, "400", ARG_INT,
					       check_ambiguity, override, 0, 0, "height", '-', additional_error))
					goto failure;

			}
			/* Log exapander state.  */
			else if (strcmp(long_options[option_index].name, "logexpand") == 0) {

				if (update_arg((void *)&(args_info->logexpand_flag), 0, &(args_info->logexpand_given),
					       &(local_args_info.logexpand_given), optarg, 0, 0, ARG_FLAG,
					       check_ambiguity, override, 1, 0, "logexpand", '-', additional_error))
					goto failure;

			}
			/* Server hostname.  */
			else if (strcmp(long_options[option_index].name, "server") == 0) {

				if (update_multiple_arg_temp(&server_list,
							     &(local_args_info.server_given), optarg, 0, 0, ARG_STRING,
							     "server", '-', additional_error))
					goto failure;

			}

			break;
		case '?':	/* Invalid option.  */
			/* `getopt_long' already printed an error message.  */
			goto failure;

		default:	/* bug: option not considered.  */
			fprintf(stderr, "%s: option unknown: %c%s\n", CMDLINE_PARSER_PACKAGE, c,
				(additional_error ? additional_error : ""));
			abort();
		}		/* switch */
	}			/* while */

	update_multiple_arg((void *)&(args_info->server_arg),
			    &(args_info->server_orig), args_info->server_given,
			    local_args_info.server_given, 0, ARG_STRING, server_list);

	args_info->server_given += local_args_info.server_given;
	local_args_info.server_given = 0;

	if (check_required) {
		error += cmdline_parser_required2(args_info, argv[0], additional_error);
	}

	cmdline_parser_release(&local_args_info);

	if (error)
		return (EXIT_FAILURE);

	return 0;

 failure:
	free_list(server_list, 1);

	cmdline_parser_release(&local_args_info);
	return (EXIT_FAILURE);
}

#ifndef CONFIG_FILE_LINE_SIZE
#define CONFIG_FILE_LINE_SIZE 2048
#endif
#define ADDITIONAL_ERROR " in configuration file "

#define CONFIG_FILE_LINE_BUFFER_SIZE (CONFIG_FILE_LINE_SIZE+3)
/* 3 is for "--" and "=" */

static int _cmdline_parser_configfile(char *const filename, int *my_argc)
{
	FILE *file;
	char my_argv[CONFIG_FILE_LINE_BUFFER_SIZE + 1];
	char linebuf[CONFIG_FILE_LINE_SIZE];
	int line_num = 0;
	int result = 0, equal;
	char *fopt, *farg;
	char *str_index;
	size_t len, next_token;
	char delimiter;

	if ((file = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "%s: Error opening configuration file '%s'\n", CMDLINE_PARSER_PACKAGE, filename);
		return EXIT_FAILURE;
	}

	while ((fgets(linebuf, CONFIG_FILE_LINE_SIZE, file)) != NULL) {
		++line_num;
		my_argv[0] = '\0';
		len = strlen(linebuf);
		if (len > (CONFIG_FILE_LINE_BUFFER_SIZE - 1)) {
			fprintf(stderr, "%s:%s:%d: Line too long in configuration file\n",
				CMDLINE_PARSER_PACKAGE, filename, line_num);
			result = EXIT_FAILURE;
			break;
		}

		/* find first non-whitespace character in the line */
		next_token = strspn(linebuf, " \t\r\n");
		str_index = linebuf + next_token;

		if (str_index[0] == '\0' || str_index[0] == '#')
			continue;	/* empty line or comment line is skipped */

		fopt = str_index;

		/* truncate fopt at the end of the first non-valid character */
		next_token = strcspn(fopt, " \t\r\n=");

		if (fopt[next_token] == '\0') {	/* the line is over */
			farg = NULL;
			equal = 0;
			goto noarg;
		}

		/* remember if equal sign is present */
		equal = (fopt[next_token] == '=');
		fopt[next_token++] = '\0';

		/* advance pointers to the next token after the end of fopt */
		next_token += strspn(fopt + next_token, " \t\r\n");

		/* check for the presence of equal sign, and if so, skip it */
		if (!equal)
			if ((equal = (fopt[next_token] == '='))) {
				next_token++;
				next_token += strspn(fopt + next_token, " \t\r\n");
			}
		str_index += next_token;

		/* find argument */
		farg = str_index;
		if (farg[0] == '\"' || farg[0] == '\'') {	/* quoted argument */
			str_index = strchr(++farg, str_index[0]);	/* skip opening quote */
			if (!str_index) {
				fprintf
				    (stderr,
				     "%s:%s:%d: unterminated string in configuration file\n",
				     CMDLINE_PARSER_PACKAGE, filename, line_num);
				result = EXIT_FAILURE;
				break;
			}
		} else {	/* read up the remaining part up to a delimiter */
			next_token = strcspn(farg, " \t\r\n#\'\"");
			str_index += next_token;
		}

		/* truncate farg at the delimiter and store it for further check */
		delimiter = *str_index, *str_index++ = '\0';

		/* everything but comment is illegal at the end of line */
		if (delimiter != '\0' && delimiter != '#') {
			str_index += strspn(str_index, " \t\r\n");
			if (*str_index != '\0' && *str_index != '#') {
				fprintf
				    (stderr,
				     "%s:%s:%d: malformed string in configuration file\n",
				     CMDLINE_PARSER_PACKAGE, filename, line_num);
				result = EXIT_FAILURE;
				break;
			}
		}

 noarg:
		if (!strcmp(fopt, "include")) {
			if (farg && *farg) {
				result = _cmdline_parser_configfile(farg, my_argc);
			} else {
				fprintf(stderr, "%s:%s:%d: include requires a filename argument.\n",
					CMDLINE_PARSER_PACKAGE, filename, line_num);
			}
			continue;
		}
		len = strlen(fopt);
		strcat(my_argv, len > 1 ? "--" : "-");
		strcat(my_argv, fopt);
		if (len > 1 && ((farg && *farg) || equal))
			strcat(my_argv, "=");
		if (farg && *farg)
			strcat(my_argv, farg);
		++(*my_argc);

		cmd_line_list_tmp = (struct line_list *)malloc(sizeof(struct line_list));
		cmd_line_list_tmp->next = cmd_line_list;
		cmd_line_list = cmd_line_list_tmp;
		cmd_line_list->string_arg = gengetopt_strdup(my_argv);
	}			/* while */

	if (file)
		fclose(file);
	return result;
}

int
cmdline_parser_configfile(char *const filename,
			  struct ggopt *args_info, int override, int initialize, int check_required)
{
	struct cmdline_parser_params params;

	params.override = override;
	params.initialize = initialize;
	params.check_required = check_required;
	params.check_ambiguity = 0;
	params.print_errors = 1;

	return cmdline_parser_config_file(filename, args_info, &params);
}

int cmdline_parser_config_file(char *const filename, struct ggopt *args_info, struct cmdline_parser_params *params)
{
	int i, result;
	int my_argc = 1;
	char **my_argv_arg;
	char *additional_error;

	/* store the program name */
	cmd_line_list_tmp = (struct line_list *)malloc(sizeof(struct line_list));
	cmd_line_list_tmp->next = cmd_line_list;
	cmd_line_list = cmd_line_list_tmp;
	cmd_line_list->string_arg = gengetopt_strdup(CMDLINE_PARSER_PACKAGE);

	result = _cmdline_parser_configfile(filename, &my_argc);

	if (result != EXIT_FAILURE) {
		my_argv_arg = (char **)malloc((my_argc + 1) * sizeof(char *));
		cmd_line_list_tmp = cmd_line_list;

		for (i = my_argc - 1; i >= 0; --i) {
			my_argv_arg[i] = cmd_line_list_tmp->string_arg;
			cmd_line_list_tmp = cmd_line_list_tmp->next;
		}

		my_argv_arg[my_argc] = 0;

		additional_error = (char *)malloc(strlen(filename) + strlen(ADDITIONAL_ERROR) + 1);
		strcpy(additional_error, ADDITIONAL_ERROR);
		strcat(additional_error, filename);
		result = cmdline_parser_internal(my_argc, my_argv_arg, args_info, params, additional_error);

		free(additional_error);
		free(my_argv_arg);
	}

	free_cmd_list();
	if (result == EXIT_FAILURE) {
		cmdline_parser_free(args_info);
		exit(EXIT_FAILURE);
	}

	return result;
}
