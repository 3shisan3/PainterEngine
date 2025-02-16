#include "PX_Abi.h"

px_void PX_AbiCreateStaticWriter(px_abi* pabi, px_void* pbuffer, px_int size)
{
	PX_memset(pabi, 0, sizeof(px_abi));
	pabi->pstatic_buffer = pbuffer;
	pabi->static_size = size;
}

px_void PX_AbiCreateStaticReader(px_abi* pabi, px_void* pbuffer, px_int size)
{
	PX_memset(pabi, 0, sizeof(px_abi));
	pabi->pstatic_buffer = pbuffer;
	pabi->static_size = size;
	pabi->static_write_offset = size;
}

px_void PX_AbiCreateDynamicWriter(px_abi* pabi, px_memorypool* mp)
{
	PX_memset(pabi, 0, sizeof(px_abi));
	PX_MemoryInitialize(mp, &pabi->dynamic);
}

px_byte *PX_AbiGetPtr(px_abi* pabi)
{
	if (pabi->dynamic.mp)
	{
		return pabi->dynamic.buffer;
	}
	else
	{
		return pabi->pstatic_buffer;
	}
}


px_byte* PX_AbiGetRawDataPtr(px_byte* pStartBuffer)
{
	return pStartBuffer + sizeof(PX_ABI_TYPE) + sizeof(px_dword) + PX_strlen((px_char*)(pStartBuffer + sizeof(PX_ABI_TYPE) + sizeof(px_dword)) + 1);
}

px_byte* PX_AbiGetRawPtr(px_abi *pabi, const px_char name[], PX_ABI_TYPE type)
{
	px_int readoffset = 0;
	px_byte* pbuffer, *pstart;
	pbuffer = PX_AbiGetPtr(pabi);
	pstart = pbuffer;
	while (PX_TRUE)
	{
		px_dword size;
		const px_char* pname;
		px_byte *prawstart= pbuffer;
		PX_ABI_TYPE _type;
		if (PX_AbiGetPtrSize(pabi)<sizeof(PX_ABI_TYPE)+sizeof(size))
		{
			return PX_NULL;
		}
		PX_memcpy(&_type, pbuffer, sizeof(_type));
		pbuffer += sizeof(PX_ABI_TYPE);
		PX_memcpy(&size, pbuffer, sizeof(size));
		pbuffer += sizeof(size);
		pname = (px_char*)(pbuffer);
		pbuffer += size;

		if (pbuffer - pstart > PX_AbiGetPtrSize(pabi))
		{
			break;
		}

		if (_type == type&&PX_strequ(pname, name))
		{
			return prawstart;
		}
		prawstart= pbuffer;
	}
	return PX_NULL;
}


px_dword PX_AbiGetRawDataSize(px_byte* pStartBuffer)
{
	return *(px_dword *)(pStartBuffer + sizeof(PX_ABI_TYPE));
}
px_dword PX_AbiGetRawBlockSize(px_byte* pStartBuffer)
{
	return sizeof(PX_ABI_TYPE) + sizeof(px_dword) + PX_strlen((px_char*)(pStartBuffer + sizeof(PX_ABI_TYPE) + sizeof(px_dword)) + 1) + PX_AbiGetRawDataSize(pStartBuffer);
}

px_int  PX_AbiGetPtrSize(px_abi* pabi)
{
	if (pabi->dynamic.mp)
	{
		return pabi->dynamic.usedsize;
	}
	else
	{
		return pabi->static_write_offset;
	}
}

px_bool PX_AbiWriteMemory(px_memory* pmemory, PX_ABI_TYPE type, const px_char name[], px_void* buffer, px_int buffersize)
{
	px_dword size;
	size = PX_strlen(name) + 1 + buffersize;
	if (!PX_MemoryCat(pmemory, &type, sizeof(type)))
	{
		return PX_FALSE;
	}
	if (!PX_MemoryCat(pmemory, &size, sizeof(size)))
	{
		return PX_FALSE;
	}
	if (!PX_MemoryCat(pmemory, name, PX_strlen(name) + 1))
	{
		return PX_FALSE;
	}
	if (!PX_MemoryCat(pmemory, buffer, buffersize))
	{
		return PX_FALSE;
	}
	return PX_TRUE;
}

