/*
* Copyright (c) 2019 Pedro Falcato
* This file is part of Carbon, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#include <carbon/panic.h>
#include <carbon/fs/vfs.h>
#include <carbon/fs/root.h>
#include <carbon/vector.h>

#include <libgen.h>

typedef struct star_header
{
	char filename[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char typeflag;
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[131];
	char atime[12];
	char ctime[12];
} __attribute__((packed)) tar_header_t;

#define TAR_TYPE_FILE		'0'
#define TAR_TYPE_HARD_LNK 	'1'
#define TAR_TYPE_SYMLNK		'2'
#define TAR_TYPE_CHAR_SPECIAL	'3'
#define TAR_TYPE_BLOCK_SPECIAL	'4'
#define TAR_TYPE_DIR		'5'

static inline size_t tar_get_size(const char *in)
{
    	size_t size = 0;
    	unsigned int j;
    	unsigned int count = 1;
    	for (j = 11; j > 0; j--, count *= 8)
		size += ((in[j - 1] - '0') * count);
    	return size;
}

vector<tar_header_t *> headers = { };
size_t n_files = 0;
size_t tar_parse(uintptr_t address)
{
	size_t i = 0;

	for (i = 0;; i++)
	{
		tar_header_t *header = (tar_header_t *) address;

		if (header->filename[0] == '\0')
			break;
		/* Remove the trailing slash */
		if(header->filename[strlen(header->filename)-1] == '/')
			header->filename[strlen(header->filename)-1] = 0;

		size_t size = tar_get_size(header->size);
		
		assert(headers.push_back(header) != false);

		address += ((size / 512) + 1) * 512;
		if (size % 512)
			address += 512;
		n_files++;
	}

	return i;
}

void initrd_mount(void)
{
	for(auto header : headers)
	{
		char *saveptr;
		char *filename = strdup(header->filename);

		char *old = filename;

		assert(filename != NULL);

		filename = dirname(filename);
		
		filename = strtok_r(filename, "/", &saveptr);

		inode *node = get_root()->get_inode();
		if(*filename != '.' && strlen(filename) != 1)
		{

			while(filename)
			{
				inode *last = node;
				if(!(node = fs::open(filename, fs::open_flags::none, node)))
				{
					node = last;
					if(!(node = fs::mkdir(filename, 0777, node)))
					{
						panic("Error loading initrd");
					}
				}
				filename = strtok_r(NULL, "/", &saveptr);
			}
		}
		/* After creat/opening the directories, create it and populate it */
		strcpy(old, header->filename);
		filename = old;
		filename = basename(filename);

		if(header->typeflag == TAR_TYPE_FILE)
		{
			inode *file = fs::create(filename, S_IFREG | 0666, node);
			assert(file != NULL);
	
			char *buffer = (char *) header + 512;
			size_t size = tar_get_size(header->size);
			assert(fs::write(buffer, size, 0, file) != -1);
		}
		else if(header->typeflag == TAR_TYPE_DIR)
		{
			inode *file = fs::mkdir(filename, 0666, node);

			assert(file != NULL);
		}
		else if(header->typeflag == TAR_TYPE_SYMLNK)
		{
			//char *buffer = (char *) iter[i]->linkname;
			/* inode *file = creat_vfs(node, filename, 0666);
			assert(file != NULL);

			assert(symlink_vfs(buffer, file) == 0);*/
			printf("TODO: Add symlink\n");
		}

		free((void *) old);
	}
}

void initrd_init(struct module *mod)
{
	tar_parse(mod->start);
	initrd_mount();
}