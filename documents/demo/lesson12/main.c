#include "PainterEngine.h"
typedef struct
{
	px_float last_cursorx, last_cursory;
	px_bool bselect;
	px_texture image;
	px_float rotation;
}PX_Object_MyObject;

PX_OBJECT_UPDATE_FUNCTION(MyObjectUpdate)
{
}

PX_OBJECT_RENDER_FUNCTION(MyObjectRender)
{
	PX_Object_MyObject *pMyObject=PX_ObjectGetDesc(PX_Object_MyObject,pObject);
	PX_TextureRenderRotation(psurface, &pMyObject->image, (px_int)pObject->x, (px_int)pObject->y, PX_ALIGN_CENTER,0, (px_int)pMyObject->rotation);//��ȾͼƬ
}

PX_OBJECT_FREE_FUNCTION(MyObjectFree)
{
	PX_Object_MyObject *pMyObject=PX_ObjectGetDesc(PX_Object_MyObject,pObject);
	PX_TextureFree(&pMyObject->image);//�ͷ�ͼƬ
}

PX_OBJECT_EVENT_FUNCTION(MyObjectOnCursorWheel)
{
	PX_Object_MyObject *pMyObject=PX_ObjectGetDescIndex(PX_Object_MyObject,pObject,0);
	if(PX_ObjectIsCursorInRegionAlign(pObject,e,PX_ALIGN_CENTER))//Object�����λ���Ƿ�ѡ�е�ǰ�����e���¼�
		pMyObject->rotation += (px_float)PX_Object_Event_GetCursorZ(e)/10;
}

PX_OBJECT_EVENT_FUNCTION(MyObjectOnCursorDown)
{
	PX_Object_MyObject* pMyObject = PX_ObjectGetDescIndex(PX_Object_MyObject, pObject, 0);
	if (PX_ObjectIsCursorInRegionAlign(pObject, e, PX_ALIGN_CENTER))//Object�����λ���Ƿ�ѡ�е�ǰ�����e���¼�
	{
		pMyObject->bselect = PX_TRUE;
		pMyObject->last_cursorx = PX_Object_Event_GetCursorX(e);
		pMyObject->last_cursory = PX_Object_Event_GetCursorY(e);
	}
}

PX_OBJECT_EVENT_FUNCTION(MyObjectOnCursorRelease)
{
	PX_Object_MyObject* pMyObject = PX_ObjectGetDescIndex(PX_Object_MyObject, pObject, 0);
	pMyObject->bselect = PX_FALSE;
}

PX_OBJECT_EVENT_FUNCTION(MyObjectOnCursorDrag)
{
	PX_Object_MyObject* pMyObject = PX_ObjectGetDescIndex(PX_Object_MyObject, pObject, 0);
	if (pMyObject->bselect)
	{
		pObject->x += PX_Object_Event_GetCursorX(e) - pMyObject->last_cursorx;
		pObject->y += PX_Object_Event_GetCursorY(e) - pMyObject->last_cursory;
	}
	pMyObject->last_cursorx = PX_Object_Event_GetCursorX(e);
	pMyObject->last_cursory = PX_Object_Event_GetCursorY(e);
}

PX_Object* PX_Object_MyObjectCreate(px_memorypool* mp, PX_Object* parent, px_float x, px_float y)
{
	PX_Object *pObject=PX_ObjectCreateEx(mp,parent,x,y,0,128,128,0,0, MyObjectUpdate, MyObjectRender, MyObjectFree,0,sizeof(PX_Object_MyObject));//����һ���յ��Զ������
	PX_Object_MyObject* pMyObject = PX_ObjectGetDescIndex(PX_Object_MyObject, pObject,0);//ȡ���Զ����������
	pMyObject->rotation = 0;
	if(!PX_LoadTextureFromFile(mp,&pMyObject->image, "assets/test.png"))//����ͼƬ
	{
		PX_ObjectDelete(pObject);//����ʧ����ɾ������
		return PX_NULL;
	}
	PX_ObjectRegisterEvent(pObject,PX_OBJECT_EVENT_CURSORWHEEL,MyObjectOnCursorWheel,0);//ע���������¼�
	PX_ObjectRegisterEvent(pObject,PX_OBJECT_EVENT_CURSORDRAG,MyObjectOnCursorDrag,0);//ע�������ק�¼�
	PX_ObjectRegisterEvent(pObject,PX_OBJECT_EVENT_CURSORDOWN,MyObjectOnCursorDown,0);//ע����갴���¼�
	PX_ObjectRegisterEvent(pObject,PX_OBJECT_EVENT_CURSORUP,MyObjectOnCursorRelease,0);//ע������ͷ��¼�
	PX_ObjectRegisterEvent(pObject, PX_OBJECT_EVENT_CURSORMOVE, MyObjectOnCursorRelease, 0);//ע������ͷ��¼�
	return pObject;
}

px_int main()
{
	PainterEngine_Initialize(800, 480);
	PX_Object_MyObjectCreate(mp,root,400,240);//����һ���Զ������
	PX_Object_MyObjectCreate(mp, root, 100, 140);//����һ���Զ������
	PX_Object_MyObjectCreate(mp, root, 300, 100);//����һ���Զ������
	return PX_TRUE;
}