px_bool PX_AbiWrite(px_abi* pabi, PX_ABI_TYPE type, const px_char name[], px_void* buffer, px_int buffersize)
{
	if (pabi->dynamic.mp)
	{
		return PX_AbiWriteMemory(&pabi->dynamic, type, name, buffer, buffersize);
	}
	else
	{
		px_dword size;
		size = PX_strlen(name) + 1 + buffersize;
		if (pabi->static_write_offset + sizeof(type) + sizeof(size) + PX_strlen(name) + 1 + buffersize > pabi->static_size)
		{
			return PX_FALSE;
		}
		PX_memcpy(pabi->pstatic_buffer + pabi->static_write_offset, &type, sizeof(type));
		pabi->static_write_offset += sizeof(type);
		PX_memcpy(pabi->pstatic_buffer + pabi->static_write_offset, &size, sizeof(size));
		pabi->static_write_offset += sizeof(size);
		PX_memcpy(pabi->pstatic_buffer + pabi->static_write_offset, name, PX_strlen(name) + 1);
		pabi->static_write_offset += PX_strlen(name) + 1;
		PX_memcpy(pabi->pstatic_buffer + pabi->static_write_offset, buffer, buffersize);
		pabi->static_write_offset += buffersize;
		return PX_TRUE;
	}
}

px_bool PX_AbiDelete(px_abi* pabi, PX_ABI_TYPE type, const px_char name[])
{
	if (pabi->dynamic.mp)
	{
		px_byte *pRawPtr = PX_AbiGetRawPtr(pabi, name, type);
		if (pRawPtr)
		{
			px_int start = (px_int)(PX_AbiGetRawPtr(pabi, name, type) - PX_AbiGetPtr(pabi));
			px_int end = start + PX_AbiGetRawBlockSize(PX_AbiGetRawPtr(pabi, name, type)) - 1;
			PX_MemoryRemove(&pabi->dynamic, start, end);
			return PX_TRUE;
		}
	}
	else
	{
		px_byte* pRawPtr = PX_AbiGetRawPtr(pabi, name, type);
		if (pRawPtr)
		{
			px_int start = (px_int)(PX_AbiGetRawPtr(pabi, name, type) - PX_AbiGetPtr(pabi));
			px_int end = start + PX_AbiGetRawBlockSize(PX_AbiGetRawPtr(pabi, name, type)) - 1;
			PX_memcpy(pRawPtr+ start, pRawPtr + end, pabi->static_write_offset - end);
			pabi->static_write_offset -= end - start;
			return PX_TRUE;
		}
	}
	return PX_FALSE;

}

px_bool PX_AbiSet(px_abi* pabi, PX_ABI_TYPE type, const px_char name[], px_void* buffer, px_int buffersize)
{
	PX_AbiDelete(pabi, type, name);
	return PX_AbiWrite(pabi, type, name, buffer, buffersize);
}



px_bool PX_AbiWrite_int(px_abi* pabi, const px_char name[], px_int _int)
{
	return PX_AbiWrite(pabi, PX_ABI_TYPE_INT, name, &_int, sizeof(_int));
}
px_bool PX_AbiWrite_dword(px_abi* pabi, const px_char name[], px_dword _dword)
{
	return PX_AbiWrite(pabi, PX_ABI_TYPE_DWORD, name, &_dword, sizeof(_dword));
}
px_bool PX_AbiWrite_word(px_abi* pabi, const px_char name[], px_word _word)
{
	return PX_AbiWrite(pabi, PX_ABI_TYPE_WORD, name, &_word, sizeof(_word));
}
px_bool PX_AbiWrite_byte(px_abi* pabi, const px_char name[], px_byte _byte)
{
	return PX_AbiWrite(pabi, PX_ABI_TYPE_BYTE, name, &_byte, sizeof(_byte));
}
px_bool PX_AbiWrite_ptr(px_abi* pabi, const px_char name[], px_void* ptr)
{
	return PX_AbiWrite(pabi, PX_ABI_TYPE_PTR, name, &ptr, sizeof(ptr));
}
px_bool PX_AbiWrite_float(px_abi* pabi, const px_char name[], px_float _float)
{
	return PX_AbiWrite(pabi, PX_ABI_TYPE_FLOAT, name, &_float, sizeof(_float));
}
px_bool PX_AbiWrite_double(px_abi* pabi, const px_char name[], px_double _double)
{
	return PX_AbiWrite(pabi, PX_ABI_TYPE_DOUBLE, name, &_double, sizeof(_double));
}
px_bool PX_AbiWrite_string(px_abi* pabi, const px_char name[], const px_char _string[])
{
	return PX_AbiWrite(pabi, PX_ABI_TYPE_STRING, name, (px_void*)_string, PX_strlen(_string) + 1);
}
px_bool PX_AbiWrite_point(px_abi* pabi, const px_char name[], px_point point)
{
	return PX_AbiWrite(pabi, PX_ABI_TYPE_POINT, name, &point, sizeof(point));
}
px_bool PX_AbiWrite_color(px_abi* pabi, const px_char name[], px_color color)
{
	return PX_AbiWrite(pabi, PX_ABI_TYPE_COLOR, name, &color, sizeof(color));
}
px_bool PX_AbiWrite_bool(px_abi* pabi, const px_char name[], px_bool _bool)
{
	return PX_AbiWrite(pabi, PX_ABI_TYPE_BOOL, name, &_bool, sizeof(_bool));
}
px_bool PX_AbiWrite_data(px_abi* pabi, const px_char name[], px_void* data, px_int size)
{
	return PX_AbiWrite(pabi, PX_ABI_TYPE_DATA, name, data, size);
}

