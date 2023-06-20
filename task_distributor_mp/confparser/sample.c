#include <stdio.h>

#include "confparser.h"
#include "dictionary.h"

void close_config(dictionary *dict)
{
	if (dict) {
		dictionary_del(dict);
		dict = NULL;
	}
}

dictionary *read_config(char *conf_file)
{
	dictionary *dict = open_conf_file(conf_file);
	if (dict == NULL) {
		printf("parse config file fail when open_conf_file file:%s\n", conf_file);
		return NULL;
	}

	return dict;
}

int main(int argc, char **argv)
{
	char *cfile = argv[1];
	
	dictionary *dict = NULL;

	// read config and return dictonary
	dict = read_config(cfile);
	if (dict == NULL) {
		printf("read_config fail:%s\n", cfile);
		return 1;
	}

	// ----- Master -----
	printf("=============== Read Config [Master]:\n");	
	char *plog_level = dictionary_get(dict, "master:log_level", NULL);
	if (plog_level == NULL) {
		printf("read [master] log_level fail\n");
		return 1;
	}
	printf("log_level:%s\n", plog_level);

	char *pmax_childs = dictionary_get(dict, "master:max_childs", NULL);
	if (pmax_childs == NULL) {
		printf("read [master] max_childs fail\n");
		return 1;
	}
	printf("max_childs=%d\n", atoi(pmax_childs));
	
	char *plog_file = dictionary_get(dict, "master:log_file", NULL);
	if (plog_file == NULL) {
		printf("read [master] log_file fail\n");
		return 1;
	}
	printf("log_file:%s\n", plog_file);

	// ----- Child -----
	printf("=============== Read Config [Child]:\n");	
	char *ptimeout = dictionary_get(dict, "child:timeout", NULL);
	if (ptimeout == NULL) {
		printf("read [child] timeout fail\n");
		return 1;
	}
	printf("timeout:%s\n", ptimeout);

	char *pmc_host = dictionary_get(dict, "child:mc_host", NULL);
	if (pmc_host == NULL) {
		printf("read [child] mc_host fail\n");
		return 1;
	}
	printf("mc_host:%s\n", pmc_host);

	char *pmc_port = dictionary_get(dict, "child:mc_port", NULL);
	if (pmc_port == NULL) {
		printf("read [child] mc_port fail\n");
		return 1;
	}
	printf("mc_port:%s\n", pmc_port);

	char *pmc_timeout = dictionary_get(dict, "child:mc_timeout", NULL);
	if (pmc_timeout == NULL) {
		printf("read [child] mc_timeout fail\n");
		return 1;
	}
	printf("mc_timeout:%s\n", pmc_timeout);

	close_config(dict);

	return 0;
}
