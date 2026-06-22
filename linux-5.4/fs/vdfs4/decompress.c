/**
 * VDFS4 -- Vertically Deliberate improved performance File System
 *
 * Copyright 2012 by Samsung Electronics, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include <linux/zlib.h>
#include <linux/stat.h>
#include <linux/namei.h>
#include <linux/vmalloc.h>
#include "vdfs4.h"
#include "debug.h"
#include <linux/lzo.h>
#include <linux/zstd.h>

#define list_to_page(head) (list_entry((head)->prev, struct page, lru))
#define list_to_page_index(pos, head, index) \
	for (pos = list_entry((head)->prev, struct page, lru); \
		page_folio(pos)->index != index;\
	pos = list_entry((pos)->prev, struct page, lru))

enum compr_type vdfs4_get_comprtype_by_magic(const char *magic, int len)
{
	if (!memcmp(magic, VDFS4_COMPR_LZO_FILE_DESCR_MAGIC, len))
		return VDFS4_COMPR_LZO;

	if (!memcmp(magic, VDFS4_COMPR_ZIP_FILE_DESCR_MAGIC, len))
		return VDFS4_COMPR_ZLIB;

	if (!memcmp(magic, VDFS4_COMPR_GZIP_FILE_DESCR_MAGIC, len))
		return VDFS4_COMPR_GZIP;

	if (!memcmp(magic, VDFS4_COMPR_ZSTD_FILE_DESCR_MAGIC, len))
		return VDFS4_COMPR_ZSTD;

	return -EINVAL;
}

decomp_fn_t vdfs4_get_decomp_fn(enum compr_type type)
{
	decomp_fn_t fn;

	switch (type) {
	case VDFS4_COMPR_ZLIB:
		fn = vdfs4_unpack_chunk_zlib;
		break;
	case VDFS4_COMPR_GZIP:
		fn = vdfs4_unpack_chunk_gzip;
		break;
	case VDFS4_COMPR_LZO:
		fn = vdfs4_unpack_chunk_lzo;
		break;
	case VDFS4_COMPR_ZSTD:
		fn = vdfs4_unpack_chunk_zstd;
		break;
	default:
		fn = NULL;
		break;
	}

	return fn;
}

static int sw_decompress(z_stream *strm, char *ibuff, unsigned long ilen,
	char *obuff, unsigned long olen, int compr_type)
{
	int rc = 0;

	strm->avail_out = olen;
	strm->next_out = obuff;
	if (compr_type == VDFS4_COMPR_ZLIB) {
		strm->avail_in = ilen;
		strm->next_in = ibuff;
		rc = zlib_inflateInit(strm);
	} else if (compr_type == VDFS4_COMPR_GZIP) {
		strm->avail_in = ilen - 10;
		strm->next_in = ibuff + 10;
		rc = zlib_inflateInit2(strm, -MAX_WBITS);
	} else {
		VDFS4_ERR("Unsupported compression type\n");
		return -EIO;
	}

	if (rc != Z_OK) {
		VDFS4_ERR("zlib_inflateInit error %d", rc);
		return rc;
	}

	rc = zlib_inflate(strm, Z_SYNC_FLUSH);
	if ((rc != Z_OK) && (rc != Z_STREAM_END)) {
		VDFS4_ERR("zlib_inflate error %d", rc);
		rc = (rc == Z_NEED_DICT) ? VDFS4_Z_NEED_DICT_ERR : rc;
		return rc;
	}

	rc = zlib_inflateEnd(strm);
	return rc;
}

#ifdef VDFS4_DEBUG_DUMP
static void print_uncomp_err_dump_header(char *name, int name_len)
{
	VDFS4_ERR("--------------------------------------------------");
	VDFS4_ERR("Software decomression ERROR");
	VDFS4_ERR(" Current : %s(%d)", current->comm,
			task_pid_nr(current));
	VDFS4_ERR("--------------------------------------------------");
	VDFS4_ERR("== VDFS4 Debugger - %15s ===== Core : %2d ===="
			, VDFS4_VERSION, task_cpu(current));
	VDFS4_ERR("--------------------------------------------------");
	VDFS4_ERR("Source image name : %.*s", name_len, name);
	VDFS4_ERR("--------------------------------------------------");
}

static void print_zlib_error(int unzip_error)
{
	switch (unzip_error) {
	case (Z_ERRNO):
		VDFS4_ERR("File operation error %d", Z_ERRNO);
		break;
	case (Z_STREAM_ERROR):
		VDFS4_ERR("The stream state was inconsistent %d",
				Z_STREAM_ERROR);
		break;
	case (Z_DATA_ERROR):
		VDFS4_ERR("Stream was freed prematurely %d", Z_DATA_ERROR);
		break;
	case (Z_MEM_ERROR):
		VDFS4_ERR("There was not enough memory %d", Z_MEM_ERROR);
		break;
	case (Z_BUF_ERROR):
		VDFS4_ERR("no progress is possible or if there was not "
				"enough room in the output buffer %d",
				Z_BUF_ERROR);
		break;
	case (Z_VERSION_ERROR):
		VDFS4_ERR("zlib library version is incompatible with"
			" the version assumed by the caller %d",
			Z_VERSION_ERROR);
		break;
	case (VDFS4_Z_NEED_DICT_ERR):
		VDFS4_ERR(" The Z_NEED_DICT error happened %d",
				VDFS4_Z_NEED_DICT_ERR);
		break;
	default:
		VDFS4_ERR("Unknown error code %d", unzip_error);
		break;
	}

}

void vdfs4_dump_fbc_error(struct vdfs4_inode_info *inode_i, void *packed,
		struct vdfs4_comp_extent_info *cext)
{
	struct vdfs4_sb_info *sbi = VDFS4_SB(inode_i->vfs_inode.i_sb);
	char *fname = inode_i->name;
	int fname_len = fname ? (int)strlen(fname) : 0;
	void *chunk;
	char *file_name = NULL;
	int name_length;
	struct vdfs4_extent_info extent;
	int ret;

	mutex_lock(&sbi->dump_meta);

	chunk = (char *)packed + cext->offset;
	print_uncomp_err_dump_header(fname, fname_len);

	memset(&extent, 0x0, sizeof(extent));
	ret = vdfs4_get_iblock_extent(&inode_i->vfs_inode,
				cext->start_block, &extent, 0);
	if (ret || (extent.first_block == 0))
		goto dump_to_console;


	file_name = kmalloc(VDFS4_FILE_NAME_LEN, GFP_NOFS);
	if (!file_name)
		VDFS4_WARNING("(NOMEM)(%s) cannot allocate space for file name\n",
			      get_sid_from_sbi(sbi));
	else {
		name_length = snprintf(file_name, VDFS4_FILE_NAME_LEN,
				"%lu_%.*s_%d_%llu_%d.dump",
			inode_i->vfs_inode.i_ino,
			fname_len,
			fname,
			(cext->flags & VDFS4_CHUNK_FLAG_UNCOMPR),
			((extent.first_block +
			cext->start_block) << 12) +
			(sector_t)cext->offset,
			(int)(cext->blocks_n << PAGE_SHIFT));


		if (name_length > 0)
			vdfs4_dump_to_disk(packed,
				(size_t)(cext->blocks_n << PAGE_SHIFT),
				(const char *)file_name);
	}
dump_to_console:
	VDFS4_WARNING("chunk info:\n\t"
			"offset = %llu\n\t"
			"len_bytes = %d\n\t"
			"blocks_n = %d\n\t"
			"is_uncompr = %d",
			((extent.first_block +
			cext->start_block) << 12) + (sector_t)cext->offset,
			cext->len_bytes, cext->blocks_n,
			(int)cext->flags & VDFS4_CHUNK_FLAG_UNCOMPR);

	VDFS4_MDUMP("", chunk, (size_t)cext->len_bytes);
	mutex_unlock(&sbi->dump_meta);
	vdfs4_print_volume_verification(sbi);
	kfree(file_name);
}
#endif

static int _unpack_chunk_zlib_gzip(void *src, void *dst, size_t offset,
		size_t len_bytes, size_t chunk_size, enum compr_type compr_type)
{
	z_stream strm;
	int ret = 0;

	strm.workspace =
		vdfs4_vmalloc((unsigned long)zlib_inflate_workspacesize());
	if (!strm.workspace)
		return -ENOMEM;

	ret = sw_decompress(&strm, (char *)src + offset, len_bytes, dst,
			chunk_size, compr_type);

	if (ret) {
#ifdef VDFS4_DEBUG_DUMP
		print_zlib_error(ret);
#endif
		ret = -EIO;
	}

	vfree(strm.workspace);
	return ret;
}

#ifdef CONFIG_ZSTD_DECOMPRESS
/*
 * The raw ZSTD_decompress()/ZSTD_getErrorString() libzstd entry points
 * declared by linux/zstd_lib.h are compiled into vmlinux but not
 * exported to modules on this kernel - only the workspace-based
 * zstd_*_dctx() wrapper API (linux/zstd.h's own lowercase functions) is.
 */
