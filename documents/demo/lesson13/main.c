#include "PainterEngine.h"
px_texture tex1;
PX_Object* image;

px_int main()
{
	PainterEngine_Initialize(800, 480);
	if(!PX_LoadTextureFromFile(mp_static,&tex1,"assets/1.png")) return 0;//��������1
	image=PX_Object_ImageCreate(mp,root,300,140,200,200,&tex1);//����Image����
	PX_Object_DragAttachObject(image, 1);//��һ��Drag����������ϵ�Image������
	return 1;
}