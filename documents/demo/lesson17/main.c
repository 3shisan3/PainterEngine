#include "PainterEngine.h"

const px_char shellcode[] = "\
#name \"main\"\n\
host void print(string s);\n\
host void sleep(int ms);\n\
int main()\n\
{\n\
 int i,j;\n\
 for(i = 1; i <= 9; i++)\n\
 {\n\
  for(j = 1; j <= i; j++)\n\
   {\n\
     print(string(i) + \" * \" + string(j) + \" = \" + string(i * j));\n\
     sleep(1000);\n\
   }\n\
 }\n\
}";

PX_VM_HOST_FUNCTION(host_print)
{
	if (PX_VM_HOSTPARAM(Ins, 0).type == PX_VARIABLE_TYPE_STRING)
	{
		PainterEngine_Print(PX_VM_HOSTPARAM(Ins, 0)._string.buffer);
	}
	return PX_TRUE;
}

PX_VM_HOST_FUNCTION(host_sleep)
{
	if (PX_VM_HOSTPARAM(Ins, 0).type == PX_VARIABLE_TYPE_INT)
	{
		PX_VM_Sleep(Ins,PX_VM_HOSTPARAM(Ins, 0)._int);
	}
	return PX_TRUE;
}

PX_VM vm;
PX_VM_DebuggerMap debugmap;


px_int main()
{
	PX_Compiler compiler;
	px_memory bin;
	PainterEngine_Initialize(800, 480);
	PX_VMDebuggerMapInitialize(mp,&debugmap);
	PainterEngine_SetBackgroundColor(PX_COLOR_BLACK);
	PX_CompilerInitialize(mp, &compiler);//��ʼ��������
	PX_CompilerAddSource(&compiler, shellcode);//����������Ӵ���
	PX_MemoryInitialize(mp, &bin);//��ʼ���ڴ�/���ڴ洢�����Ľ��
	if (!PX_CompilerCompile(&compiler, &bin, &debugmap, "main"))
	{
		//����ʧ��
		return 0;
	}
	PX_CompilerFree(&compiler);//�ͷű�����
	PX_VMInitialize(&vm,mp,bin.buffer,bin.usedsize);//��ʼ�������
	PX_VMRegisterHostFunction(&vm, "print", host_print,0);//ע����������print
	PX_VMRegisterHostFunction(&vm, "sleep", host_sleep,0);//ע����������sleep
	PX_VMBeginThreadFunction(&vm, 0, "main", PX_NULL, 0);//��ʼ�������������
	PX_Object *pDbgObject = PX_Object_AsmDebuggerCreate(mp, root, 0, 0, 800, 480, 0);
	pDbgObject->Visible = PX_TRUE;
	PX_Object_AsmDebuggerAttach(pDbgObject, &debugmap, &vm);
	return 0;
}