px_bool PX_AbiSet_int(px_abi* pabi, const px_char name[], px_int _int)
{
	return PX_AbiSet(pabi, PX_ABI_TYPE_INT, name, &_int, sizeof(_int));
}
px_bool PX_AbiSet_dword(px_abi* pabi, const px_char name[], px_dword _dword)
{
	return PX_AbiSet(pabi, PX_ABI_TYPE_DWORD, name, &_dword, sizeof(_dword));
}
px_bool PX_AbiSet_word(px_abi* pabi, const px_char name[], px_word _word)
{
	return PX_AbiSet(pabi, PX_ABI_TYPE_WORD, name, &_word, sizeof(_word));
}
px_bool PX_AbiSet_byte(px_abi* pabi, const px_char name[], px_byte _byte)
{
	return PX_AbiSet(pabi, PX_ABI_TYPE_BYTE, name, &_byte, sizeof(_byte));
}
px_bool PX_AbiSet_ptr(px_abi* pabi, const px_char name[], px_void* ptr)
{
	return PX_AbiSet(pabi, PX_ABI_TYPE_PTR, name, &ptr, sizeof(ptr));
}
px_bool PX_AbiSet_float(px_abi* pabi, const px_char name[], px_float _float)
{
	return PX_AbiSet(pabi, PX_ABI_TYPE_FLOAT, name, &_float, sizeof(_float));
}
px_bool PX_AbiSet_double(px_abi* pabi, const px_char name[], px_double _double)
{
	return PX_AbiSet(pabi, PX_ABI_TYPE_DOUBLE, name, &_double, sizeof(_double));
}
px_bool PX_AbiSet_string(px_abi* pabi, const px_char name[], const px_char _string[])
{
	return PX_AbiSet(pabi, PX_ABI_TYPE_STRING, name, (px_void*)_string, PX_strlen(_string) + 1);
}
px_bool PX_AbiSet_point(px_abi* pabi, const px_char name[], px_point point)
{
	return PX_AbiSet(pabi, PX_ABI_TYPE_POINT, name, &point, sizeof(point));
}
px_bool PX_AbiSet_color(px_abi* pabi, const px_char name[], px_color color)
{
	return PX_AbiSet(pabi, PX_ABI_TYPE_COLOR, name, &color, sizeof(color));
}
px_bool PX_AbiSet_bool(px_abi* pabi, const px_char name[], px_bool _bool)
{
	return PX_AbiSet(pabi, PX_ABI_TYPE_BOOL, name, &_bool, sizeof(_bool));
}
px_bool PX_AbiSet_data(px_abi* pabi, const px_char name[], px_void* data, px_int size)
{
	return PX_AbiSet(pabi, PX_ABI_TYPE_DATA, name, data, size);
}


px_bool PX_AbiMemoryWrite_int(px_memory* pmem, const px_char name[], px_int _int)
{
	return PX_AbiWriteMemory(pmem, PX_ABI_TYPE_INT, name, &_int, sizeof(_int));
}

px_bool PX_AbiMemoryWrite_dword(px_memory* pmem, const px_char name[], px_dword _dword)
{
		return PX_AbiWriteMemory(pmem, PX_ABI_TYPE_DWORD, name, &_dword, sizeof(_dword));

}
px_bool PX_AbiMemoryWrite_word(px_memory* pmem, const px_char name[], px_word _word)
{
		return PX_AbiWriteMemory(pmem, PX_ABI_TYPE_WORD, name, &_word, sizeof(_word));

}
px_bool PX_AbiMemoryWrite_byte(px_memory* pmem, const px_char name[], px_byte _byte)
{
	return PX_AbiWriteMemory(pmem, PX_ABI_TYPE_BYTE, name, &_byte, sizeof(_byte));
}