int vdfs4_unpack_chunk_zstd(void *src, void *dst, size_t offset,
		size_t len_bytes, size_t chunk_size)
{
	size_t out_len, workspace_size;
	void *workspace;
	zstd_dctx *dctx;
	int ret = 0;

	workspace_size = zstd_dctx_workspace_bound();
	workspace = vdfs4_vmalloc((unsigned long)workspace_size);
	if (!workspace)
		return -ENOMEM;

	dctx = zstd_init_dctx(workspace, workspace_size);
	if (!dctx) {
		vfree(workspace);
		return -ENOMEM;
	}

	out_len = zstd_decompress_dctx(dctx, dst, chunk_size,
				  (char *)src + offset, len_bytes);
	if (zstd_is_error(out_len)) {
		VDFS4_ERR("ZSTD decomp error(%s,off:%zu,len:%zu,olen:%zu)\n",
			  zstd_get_error_name(out_len),
			  offset, len_bytes, chunk_size);
		ret = -EFAULT;
	}

	vfree(workspace);
	return ret;
}
#else
int vdfs4_unpack_chunk_zstd(void *src, void *dst, size_t offset,
		size_t len_bytes, size_t chunk_size)
{
	VDFS4_ERR("This kernel doesn't support zstd decompression\n");
	return -EINVAL;
}
#endif

int vdfs4_unpack_chunk_zlib(void *src, void *dst, size_t offset,
		size_t len_bytes, size_t chunk_size)
{
	return _unpack_chunk_zlib_gzip(src, dst, offset, len_bytes, chunk_size,
			VDFS4_COMPR_ZLIB);
}

int vdfs4_unpack_chunk_gzip(void *src, void *dst, size_t offset,
		size_t len_bytes, size_t chunk_size)
{
	return _unpack_chunk_zlib_gzip(src, dst, offset, len_bytes, chunk_size,
			VDFS4_COMPR_GZIP);
}

int vdfs4_unpack_chunk_lzo(void *src, void *dst, size_t offset,
		size_t len_bytes, size_t chunk_size)
{
	int res = 0;
	const unsigned char *packed_data;

	packed_data = (char *)src + offset;

	res = lzo1x_decompress_safe(packed_data, (size_t)len_bytes, dst,
			&chunk_size);
	if (res != LZO_E_OK)
		res = -EIO;

	return res;
}
