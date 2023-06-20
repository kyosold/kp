AUTOMAKE_OPTIONS=foreign

# [动态库]
lib_LTLIBRARIES=libct_confparser.la
libct_confparser_la_SOURCES=iniparser.c dictionary.c confparser.c
libct_confparser_la_CFLAGS=-g -I./

# 拷贝头文件到{$prefix}/include/libct/confparser/下
confparserincludedir=$(includedir)/@PACKAGENAME@/confparser
confparserinclude_HEADERS=iniparser.h dictionary.h confparser.h

# 拷贝库文件到{$prefix}/lib64/libct/confparser/下
libdir=$(prefix)/lib64/@PACKAGENAME@/confparser

EXTRA_DIST=sample.c sample.ini

confparserdatadir=$(datarootdir)/@PACKAGENAME@/confparser
confparserdata_DATA=sample.c sample.ini