px_bool PX_AbiMemoryWrite_ptr(px_memory* pmem, const px_char name[], px_void* ptr)
{
	return PX_AbiWriteMemory(pmem, PX_ABI_TYPE_PTR, name, &ptr, sizeof(ptr));
}
px_bool PX_AbiMemoryWrite_float(px_memory* pmem, const px_char name[], px_float _float)
{
	return PX_AbiWriteMemory(pmem, PX_ABI_TYPE_FLOAT, name, &_float, sizeof(_float));
}
px_bool PX_AbiMemoryWrite_double(px_memory* pmem, const px_char name[], px_double _double)
{
	return PX_AbiWriteMemory(pmem, PX_ABI_TYPE_DOUBLE, name, &_double, sizeof(_double));
}
px_bool PX_AbiMemoryWrite_string(px_memory* pmem, const px_char name[], const px_char _string[])
{
	return PX_AbiWriteMemory(pmem, PX_ABI_TYPE_STRING, name, (px_void*)_string, PX_strlen(_string) + 1);
}
px_bool PX_AbiMemoryWrite_point(px_memory* pmem, const px_char name[], px_point point)
{
	return PX_AbiWriteMemory(pmem, PX_ABI_TYPE_POINT, name, &point, sizeof(point));
}
px_bool PX_AbiMemoryWrite_color(px_memory* pmem, const px_char name[], px_color color)
{
	return PX_AbiWriteMemory(pmem, PX_ABI_TYPE_COLOR, name, &color, sizeof(color));
}
px_bool PX_AbiMemoryWrite_bool(px_memory* pmem, const px_char name[], px_bool _bool)
{
	return PX_AbiWriteMemory(pmem, PX_ABI_TYPE_BOOL, name, &_bool, sizeof(_bool));
}
px_bool PX_AbiMemoryWrite_data(px_memory* pmem, const px_char name[], px_void* data, px_int size)
{
	return PX_AbiWriteMemory(pmem, PX_ABI_TYPE_DATA, name, data, size);
}



px_void* PX_AbiRead(px_abi* pabi, PX_ABI_TYPE type, const px_char name[],px_int *pdatasize)
{
	PX_ABI_TYPE *ptype;
	px_dword readoffset=0;
	px_dword readsize;
	px_int datasize;
	px_byte* pbuffer;
	if (!pabi)
	{
		return PX_NULL;
	}
	*pdatasize=0;
	pbuffer=PX_AbiGetPtr(pabi);
	readsize=PX_AbiGetPtrSize(pabi);

	while (readoffset<readsize)
	{
		px_dword size;
		ptype = (PX_ABI_TYPE *)(pbuffer + readoffset);
		readoffset += sizeof(PX_ABI_TYPE);
		if (readoffset >= readsize)
			break;
		PX_memcpy(&size, pbuffer + readoffset, sizeof(size));
		readoffset += sizeof(size);
		if (size==0||size>= readsize|| readoffset + size > readsize)
		{
			return PX_NULL;
		}

		if (*ptype == type)
		{
			if (PX_strequ((px_char*)(pbuffer + readoffset), name))
			{
				px_int _strlen=PX_strlen(name) + 1;
				if (_strlen==1)
				{
					return PX_NULL;
				}
				if (readoffset+ _strlen>= readsize)
				{
					return PX_NULL;
				}
				readoffset += _strlen;
				datasize = size - _strlen;
				if (pdatasize)
				{
					*pdatasize = datasize;
				}
				if (type== PX_ABI_TYPE_STRING)
				{
					px_char *pstr = (px_char*)(pbuffer + readoffset);
					if (pstr[datasize-1]==0)
					{
						return pbuffer + readoffset;
					}
					else
					{
						return PX_NULL;
					}
				}
				return pbuffer + readoffset;
			}
			else
			{
				readoffset += size;
			}
		}
		else
		{
			readoffset += size;
		}
	}
	return PX_NULL;
}

px_byte* PX_AbiGetDataPtr(px_abi* pabi, const px_char name[])
{
	px_int readoffset = 0;
	px_byte* pbuffer,*pstart;
	pbuffer=PX_AbiGetPtr(pabi);
	pstart =pbuffer;
	while (PX_TRUE)
	{
		px_dword size;
		const px_char* pname;
		px_byte* databuffer;
		PX_ABI_TYPE type = *(PX_ABI_TYPE*)(pbuffer);
		pbuffer += sizeof(PX_ABI_TYPE);
		PX_memcpy(&size, pbuffer, sizeof(size));
		pbuffer += sizeof(size);
		pname = (px_char*)(pbuffer);
		databuffer = pbuffer + PX_strlen(pname) + 1;
		pbuffer += size;

		if (pbuffer - pstart > PX_AbiGetPtrSize(pabi))
		{
			break;
		}

		if (PX_strequ(pname, name))
		{
			return databuffer;
		}
		
	}
	return PX_NULL;
}

px_int PX_AbiRead_int(px_abi* pabi, const px_char name[], px_int* _int)
{
	px_int datasize;
	px_int *pint=(px_int*)PX_AbiRead(pabi, PX_ABI_TYPE_INT, name,&datasize);
	if (pint)
	{
		*_int = *pint;
		return datasize;
	}
	return 0;
}

px_int PX_AbiRead_dword(px_abi* pabi, const px_char name[], px_dword* _dword)
{
	px_int datasize;
	px_dword *pdword = (px_dword*)PX_AbiRead(pabi, PX_ABI_TYPE_DWORD, name, &datasize);
	if (pdword)
	{
		*_dword = *pdword;
		return datasize;
	}
	return 0;
}

px_int PX_AbiRead_word(px_abi* pabi, const px_char name[], px_word* _word)
{
	px_int datasize;
	px_word *pdword = (px_word*)PX_AbiRead(pabi, PX_ABI_TYPE_WORD, name, &datasize);
	if (pdword)
	{
		*_word = *pdword;
		return datasize;
	}
	return 0;
}

px_int PX_AbiRead_byte(px_abi* pabi, const px_char name[], px_byte* _byte)
{
	px_int datasize;
	px_byte *pbyte = (px_byte*)PX_AbiRead(pabi, PX_ABI_TYPE_BYTE, name, &datasize);
	if (pbyte)
	{
		*_byte = *pbyte;
		return datasize;
	}
	return 0;
}


px_int PX_AbiRead_ptr(px_abi* pabi, const px_char name[], px_void** pptr)
{
	px_int datasize;
	px_void** ptr = (px_void**)PX_AbiRead(pabi, PX_ABI_TYPE_PTR, name, &datasize);
	if (ptr)
	{
		*pptr = *ptr;
		return datasize;
	}
	return 0;

}
px_int PX_AbiRead_float(px_abi* pabi, const px_char name[], px_float* _float)
{
	px_int datasize;
	px_float *pfloat = (px_float*)PX_AbiRead(pabi, PX_ABI_TYPE_FLOAT, name, &datasize);
	if (pfloat)
	{
		*_float = *pfloat;
		return datasize;
	}
	return 0;
}
px_int PX_AbiRead_double(px_abi* pabi, const px_char name[], px_double* _double)
{
	px_int datasize;
	px_double *pdouble = (px_double*)PX_AbiRead(pabi, PX_ABI_TYPE_DOUBLE, name, &datasize);
	if (pdouble)
	{
		*_double = *pdouble;
		return datasize;
	}
	return 0;

}
px_int PX_AbiRead_string2(px_abi* pabi, const px_char name[], px_char *_string[])
{
	px_int datasize;
	px_char *pstring = (px_char*)PX_AbiRead(pabi, PX_ABI_TYPE_STRING, name, &datasize);
	if (pstring)
	{
		*_string = pstring;
		return datasize;
	}
	return 0;
}

px_int PX_AbiRead_string(px_abi* pabi, const px_char name[], px_char _string[],px_int size)
{
	px_int datasize;
	px_char *pstring = (px_char*)PX_AbiRead(pabi, PX_ABI_TYPE_STRING, name, &datasize);
	if (pstring&&datasize<=size)
	{
		PX_strcpy(_string,  pstring, size);
		return datasize;
	}
	return 0;
}

px_int PX_AbiRead_point(px_abi* pabi, const px_char name[], px_point* point)
{
	px_int datasize;
	px_point *ppoint = (px_point*)PX_AbiRead(pabi, PX_ABI_TYPE_POINT, name, &datasize);
	if (ppoint)
	{
		*point = *ppoint;
		return datasize;
	}
	return 0;

}

px_int PX_AbiRead_color(px_abi* pabi, const px_char name[], px_color* color)
{
	px_int datasize;
	px_color *pcolor = (px_color*)PX_AbiRead(pabi, PX_ABI_TYPE_COLOR, name, &datasize);
	if (pcolor)
	{
		*color = *pcolor;
		return datasize;
	}
	return 0;

}
px_int PX_AbiRead_bool(px_abi* pabi, const px_char name[], px_bool* _bool)
{
	px_int datasize;
	px_bool *pbool = (px_bool*)PX_AbiRead(pabi, PX_ABI_TYPE_BOOL, name, &datasize);
	if (pbool)
	{
		*_bool = *pbool;
		return datasize;
	}
	return PX_FALSE;
}
px_int PX_AbiRead_data(px_abi* pabi, const px_char name[], px_void* data, px_int size)
{
	px_int datasize;
	px_void *pdata = PX_AbiRead(pabi, PX_ABI_TYPE_DATA, name, &datasize);
	if (pdata&&datasize<=size)
	{
		PX_memcpy(data, pdata, size);
		return datasize;
	}
	return PX_FALSE;
}

px_int* PX_AbiGet_int(px_abi* pabi, const px_char name[])
{
	px_int datasize;
	return (px_int*)PX_AbiRead(pabi, PX_ABI_TYPE_INT, name, &datasize);
}
px_dword* PX_AbiGet_dword(px_abi* pabi, const px_char name[])
{
	px_int datasize;
	return (px_dword*)PX_AbiRead(pabi, PX_ABI_TYPE_DWORD, name, &datasize);
}
px_word* PX_AbiGet_word(px_abi* pabi, const px_char name[])
{
	px_int datasize;
	return (px_word*)PX_AbiRead(pabi, PX_ABI_TYPE_WORD, name, &datasize);
}
px_byte* PX_AbiGet_byte(px_abi* pabi, const px_char name[])
{
	px_int datasize;
	return (px_byte*)PX_AbiRead(pabi, PX_ABI_TYPE_BYTE, name, &datasize);
}
px_void** PX_AbiGet_ptr(px_abi* pabi, const px_char name[])
{
	px_int datasize;
	return (px_void**)PX_AbiRead(pabi, PX_ABI_TYPE_PTR, name, &datasize);
}
px_float* PX_AbiGet_float(px_abi* pabi, const px_char name[])
{
	px_int datasize;
	return (px_float*)PX_AbiRead(pabi, PX_ABI_TYPE_FLOAT, name, &datasize);
}
px_double* PX_AbiGet_double(px_abi* pabi, const px_char name[])
{
	px_int datasize;
	return (px_double*)PX_AbiRead(pabi, PX_ABI_TYPE_DOUBLE, name, &datasize);
}
const px_char* PX_AbiGet_string(px_abi* pabi, const px_char name[])
{
	px_int datasize;
	return (const px_char*)PX_AbiRead(pabi, PX_ABI_TYPE_STRING, name, &datasize);
	
}
px_point* PX_AbiGet_point(px_abi* pabi, const px_char name[])
{
	px_int datasize;
	return (px_point*)PX_AbiRead(pabi, PX_ABI_TYPE_POINT, name, &datasize);
}
px_color* PX_AbiGet_color(px_abi* pabi, const px_char name[])
{
	px_int datasize;
	return (px_color*)PX_AbiRead(pabi, PX_ABI_TYPE_COLOR, name, &datasize);
}
px_bool* PX_AbiGet_bool(px_abi* pabi, const px_char name[])
{
	px_int datasize;
	return (px_bool*)PX_AbiRead(pabi, PX_ABI_TYPE_BOOL, name, &datasize);
}
px_void* PX_AbiGet_data(px_abi* pabi, const px_char name[], px_int* size)
{
	return PX_AbiRead(pabi, PX_ABI_TYPE_DATA, name, size);
}


px_void PX_AbiDynamicFree(px_abi* pabi)
{
	if (pabi->dynamic.mp)
	{
		PX_MemoryFree(&pabi->dynamic);
	}
	PX_memset(pabi, 0, sizeof(px_abi));
}

px_void PX_AbiFree(px_abi* pabi)
{
	if (pabi->dynamic.mp)
	{
		PX_AbiDynamicFree(pabi);
	}
}